#pragma once

#include "vltSerial.hpp"
#include "vltOption.hpp"

namespace voltiris
{
    // Process incoming data coming from the serial port
    // Should be placed in a loop and called regularily
    // Function return after processing incoming data or
    // if no serial data is available.
    // Return value is the number of processed characters or
    // SERIAL_BUFFER_OVERFLOW (-1)
    int processIncomingSerialData (SerialPort* sp);
}
