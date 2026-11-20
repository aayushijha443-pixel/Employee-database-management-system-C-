// Employee Database Management System
// Backend: C++ (singly linked list + file saving + small web server)
// Compile: g++ server.cpp -o server      (Windows: add -lws2_32 at the end)
// Run    : ./server   then open the address it prints (http://localhost:xxxxx)

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>
#include <cctype>
#include <cstdlib>
#ifdef _WIN32
#include <winsock2.h>
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
typedef int SOCKET;
#define closesocket close
#endif
using namespace std;

// ---------- singly linked list ----------
struct Node {
    int id;
    string name, desig, dept;
    double sal;
    Node* next;
};
Node* head = NULL;

Node* findById(int id) {
    for (Node* c = head; c; c = c->next)
        if (c->id == id) return c;
    return NULL;
}

bool addEmp(int id, string n, string d, string dp, double s) {
    if (findById(id)) return false;               // duplicate id prevention
    Node* x = new Node();
    x->id = id; x->name = n; x->desig = d; x->dept = dp; x->sal = s; x->next = NULL;
    if (!head) head = x;
    else {
        Node* c = head;
        while (c->next) c = c->next;
        c->next = x;
    }
    return true;
}

bool delEmp(int id) {
    Node *prev = NULL, *c = head;
    while (c && c->id != id) { prev = c; c = c->next; }
    if (!c) return false;
    if (prev) prev->next = c->next; else head = c->next;
    delete c;
    return true;
}

// ---------- saving / loading (employees.txt, one employee per line) ----------
void save() {
    ofstream f("employees.txt");
    for (Node* c = head; c; c = c->next)
        f << c->id << "|" << c->name << "|" << c->desig << "|" << c->dept << "|" << c->sal << "\n";
}

void load() {
    ifstream f("employees.txt");
    string line;
    while (getline(f, line)) {
        stringstream ss(line);
        string p[5];
        int i = 0;
        while (i < 5 && getline(ss, p[i], '|')) i++;
        if (i == 5) addEmp(atoi(p[0].c_str()), p[1], p[2], p[3], atof(p[4].c_str()));
    }
}

// ---------- small helpers ----------
string lower(string s) {
    for (size_t i = 0; i < s.size(); i++) s[i] = tolower((unsigned char)s[i]);
    return s;
}

// trims spaces and removes '|' and control characters so the file format stays safe
string clean(string s) {
    for (size_t i = 0; i < s.size(); i++)
        if (s[i] == '|' || (unsigned char)s[i] < 32) s[i] = ' ';
    size_t a = s.find_first_not_of(' '), b = s.find_last_not_of(' ');
    return a == string::npos ? "" : s.substr(a, b - a + 1);
}

// turns "id=1&name=Ravi+K" into a map (also decodes %20 style characters)
map<string, string> parse(const string& d) {
    map<string, string> m;
    string k, v, *cur = &k;
    for (size_t i = 0; i <= d.size(); i++) {
        char c = i < d.size() ? d[i] : '&';
        if (c == '=') cur = &v;
        else if (c == '&') { if (!k.empty()) m[k] = v; k = v = ""; cur = &k; }
        else if (c == '+') *cur += ' ';
        else if (c == '%' && i + 2 < d.size()) {
            *cur += (char)strtol(d.substr(i + 1, 2).c_str(), NULL, 16);
            i += 2;
        } else *cur += c;
    }
    return m;
}

string esc(const string& s) {                       // makes text safe inside JSON
    string o;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '"' || s[i] == '\\') o += '\\';
        o += s[i];
    }
    return o;
}

string toJson(Node* e) {
    ostringstream o;
    o << "{\"id\":" << e->id << ",\"name\":\"" << esc(e->name) << "\",\"designation\":\""
      << esc(e->desig) << "\",\"department\":\"" << esc(e->dept) << "\",\"salary\":"
      << fixed << setprecision(2) << e->sal << "}";
    return o.str();
}

string msg(bool ok, string t) {
    return string("{\"ok\":") + (ok ? "true" : "false") + ",\"message\":\"" + esc(t) + "\"}";
}

// ---------- decides what to do for each request ----------
string handle(string method, string path, string body, int& status, string& type) {
    string query;
    size_t q = path.find('?');
    if (q != string::npos) { query = path.substr(q + 1); path = path.substr(0, q); }
    map<string, string> f = parse(method == "GET" ? query : body);
    status = 200;
    type = "application/json";

    if (path == "/") {                               // send the web page
        ifstream in("index.html", ios::binary);
        if (!in) { status = 404; type = "text/plain"; return "index.html not found"; }
        stringstream buf;
        buf << in.rdbuf();
        type = "text/html";
        return buf.str();
    }

    int id = atoi(f["id"].c_str());

    // show all / search by id / search by department
    if (path == "/api/all" || path == "/api/search" || path == "/api/department") {
        string want = lower(clean(f["name"])), out = "[";
        for (Node* c = head; c; c = c->next) {
            bool hit = path == "/api/all" || (path == "/api/search" && c->id == id) ||
                       (path == "/api/department" && lower(c->dept) == want);
            if (hit) out += (out.size() > 1 ? "," : "") + toJson(c);
        }
        return out + "]";
    }

    if (path == "/api/delete") {
        if (!delEmp(id)) { status = 404; return msg(false, "No employee found with that ID."); }
        save();
        return msg(true, "Employee deleted successfully.");
    }

    if (path == "/api/add" || path == "/api/update") {
        string n = clean(f["name"]), d = clean(f["designation"]), dp = clean(f["department"]);
        double s = atof(f["salary"].c_str());
        if (id <= 0 || n.empty() || d.empty() || dp.empty() || f["salary"].empty() || s < 0) {
            status = 400;
            return msg(false, "Enter a valid ID, name, designation, department and salary.");
        }
        Node* e = findById(id);
        if (path == "/api/add") {
            if (e) { status = 409; return msg(false, "Employee ID " + f["id"] + " already exists."); }
            addEmp(id, n, d, dp, s);
            save();
            return msg(true, "Employee added successfully.");
        }
        if (!e) { status = 404; return msg(false, "No employee found with that ID."); }
        e->name = n; e->desig = d; e->dept = dp; e->sal = s;
        save();
        return msg(true, "Employee updated successfully.");
    }

    status = 404;
    return msg(false, "Unknown request.");
}

// ---------- web server ----------
int main() {
#ifdef _WIN32
    WSADATA w;
    WSAStartup(MAKEWORD(2, 2), &w);
#endif
    load();
    SOCKET srv = socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));
    sockaddr_in a = {};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_ANY);
    a.sin_port = 0;                                  // 0 = let the system pick a free port
    if (bind(srv, (sockaddr*)&a, sizeof(a)) < 0) { cout << "Could not start server.\n"; return 1; }
    listen(srv, 10);
    socklen_t alen = sizeof(a);
    getsockname(srv, (sockaddr*)&a, &alen);          // find out which port we were given
    ostringstream url;
    url << "http://localhost:" << ntohs(a.sin_port);
    cout << "Server running. Opening the website in your browser..." << endl;
    cout << "(If it does not open, go to " << url.str() << ")\nPress Ctrl+C here to stop." << endl;
#ifdef _WIN32
    system(("start " + url.str()).c_str());          // opens the browser automatically
#elif defined(__APPLE__)
    system(("open " + url.str()).c_str());
#else
    system(("xdg-open " + url.str() + " >/dev/null 2>&1 &").c_str());
#endif

    while (true) {
        SOCKET cl = accept(srv, NULL, NULL);
        string req;
        char buf[4096];
        size_t end;
        while ((end = req.find("\r\n\r\n")) == string::npos) {   // read the headers
            int n = recv(cl, buf, sizeof(buf), 0);
            if (n <= 0) break;
            req.append(buf, n);
        }
        if (end == string::npos) { closesocket(cl); continue; }

        string hdr = req.substr(0, end), body = req.substr(end + 4);
        size_t p = lower(hdr).find("content-length:");
        size_t len = (p == string::npos) ? 0 : atoi(hdr.c_str() + p + 15);
        while (body.size() < len) {                              // read the rest of the body
            int n = recv(cl, buf, sizeof(buf), 0);
            if (n <= 0) break;
            body.append(buf, n);
        }

        istringstream first(hdr.substr(0, hdr.find("\r\n")));   // e.g. GET /api/all HTTP/1.1
        string method, path, type;
        first >> method >> path;
        int status;
        string out = handle(method, path, body, status, type);

        ostringstream r;
        r << "HTTP/1.1 " << status << (status == 200 ? " OK" : " Error")
          << "\r\nContent-Type: " << type << "; charset=utf-8\r\nContent-Length: " << out.size()
          << "\r\nConnection: close\r\n\r\n" << out;
        string resp = r.str();
        for (size_t s = 0; s < resp.size();) {
            int n = send(cl, resp.c_str() + s, (int)(resp.size() - s), 0);
            if (n <= 0) break;
            s += n;
        }
        closesocket(cl);
    }
}