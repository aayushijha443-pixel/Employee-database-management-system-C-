let employees = JSON.parse(
    localStorage.getItem("employees")
) || [];

const form = document.getElementById("employeeForm");
const table = document.getElementById("employeeTable");
const searchInput = document.getElementById("searchInput");

function saveEmployees() {
    localStorage.setItem("employees", JSON.stringify(employees));
}

function displayEmployees(list = employees) {
    table.innerHTML = "";

    list.forEach(function(employee) {
        const row = document.createElement("tr");

        [
            employee.id,
            employee.name,
            employee.department,
            employee.designation,
            employee.salary
        ].forEach(function(value) {
            const cell = document.createElement("td");
            cell.textContent = value;
            row.appendChild(cell);
        });

        const actions = document.createElement("td");

        const editButton = document.createElement("button");
        editButton.textContent = "Edit";
        editButton.className = "action-button";
        editButton.addEventListener("click", function() {
            editEmployee(employee.id);
        });

        const deleteButton = document.createElement("button");
        deleteButton.textContent = "Delete";
        deleteButton.className = "action-button delete-button";
        deleteButton.addEventListener("click", function() {
            deleteEmployee(employee.id);
        });

        actions.appendChild(editButton);
        actions.appendChild(deleteButton);
        row.appendChild(actions);

        table.appendChild(row);
    });
}

form.addEventListener("submit", function(event) {
    event.preventDefault();

    const id = document.getElementById("employeeId").value.trim();

    const employee = {
        id: id,
        name: document.getElementById("employeeName").value.trim(),
        department: document.getElementById("department").value.trim(),
        designation: document.getElementById("designation").value.trim(),
        salary: document.getElementById("salary").value
    };

    if (employees.some(function(item) {
        return item.id === id;
    })) {
        alert("This Employee ID already exists!");
        return;
    }

    employees.push(employee);
    saveEmployees();
    displayEmployees();
    form.reset();

    alert("Employee added successfully!");
});

function editEmployee(id) {
    const employee = employees.find(function(item) {
        return item.id === id;
    });

    if (!employee) return;

    const name = prompt("Enter employee name:", employee.name);
    if (name === null) return;

    const department = prompt(
        "Enter department:", employee.department
    );
    if (department === null) return;

    const designation = prompt(
        "Enter designation:", employee.designation
    );
    if (designation === null) return;

    const salary = prompt("Enter salary:", employee.salary);
    if (salary === null) return;

    if (
        !name.trim() ||
        !department.trim() ||
        !designation.trim() ||
        salary.trim() === "" ||
        !Number.isFinite(Number(salary)) ||
        Number(salary) < 0
    ) {
        alert("Please enter valid employee details.");
        return;
    }

    employee.name = name.trim();
    employee.department = department.trim();
    employee.designation = designation.trim();
    employee.salary = salary;

    saveEmployees();
    displayEmployees();
}

function deleteEmployee(id) {
    const confirmed = confirm(
        "Are you sure you want to delete this employee?"
    );

    if (!confirmed) return;

    employees = employees.filter(function(employee) {
        return employee.id !== id;
    });

    saveEmployees();
    displayEmployees();
}

searchInput.addEventListener("input", function() {
    const searchId = searchInput.value.trim();

    const filteredEmployees = employees.filter(function(employee) {
        return employee.id.includes(searchId);
    });

    displayEmployees(filteredEmployees);
});

displayEmployees();