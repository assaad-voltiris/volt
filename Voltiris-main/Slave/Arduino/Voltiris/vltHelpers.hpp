#pragma once

namespace voltiris
{
    #ifdef ARDUINO

        #include <Arduino.h>

        typedef uint8_t  uint8;
        typedef uint16_t uint16;
        typedef int16_t  int16;
        
        void assert (bool condition);

    #else
    
        // Write custom assert if needed
        inline void assert (bool condition) {}

    #endif


    // --------------------
    // Serial configuration
    // --------------------
    
    // Messages from the serial interface
    const int SERIAL_NO_DATA = 0;
    const int SERIAL_NO_CHARACTER_AVAILABLE = -1;

    // Size of the buffer when receiving commands
    // Maximum buffer size is 0xff + header (':') + end ('\r\n');
    const int SERIAL_BUFFER_SIZE = 2 * 256 + 3;

    // Buffer overflow error when processIncomingSerialData ()
    const int SERIAL_BUFFER_OVERFLOW = -1;

    // ---------------------
    // Options configuration
    // ---------------------
    
    // Maximim number of options
    const int MAX_OPTIONS = 16; // To change !!!

    // Address where the option addresses are stored
    const uint16 OPTIONS_ADDRESS_START = 0x300;

    // Computed dynamically
    extern uint16 OPTIONS_ADDRESS_END;
    

    // --------------------
    // Memory configuration
    // --------------------

    // Version of memory register
    const uint8 MEMORY_VERSION = 0;

    // Address where the memory version is stored
    const uint16 MEMORY_VERSION_ADDRESS = 0x100;

    // Address of the different memory access
    const uint16 CMD_HARD_RESET         = 0x00;
    const uint16 CMD_RESET_SLAVE_ID     = 0x01;
    const uint16 CMD_SLAVE_INDENT       = 0x02;
    const uint16 CMD_GET_OPT_INFO_START = 0x20;
    const uint16 CMD_GET_OPT_INFO_END   = 0x50;
    
    // Address where the memory buffer is stored
    const uint16 BUFFER_ADDRESS_START   = 0x200;
    const uint16 BUFFER_ADDRESS_END     = 0x2fe;
    
    // Size of the memory buffer
    const uint16 BUFFER_SIZE = BUFFER_ADDRESS_END - BUFFER_ADDRESS_START;
    

    // -------------------
    // Other configuration
    // -------------------

    // Maximum ID of slave
    const uint8 MAX_SLAVE_ID = 33;

    // --------------------
    // Helper class
    // --------------------


    // Buffer class
    template<uint16 _capacity> struct Buffer
    {
        uint8 data [_capacity];
        uint16 size = 0;

        Buffer () = default;
        Buffer (const Buffer& other) = default;
        ~Buffer () = default;

        void reset () { size = 0; }
        uint16 capacity () const { return _capacity; }

        bool add (const uint8 val)
        {
            if (size >= _capacity)
                return false;
            data[size++] = val;
            return true;
        }
    };

    // R/W buffer used for memory operations (0x200)
    extern Buffer<BUFFER_SIZE> buffer;
}
