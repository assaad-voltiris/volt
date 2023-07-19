# Voltiris Master Application

Source code of the Application (in C#). Currently tested on Mac OSX. Should run similarly on Linux or Windows.

# IDE Installation

Install [Microsoft Visual Code](https://code.visualstudio.com/download). Go in Menu -> Open Directory and select the directory with this file. Install all the recommended plug-in, as well as the "Microsoft Platform" (that is also proposed by Visual Code).

You can compile and run the application using the F5 key. Once started, open your browser and type:

```
http://localhost:8080/api/
```

You  should see the GUI of the application.

# Master Application

The application is using the Webserver framework GenHTTP. It allows to receive Web requests on the 8080 port.

The http://localhost:8080/api/ web page presents a high-level web interface to communicate with the Electronics / Slaves.

- In the __Settings__ menu, you can select the Log Level, as well as the Serial Port that is connected to the Electronics.
- In the __Unit Tests__ menu, run automatic tests with the Slaves. All Unit Tests are written in Javascript.
- The __Debug__ menu enables to perform low-level operations with Slaves (see [Debug Commands](#debug-commands)).
- The __Logs__ menu displays the recorded Logs. Logs granularity can be set in Settings menu.

## Debug Commands

The following commands can be used from any web browser using 

```
http://localhost:8080/cmd/<command>?<arg0>=1&<arg1>=256
```

Or with the following syntax in the __Debug__ interface:

```
<command>?<arg0>=1&<arg1>=256
```

Most commands return a JSON answer composed of a status and values.

The different status and their meaning:
- __Succeed__: success of the command
- __Error__: failure of the command
- __Unknown__: Slave provides an answer that cannot be interpreted by Master
- __ComTimeout__: A serial communication timeout occurs
- __ComError__: A serial error occurs (e.g. port disconnection)
- __ArgError__: Provided argument(s) are wrong (typically out of bounds)

If command is malformed, the webserver can generate an Error 404.

The __values__ array in the JSON answer are bytes.

An __id__ value of 0 means that the command targets all the connected Slaves.

If not specified, all the numbers in the arguments are specified in decimal!

### Hard Reset

Perform a hard reset (restart the firmware of the Electronics) on a specified or on all Slaves.

```
hardReset?id=0
```

Will perform a hard reset on all slaves.
This command is not expecting a response from the slaves and will only return the status of the Master write operation.

```
Succeed
```
### Reset Specified Slaves

This command is used during the pairing operation. It asks specified slaves (having a bit 0 in the bit mask) to take randomly another id that is specifies with a bit = 0.
__ids__ is a 32 bits bit mask (in big endian)

```
resetSlaves?ids=00000000000000000000000000000011
```

Slave(s) with an id different of 1 or 2 have to change their id and take an id comprised between 3 and 33.
This command is not expecting a response from the slaves and will only return the status of the Master write operation.

```
Succeed
```

### Serial Numner 

Retrieve the 64 bits __Serial Number__ of a specific __Slave__.


```
serialNumber?id=1
```

Response is in big endian.

```
{"status":"Succeed","values":[222,173,190,239,192,254,186,190]}
```

### Get Register

Retrieve a 16bits register at a specific __address__ from a __Slave__.

```
getRegister?id=1&address=256
```

Request slave (id=1) to retrieve the current version of the memory mapping (currently 0).

```json
{"status":"Succeed","values":[0]}
```

### Set Register

Set a __Slave__ 16bits register at a specific __address__.
An uint16 or int16 __value__ can be used depending on the type of the register.

```
setRegister?id=1&address=800&value=1
```

Request slave (id=1) to set the 16bit register at address 800 with value = 1
Only the status of the JSON answer will indicate if the command is successfull.

```json
{"status":"Succeed","values":[]}
```

### Read Memory

Read a chunk of memory at __address__ from a __Slave__.
__size__ is specified in bytes.
__address__ 0 is at the start of the memory bank (effective absolute address 0x200).

```
readMemory?id=1&address=0&size=16
```

Request slave (id=1) to get the 16 bytes starting at address 0 in the memory bank (effective absolute address 0x200). 

```json
{"status":"Succeed","values":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]}
```

### Write Memory 

Write a chunk of memory to a __Slave__ at a specific __address__.
__data__ is represented by a string of hexadecimal bytes.
__address__ 0 is at the start of the memory bank (effective absolute address 0x200).

```
writeMemory?id=1&address=0&data=0001020304ff
```

Request slave (id=1) to write the following bytes at address 0 in the memory bank (effective absolute address 0x200): [0x00, 0x01, 0x02, 0x03, 0x04, 0xff].

Only the status of the JSON answer will indicate if the command is successful.

```json
{"status":"Succeed","values":[]}
```

### Get Option Descriptor

Retrieve the full JSON descriptor of a specific option.

```
getOptionInfo?id=1&optIndex=0
```

Increment __optIndex__ to list all options until you retrieve a status error.

```json
{"status":"Succeed","values":[123,34,110,97,109,101,34,58,34,80,111,115,105,116,105,111,110,95,98,49,34,44,34,116,121,112,101,34,58,34,117,105,110,116,49,54,34,44,34,109,105,110,34,58,48,44,34,109,97,120,34,58,54,48,48,48,44,34,115,99,97,108,101,34,58,49,48,44,34,97,99,99,101,115,115,34,58,34,82,87,34,44,34,100,105,109,34,58,50,44,34,117,110,105,116,34,58,34,109,109,34,44,34,97,100,100,114,101,115,115,34,58,55,54,56,44,34,98,114,111,97,100,99,97,115,101,34,58,34,102,97,108,115,101,34,125]}
```

Note that the values are representing a string in ASCII format. Previous response corresponds to the following ASCII string:

```json
{"name":"Position_b1","type":"uint16","min":0,"max":6000,"scale":10,"access":"RW","dim":2,"unit":"mm","address":768,"broadcase":"false"}
```

For example, to access 'Position_b1' option, addresses 768 and 770 should be used. 
[Get Register](#get-register) and [Set Register](#set-register)
 can be useed to get/set a value to this specific option.


## Known limitations (2023-07-17)

- Updating the serial port requires an application restart
- Current physical layer is RS232
- Encryption not implemented
- Option broadcast not implemented
