# Voltiris Framework Protocol and Arduino Test Implementation

The following C/C++ files are dedicated to the protocol framework (files with 'vlt' prefix) or
for the Arduino test implementation (files with 'ard' prefix and 'Voltiris.ino').

## Voltiris Protocol

The framework relies on a serial bus communication to interact with the Master. Multiple Electronics are connected to a single Master via a serial RS-485 link.
The interactions are based on a Master / Slave mechanism. Master queries a specific or all Slaves. The requested Slave responds to Master inquiry. In some cases (e.g. reset) or broadcasted commands, no respond is expected.
Serial packets are based on the MBUS standard (see the __~/Documents/__ directory at the root of this git package). Voltiris protocol only uses the "Read Input Registers" (Command 0x04) and "Preset Multiple Regs" (Command 0x10) for communication.

Detailed Specifications are also available as a PPTX presentation on the __~/Documents/__ directory.

## Implementation

Access to the framework functions and variables is through the voltiris namespace.

When implementing for a specific Architecture / Microcontroller, the following functions and typedefs should be implemented:

### Helpers

The 'vltHelpers.hpp' contains some definitions that must be implemented, like:
- The types for unsigned byte (__uint8__), unsigned short (__uint16__) and short (__int16__)
- Optionaly an __assert ()__ method that display an error on the board (used during the debug phase only)

Helpers also define the Memory address space, size of the various internal buffers, as well as a __Buffer__ template class used by the library.

### Serial Communication

The following serial functions need to be implemented:

```C++
// Serial port object that may be completed for a specific architecture
// Ex: struct MySerialPort: SerialPort { ... }
struct SerialPort {};

// Initialize serial port
SerialPort* serialInit ();

// Read next character
// Return the character or SERIAL_NO_CHARACTER_AVAILABLE (-1) if not available
// Similar to Serial.read() on Arduino
int serialRead (SerialPort* sp);

// Get the number of available characters to read
// Similar to Serial.available() on Arduino
int serialAvailable (SerialPort* sp);

// Write a buffer to the serial port.
// Return the number of bytes written
// Similar to Serial.write(buf, len) on Arduino
int serialWrite (SerialPort* sp, uint8* buffer, uint16 bufferSizeinBtes);
```

The Arduino implementation is located in 'ardSerial.hpp' and 'ardSerial.cpp'.

### Options

Options descriptors are defined via the structure __Option__ (in file 'vltOption.hpp').

The two __init()__ helpers members allow to easily construct an __uint16__ or respectively an __int16__ option descriptor.
You need to set a __getValue()__ function (respectivey __setValue ()__) if the option can be read (respectively written).
User data member __userData__ can alternatively be use to store the effective value of the option or your custom object. This is a reference object that will not be destroyed at termination.

Once created and initialized, option should be added with the __addOption ()__ function. For example, the full option descriptor for __Position_b1__ on Arduino (file 'ardFirmware.cpp') is:

```C++

static Option optPosition_b1;
static Option::Value valPositionsB1 [2];

// ...

optPosition_b1.init ("Position_b1", (uint16) 0, (uint16) 6000, (uint16) 10, Option::Dimension::DIM_2, Option::Unit::MILLIMETERS, false);
optPosition_b1.setValue = setPosition; // See code for complete implementation
optPosition_b1.getValue = getPosition; // See code for complete implementation
optPosition_b1.userData = (void*) valPositionsB1;
valPositionsB1[0].UINT_16 = optPosition_b1.min.UINT_16;
valPositionsB1[1].UINT_16 = optPosition_b1.min.UINT_16;
assert (addOption (optPosition_b1));

```

### Commands

The command processor (files 'vltCommands.hpp', 'vltCommands.cpp') is the brain of the framework. It processes commands from the Master via the Serial Port and call the necessary internal functions. It also automatically packages a response to the Master.

It uses two internal buffers:
- __bufferAscii__ to store incoming packet and create responses and,
- __bufferBin__, used by the code to get effective binary data from the packet (respectively set binary data for the response).

The command processor uses a very basic state machine to process incoming data, waiting for a ':' to indicate the start of the packet (state INIT) and waiting for a termination of \r\n to define the end of the packet (state END). The middle part is further processed by __processPacket ()__.

## Arduino implementation

### Installation

Install [Arduino IDE](https://www.arduino.cc/en/software), connect the Arduino card, open the Arduino project 'Voltiris.ino',  specify the correct Serial Port address (menu Tools->Port), compile (upper left check button) and upload to the board (upper left right arrow).

## Implementation

Arduino IDE define the ARDUINO macro. Code for Arduino in the Framework is protected within:  

```C++
#ifdef ARDUINO
    // ...
#endif
```

## Main program

Arduino main program is the minimalistic implementation of the Framework:

```C++
#include "vltSerial.hpp"
#include "vltFirmware.hpp"
#include "vltCommands.hpp"

using namespace voltiris;

SerialPort* sp = NULL;

void setup()
{
    initialize ();
    sp = serialInit ();

    // ...
}

void loop() {

while (1)
{
    switch (processIncomingSerialData (sp))
    {
    case SERIAL_BUFFER_OVERFLOW: // Junk data in buffer
    case 0: // No data available
        delay (10);
        break;
    default: // Some characters have been processed
        break;
    }
}
}
```

## Known limitations (2023-07-17)

- Firmware update not yet implemented
- Current physical layer is RS232
- Encryption not implemented
- No dynamic address shuffle implemented (needed to secure encryption)
- Option broadcast not implemented
