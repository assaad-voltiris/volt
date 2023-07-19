// --------------------------------
// Unit tests and framework classes
// --------------------------------

const serverPrefix = `http://localhost:8080/cmd/`;

const UnitTestStatus = {
    Start: `style="font-weight: bold; color: black;"`,
    Info: ``,
    Success: `div style="color: #5DCA88;"`,
    Failure: `div style="color: red;"`,
};

class UnitTest {
    constructor(name, testFunc) {
        this.name = name;
        this.testFunc = testFunc;
        this.console = document.getElementById('unitTestsConsole');
    }

    fetchJson (url) {
        return fetch(`${serverPrefix}${url}`)
            .then((response) => {
                if (!response.ok)
                    throw new Error (response.status);
                return response.json(); })
            .then((json) => {
                if (json.status != "Succeed")
                    throw new Error (`Function call failure '${url}': '${json.status}'`);
                return json;})
            .catch(error => {
                throw error;
            });
    }
   
    log (message, status) {
        this.console.innerHTML +=`<div ${status}>${message}</div>\n`;
    }

    async run() {

        this.log (`Running test '${this.name}'`, UnitTestStatus.Start);

        try {
            await this.testFunc();  
        }
        catch (error) {
            if (error.message != "SUCCESS")
            {
                this.log (`Failure: ${error}`, UnitTestStatus.Failure);
                return false;
            }
        }
        this.log (`Success`, UnitTestStatus.Success);
        return true;
    }
}

let tests = [];

// -----------------------------------------------------------------
// Retrieve connected devices (required for later tests)
// -----------------------------------------------------------------

let connectedDevice = 1;

tests.push (new UnitTest('Retrieve connected devices', async function() {

    const url = `detectSlaves`;
    
    this.log (`Executing command '${url}'`, UnitTestStatus.Info);
    return this.fetchJson (url)
        .then ((json) => {
            const res = json.values;
            if (json.values.length == 0)
                throw new Error (`No device connected`);
            json.values.forEach (val => this.log (`Device '${val}' connected`, UnitTestStatus.Info));    
            connectedDevice = json.values[0];
            this.log (`Taking device '${connectedDevice}' as reference for later tests`, UnitTestStatus.Info);
            return;
        })
        .catch(error => {
            throw error;
        });
}));

// -----------------------------------------------------------------
// Read Serial Number of current device
// -----------------------------------------------------------------

const toHexString = (bytes) => {
    return Array.from(bytes, (byte) => {
      return ('0' + (byte & 0xff).toString(16)).slice(-2);
    }).join('');
  };

tests.push (new UnitTest(`Read Serial Number of current device`, async function() {

    const url = `serialNumber?id=${connectedDevice}`;
    const expectedSN = `deadbeefc0febabe`;

    this.log (`Executing command '${url}', expecting sn = '${expectedSN}'`, UnitTestStatus.Info);
    return this.fetchJson (url)
        .then ((json) => {
            const sn = toHexString (json.values);
            this.log (`Device serial number is '${sn}'`, UnitTestStatus.Info);
            if (sn != expectedSN)
                throw new Error (`Expected serial number '${expectedSN}', got '${sn}'`);
            return;
        })
        .catch(error => {
            throw error;
        });
}));

// -----------------------------------------------------------------
// Read Options of current device
// -----------------------------------------------------------------

async function getOptionInformation (test, index)
{
    const url = `getOptionInfo?id=${connectedDevice}&optIndex=${index}`;
    test.log (`Getting option ${index}:`, UnitTestStatus.Info);

    return test.fetchJson (url)
                .then ((json) => {
                    const optString = String.fromCharCode.apply (null, json.values);
                    test.log (`Retrieved: '${optString}'`, UnitTestStatus.Info);
                    const objJson = JSON.parse (optString);
                    test.log (`Option name: '${objJson.name}'`, UnitTestStatus.Info);
                    test.objJson = objJson;
                })
                .catch(error => {
                    test.log (`Error ${error}:`, UnitTestStatus.Error);
                    const errorWithIndex = Object.assign(error , { __index: index})
                    throw errorWithIndex;
                }); 
}

const expectedOptions = 8;

tests.push (new UnitTest(`Read the ${expectedOptions} Options of current device`, async function() {

    let i = 0;
    
    var promise = Promise.resolve();

    for (let i = 0; i < 100; i++)  
        promise = promise.then (() => getOptionInformation (this, i))
                        .catch(error => {
                            if (error.__index == undefined)
                                throw error;
                            if (error.__index == expectedOptions)
                                return Promise.reject(new Error ("SUCCESS"));
                            else
                                return Promise.reject(new Error (`Expecting ${expectedOptions} options, got ${error.__index}`));
                        });
                            
    return promise;
}));

tests.push (new UnitTest(`Read / write in memory`, async function() {

    let data = new Uint8Array (128);
    for (let i = 0; i < data.length; i++)
        data [i] = i;
    const dataString = toHexString (data);

    this.log (`Write ${data.length} bytes: '${dataString}':`, UnitTestStatus.Info);

    const urlWrite = `writeMemory?id=${connectedDevice}&address=0&data=${dataString}`;
    const urlRead  = `readMemory?id=${connectedDevice}&address=0&size=${data.length}`;

    return this.fetchJson (urlWrite)
        .then ((jsonWrite) => {
            this.log (`Read ${data.length} bytes`, UnitTestStatus.Info);
        })
        .then (() => this.fetchJson (urlRead))
        .then ((jsonRead) => {
            if (data.toString() == jsonRead.values.toString())
            {
                this.log (`Read values match written ones`, UnitTestStatus.Info);
                return;
            }
            throw new Error ("Written and read arrays do not match");
        })
        .catch(error => {
            throw error;
        });
}));

tests.push (new UnitTest(`Read / write option registers`, async function() {
    
    return getOptionInformation (this, 0)
        .then(() => {
            this.log (`Read option at address ${this.objJson.address}`, UnitTestStatus.Info);
            const urlGet = `getRegister?id=${connectedDevice}&address=${this.objJson.address}`;
            this.log (urlGet, UnitTestStatus.Info);
            return this.fetchJson (urlGet);
        })
        .then((getJson) => {
            this.log (` => ${getJson.values}`, UnitTestStatus.Info);
            const urlSet = `setRegister?id=${connectedDevice}&address=${this.objJson.address}&value=${this.objJson.max}`;
            this.log (urlSet, UnitTestStatus.Info);
            this.initialValue = getJson.values;
            return this.fetchJson (urlSet);
        })
        .then((setJson) => {
            const urlGet = `getRegister?id=${connectedDevice}&address=${this.objJson.address}`;
            this.log (urlGet, UnitTestStatus.Info);
            return this.fetchJson (urlGet);
        })
        .then((getJson)=> {
            this.log (` => ${getJson.values}`, UnitTestStatus.Info);
            if (getJson.values != this.objJson.max)
                throw new Error (`Was expecting to get ${this.objJson.max} but found ${getJson.values} instead`);
            this.log (`Option correctly set`, UnitTestStatus.Info);
            const urlSet = `setRegister?id=${connectedDevice}&address=${this.objJson.address}&value=${this.initialValue}`;
            this.log (urlSet, UnitTestStatus.Info);
            return this.fetchJson (urlSet);
        });
}));
