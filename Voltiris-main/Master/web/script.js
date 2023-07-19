// ---------------------------
// Menus hide / show callbacks
// ---------------------------

function show (e, name)
{
    // Prevent the default action (navigation)
    e.preventDefault();

    // Hide other menu contents
    document.getElementById('settings').style.display = 'none';
    document.getElementById('unitTests').style.display = 'none';
    document.getElementById('debug').style.display = 'none';
    document.getElementById('logs').style.display = 'none';

    // Display the specific form
    document.getElementById(name).style.display = 'block';
}

// Add an event listeners to the menu links
document.getElementById('settingsLink').addEventListener('click', function(e) {
    show (e, 'settings');
});
document.getElementById('unitTestsLink').addEventListener('click', function(e) {
    show (e, 'unitTests');
});
document.getElementById('debugLink').addEventListener('click', function(e) {
    show (e, 'debug');
});
document.getElementById('logLink').addEventListener('click', function(e) {
    retrieveAndDisplayLogs ();
    show (e, 'logs');
});

// -----------
// Log section
// -----------

let logData = [];

function sortAndDisplayLogs() {
    // Get the selected sorting method
    let sortingMethod = document.getElementById('logSort').value;

    // Sort the logs
    if (sortingMethod === 'dateAscending') {
        logData.sort((a, b) => a.date - b.date);
    } else { // default is 'dateDescending'
        logData.sort((a, b) => b.date - a.date);
    }

    // Clear the log display
    let logTableBody = document.getElementById('logTable').querySelector('tbody');
    while (logTableBody.firstChild) {
        logTableBody.firstChild.remove();
    }

    // Display the logs
    logData.forEach(logItem => {
        let tableRow = document.createElement('tr');
        let date = new Date (logItem.date);
        let dateFormatted = date.toISOString().replace ("T", " ").replace ("Z", "");
        tableRow.innerHTML = `<td>${dateFormatted}</td><td class="${logItem.gravity}">${logItem.gravity}</td><td>${logItem.text}</td>`;
        logTableBody.append(tableRow);
    });
}

// Update the log display whenever the sorting method changes
document.getElementById('logSort').addEventListener('change', sortAndDisplayLogs);

// -------------
// Debug section
// -------------

document.getElementById('debugSubmit').addEventListener('click', function(evt, ui) {
    
    const command = document.getElementById('debugCommand').value.trim();
    let result;
    
    evt.preventDefault();

    // You can add your actual command handling code here.
    // This is a simple example that echoes the command.
    result = `> ${command}`;

    // Append the command and its result to the debugConsole div
    document.getElementById('debugConsole').innerHTML +=
    `<div style="font-weight: bold; color: black;">${result}</div>\n`;

    // Clear the input field
    document.getElementById('debugCommand').value = '';

    fetch(`${serverPrefix}${command}`)
        .then((response) =>
        {
            if (!response.ok)
                document.getElementById('debugConsole').innerHTML +=
                    `<div style="color:red;">Error: ${response.status}</div>\n`;
            else
                response.text().then ((text) => {
                    document.getElementById('debugConsole').innerHTML +=
                        `<div style="color:blue;">${text}</div>\n`;
                });
        });
});

// Clear the debug console
document.getElementById('clearDebugSubmit').addEventListener('click', function(evt, ui) {
    evt.preventDefault();
    document.getElementById('debugConsole').innerHTML = "";
});

// ------------------
// Unit Tests section
// ------------------

// Populate test list
let testList = document.getElementById('testList');
tests.forEach(test => {
    let listItem = document.createElement('li');
    listItem.textContent = test.name;
    listItem.addEventListener('click', function() {
        document.body.style.cursor = 'wait'; // Change cursor
        test.run ();
        //document.getElementById('unitTestsConsole').textContent += '\n' + test.run();
        document.body.style.cursor = 'default'; // Change cursor back
    });
    testList.append(listItem);
});

// Run all tests button
document.getElementById('runAllTests').addEventListener('click', function(evt, ui) {
    evt.preventDefault();
    document.body.style.cursor = 'wait'; // Change cursor

    var promise = Promise.resolve();
    tests.forEach(test => {
      promise = promise.then (() => test.run ());
    });
    promise.then (() => document.body.style.cursor = 'default');
});

// Clear the unit tests console
document.getElementById('clearUnitTestsSubmit').addEventListener('click', function(evt, ui) {
    evt.preventDefault();
    document.getElementById('unitTestsConsole').innerHTML = "";
});
