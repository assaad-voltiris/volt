#pragma once

#include "vltHelpers.hpp"


namespace voltiris
{  
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
};
