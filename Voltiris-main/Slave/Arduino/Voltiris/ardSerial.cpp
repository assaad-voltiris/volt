#ifdef ARDUINO

    #include "ardSerial.hpp"

    // Arduino documentation on serial port:
    // https://www.arduino.cc/reference/en/language/functions/communication/serial/

    namespace voltiris
    {
        static ArduinoSerialPort serial;

        SerialPort* serialInit ()
        {
            ::Serial.begin(115200);
            while (!::Serial)
                delay (10); // wait for serial port to connect. Needed for native USB

            return (SerialPort*) &serial; 
        }

        int serialRead (SerialPort* sp)
        {
            assert (sp != NULL);
            return ::Serial.read();
        }

        int serialAvailable (SerialPort* sp)
        {
            assert (sp != NULL);
            return ::Serial.available();
        }

        int serialWrite (SerialPort* sp, uint8* buffer, uint16 bufferSizeinBtes)
        {
            assert (sp != NULL);
            return (int) ::Serial.write(buffer, (size_t) bufferSizeinBtes);
        }
    }

#endif
