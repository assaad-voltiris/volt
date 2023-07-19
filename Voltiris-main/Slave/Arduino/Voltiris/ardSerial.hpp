#pragma once

#ifdef ARDUINO

    #include <Arduino.h>

    #include "vltSerial.hpp"

    namespace voltiris
    {
        struct ArduinoSerialPort: SerialPort {};
    }

#endif