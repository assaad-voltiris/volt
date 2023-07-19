
// ------------------------------------------------------------
// Retrieve the current log level, display dropdown and update it
// ------------------------------------------------------------

function setupLogLevelDropDown()
{
  let logLevelDropDown = document.getElementById('logLevelDropDown');

  // Remove any existing options
  while (logLevelDropDown.firstChild)
    logLevelDropDown.firstChild.remove();

  fetch("http://localhost:8080/settings/getLogLevel")
  .then((response) => response.text())
  .then((currentLogLevel) =>
  {
    fetch("http://localhost:8080/settings/logLevels")
    .then((response) => response.json())
    .then((json) => 
    {
      // Create a new <option> for each option in the JSON array
      json.forEach(function(optionText) 
      {
        let newOption = document.createElement('option');
        newOption.text = optionText;
        newOption.value = optionText;
        newOption.selected = (optionText == currentLogLevel);
        logLevelDropDown.add(newOption);
      })
    });
  });

  logLevelDropDown.addEventListener('change', function(e) 
  {
    let selectedOption = e.target.value;
    fetch(`http://localhost:8080/settings/setLogLevel?level=${selectedOption}`)
    .then((response) => response.text())
    .then((currentLogLevel) => {
      if (currentLogLevel != selectedOption)
        console.error (`Cannot change log level to: ${selectedOption}`);
      });
  });
}

// ------------------------------------------------------------
// Retrieve the current log level, display dropdown and update it
// ------------------------------------------------------------

function setupSerialPortName()
{
  let serialPortNameInput = document.getElementById('serialPortName');

  fetch("http://localhost:8080/settings/getSerialPortName")
  .then((response) => response.text())
  .then((name) =>
  {
    serialPortNameInput.value = name;
  });

  serialPortNameInput.addEventListener('change', function(e) 
  {
    let name = encodeURIComponent (e.target.value);
    fetch(`http://localhost:8080/settings/setSerialPortName?name=${name}`)
    .then((response) => response.text());
  });
}

// ------------------------------------------------------------
// Retrieve logs and display them
// ------------------------------------------------------------

function retrieveAndDisplayLogs()
{
  fetch("http://localhost:8080/settings/getLogs")
    .then((response) => response.json())
    .then((json) => 
    {
      logData = [];

      json.forEach(function(logItemJson) 
      {
        let logItem = {
          date: Date.parse (logItemJson.date),
          gravity: logItemJson.logLevel,
          text: logItemJson.description
        };
        logData.push(logItem);
      });
      sortAndDisplayLogs ();
    });
}

