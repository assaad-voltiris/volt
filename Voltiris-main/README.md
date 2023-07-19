# Voltiris

This package contains Voltiris Application, including a test Firmware for Arduino.

The different directories of this distributions are the following:

### Documents

MBUS official documents, Specification and Detailed Specifications of the protocol between the Application and the Electronics.

### Master

Source code of the Application (in C#). Currently tested on Mac OSX. Should run similarly on Linux or Windows.

### Slave

Source code of the protocol framework and the Arduino test implementation.

## Known limitations (2023-07-16)

- Firmware update not yet implemented
- Current physical layer is RS232
- Encryption not implemented
- No dynamic address shuffle implemented (needed to secure encryption)
