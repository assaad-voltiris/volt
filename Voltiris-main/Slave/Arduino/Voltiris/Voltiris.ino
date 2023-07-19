#ifdef ARDUINO

  #include "vltSerial.hpp"
  #include "vltFirmware.hpp"
  #include "vltCommands.hpp"

  voltiris::SerialPort* sp = NULL;

  void setup()
  {
    voltiris::initialize ();
    sp = voltiris::serialInit ();

    // initialize digital pin LED_BUILTIN as an output.
    // Used by the assert () function to blink LED.
    pinMode (LED_BUILTIN, OUTPUT);
  }

  void loop() {

    while (1)
    {
      switch (voltiris::processIncomingSerialData (sp))
      {
        case voltiris::SERIAL_BUFFER_OVERFLOW: // Junk data in buffer
        case 0: // No data available
          delay (10);
          break;
        default: // Some characters have been processed
          break;
      }
    }
  }

#endif
