#include "vltHelpers.hpp"
#include "vltCommands.hpp"
#include "vltFirmware.hpp"
#include "vltOption.hpp"


namespace voltiris
{
    // Buffer containing the incoming and outcoming ASCII packets
    static Buffer<SERIAL_BUFFER_SIZE> bufferAscii;

    // Binary buffer
    static Buffer<SERIAL_BUFFER_SIZE/2> bufferBin;
    
    // Minimum command size: ":\r\n"
    const uint16 MIN_CMD_SIZE = 3;

    // Size of a read input register command
    const uint8 READ_INPUT_REGISTERS_CMD_SIZE = 7;

    // Code of the read input registers command
    const uint8 READ_INPUT_REGISTERS_CMD = 0x04;

    // Code of the preset multiple registers command
    const uint8 PRESET_MULT_REGISTERS_CMD = 0x10;


    // Convert and ASCII character (eg: 'F') to binary (eg 0xf)
    static inline uint8 toByte (const char c, bool& error)
    {
        if (c >= '0' && c <= '9')
            return (uint8) (c - '0');
        if (c >= 'a' && c <= 'f')
            return (uint8) (c - 'a' + 10);
        if (c >= 'A' && c <= 'F')
            return (uint8) (c - 'A' + 10);
        error = true;
        return 0;
    }

    // Convert ASCII buffer to binary
    // Return the status of conversion
    static inline bool convertAsciiToBinary ()
    {
        bufferBin.reset ();
        if (bufferAscii.size % 2 != 0)
            return false;
        bool error = false;
        for (uint16 i = 0; i < bufferAscii.size;)
        {
            uint8 hi  = toByte(bufferAscii.data[i++], error);
            uint8 low = toByte(bufferAscii.data[i++], error);
            bufferBin.add ((hi << 4) | low);
        }
        return !error;
    }

    // Convert a value from 0 to 0xf to a character
    static inline char toChar (uint8 in)
    {
        if (in <= 9)
            return in + '0';
        in -= 10;
        return in + 'A';
    }

    // Convert a byte to 2 characters representing the number in hex
    static inline void toChar (uint8 in, char& hi, char& low)
    {
        hi  = toChar (in >> 4);
        low = toChar (in & 0xf);
    }

    // Convert binary buffer to ASCII
    static inline void convertBinaryToAscii ()
    {
        bufferAscii.reset ();
        bufferAscii.add (':');

        for (uint16 i = 0; i < bufferBin.size; i++)
        {
            char hi, low;
            toChar (bufferBin.data [i], hi, low);
            bufferAscii.add (hi);
            bufferAscii.add (low);
        }
        bufferAscii.add ('\r');
        bufferAscii.add ('\n');
    }

    // Get a byte in the binary buffer
    // and increment index accordingly
    static inline uint8 get8 (uint16& index)
    {
        return bufferBin.data [index++];
    }

    // Get a uint16 in the binary buffer
    // and increment index accordingly
    static inline uint16 get16 (uint16& index)
    {
        uint16 hi  = (uint16) get8 (index);
        uint16 low = (uint16) get8 (index);
        return (hi << 8) | low;
    }

    // Get raw data in the binary buffer
    // and increment index accordingly
    static inline uint8* getX (uint16& index, const uint16 size)
    {
        uint8* ret = &bufferBin.data [index];
        index += size;
        return ret;
    }

    // Set a byte in the binary buffer
    static inline void add8 (const uint8 value)
    {
        bufferBin.add (value);
    }

    // Set a uint16 in the binary buffer
    static inline void add16 (const uint16 value)
    {
        add8 ((uint8) (value >> 8));
        add8 ((uint8) (value & 0xff));
    }

    static inline void addX (uint8* address, const uint16 size)
    {
        for (uint16 i = 0; i < size; i++)
            add8 (*address++);
    }

    // Compute the CRC8 of data in the bufferBin
    // until crcIndex position
    static inline uint8 computeCRC8 (const uint16 crcIndex)
    {
        uint8 crc8 = 0;
        for (uint16 i = 0; i < crcIndex; i++)
            crc8 += bufferBin.data [i];
        return crc8;
    }

    static inline void readError (SerialPort& sp)
    {
        // Send error
        bufferBin.reset ();
        add8 (configuration.slaveId);
        add8 (READ_INPUT_REGISTERS_CMD);
        add8 (0);
        add8 (computeCRC8(bufferBin.size));
        convertBinaryToAscii ();
        serialWrite (&sp, bufferAscii.data, bufferAscii.size);
        bufferAscii.reset ();
    }


    static inline void sendReadInputRegisterPacket (SerialPort& sp, const uint16 value)
    {
        bufferBin.reset ();
        add8 (configuration.slaveId);
        add8 (READ_INPUT_REGISTERS_CMD);
        add8 (sizeof(uint16));
        add16 (value);
        add8 (computeCRC8(bufferBin.size));
        convertBinaryToAscii ();
        serialWrite (&sp, bufferAscii.data, bufferAscii.size);
        bufferAscii.reset ();
    }

    static inline void slaveIdentification (SerialPort& sp)
    {
        bufferBin.reset ();
        add8 (configuration.slaveId);
        add8 (READ_INPUT_REGISTERS_CMD);
        add8 (8);
        uint8* data = getSerialNumber ();
        addX (data, 8);
        add8 (computeCRC8(bufferBin.size));
        convertBinaryToAscii ();
        serialWrite (&sp, bufferAscii.data, bufferAscii.size);
        bufferAscii.reset ();
    }

    static inline void getOptionInfo (SerialPort& sp, const uint16 address, const Option& opt)
    {
        uint16 bufferSize = buffer.capacity ();
        if (!opt.convertToJson (buffer.data, bufferSize))
        {
            readError (sp);
            return;
        }
        buffer.size = bufferSize;
        
        bufferBin.reset ();
        add8 (configuration.slaveId);
        add8 (READ_INPUT_REGISTERS_CMD);
        add8 (2);
        add16 (bufferSize);
        add8 (computeCRC8(bufferBin.size));
        convertBinaryToAscii ();
        serialWrite (&sp, bufferAscii.data, bufferAscii.size);
        bufferAscii.reset ();
    }

    static inline void readMemory (SerialPort& sp, uint16 index, uint16 size)
    {
        bufferBin.reset ();
        add8 (configuration.slaveId);
        add8 (READ_INPUT_REGISTERS_CMD);
        add8 (size);
        addX (buffer.data + index, size);
        add8 (computeCRC8(bufferBin.size));
        convertBinaryToAscii ();
        serialWrite (&sp, bufferAscii.data, bufferAscii.size);
        bufferAscii.reset ();
    }

    // Process a Read Input Register Packet
    static inline void processReadInputRegisterPacket (SerialPort& sp)
    {
        uint16 index = 0;

        // Packet as defined in the Modbus protocol
        uint8  slaveAddress   = get8(index);
        uint8  cmd            = get8(index);
        uint16 address        = get16(index);
        uint16 numberOfUint16 = get16(index);
        uint8  crc8           = get8(index);

        // Check that packet has the correct size
        if (bufferBin.size < READ_INPUT_REGISTERS_CMD_SIZE)
            return;

        // Verify the packet CRC
        if (crc8 != computeCRC8 (index-1))
            return;

        // Verify that the packet is addressed to this slave
        if (slaveAddress != configuration.slaveId)
            return;

        // Check if address is a getOptionInfo () cmd
        if (address >= CMD_GET_OPT_INFO_START && // 0x20
            address <= CMD_GET_OPT_INFO_END) // 0x50
        {
            Option* opt = getOption (address - CMD_GET_OPT_INFO_START);
            if (numberOfUint16 != 1 || opt == NULL)
                readError (sp);
            else
                getOptionInfo (sp, address, *opt);
            return;
        }

        // Check if address is in the R/W memory
        if (address >= BUFFER_ADDRESS_START && // 0x200
            address <= BUFFER_ADDRESS_END) // 0x2fe
        {
            uint16 index = address - BUFFER_ADDRESS_START;
            uint16 size = 2 * numberOfUint16;

            if (index + size > BUFFER_SIZE)
                readError (sp);
            else
                readMemory (sp, index, size);
            return;
        }

        // Check if address is in the option memory
        if (address >= OPTIONS_ADDRESS_START && // 0x300
            address <= OPTIONS_ADDRESS_END) // dynamic
        {
            uint16 value;
            uint16 size = 2 * numberOfUint16;

            if (numberOfUint16 != 1 || !getOptionValueAtAddress (address, value))
                readError (sp);
            else
                sendReadInputRegisterPacket (sp, value);
            return;
        }

        switch (address)
        {
            case CMD_SLAVE_INDENT: // 0x02

                if (numberOfUint16 != 4)
                    readError (sp);
                else
                    slaveIdentification (sp);
                return;

            case MEMORY_VERSION_ADDRESS: // 0x100
                
                if (numberOfUint16 != 1)
                    readError (sp);
                else   
                    sendReadInputRegisterPacket (sp, MEMORY_VERSION);
                return;
        }

        // GMA !!!
        char data [128];
        sprintf (data, "cmd: %d %d %d\n", (int) index, (int) crc8, (int) computeCRC8 (index-1)); //":deadbeaf\r\n";
        serialWrite (&sp, data, strlen(data));
    }

    static inline void checkSlaveId (const uint8* mask32bits)
    {
        uint8 zeroCount = 0;
        bool shouldReset = false;

        uint8 currentId = 1;
        for (int i = 3; i >= 0; i--)
        {
            uint8 bit = 1;
            uint8 mask8bits = mask32bits[i];
            for (int i = 0; i < 8; i++)
            {
                if ((bit & mask8bits) == 0)
                    zeroCount++;
                else
                    if (currentId == configuration.slaveId)
                        shouldReset = true;
                currentId++;
                bit <<= 1;
            }    
        }

        if (shouldReset)
        {
            uint8 newIdIndex = randomByte () % zeroCount;

            zeroCount = 0;
            uint8 currentId = 1;

            for (int i = 3; i >= 0; i--)
            {
                uint8 bit = 1;
                uint8 mask8bits = mask32bits[i];
                for (int i = 0; i < 8; i++)
                {
                    if ((bit & mask8bits) == 0)
                        zeroCount++;

                    if (zeroCount == newIdIndex)
                    {
                        configuration.slaveId = currentId;
                        return;
                    }
        
                    currentId++;
                    bit <<= 1;
                }    
            }
        }
    }

    static inline void writeResponse (SerialPort& sp, uint16 address, uint16 nbRegisters = 0)
    {
        // Send error
        bufferBin.reset ();
        add8 (configuration.slaveId);
        add8 (PRESET_MULT_REGISTERS_CMD);
        add16 (address);
        add16 (nbRegisters);
        add8 (computeCRC8(bufferBin.size));
        convertBinaryToAscii ();
        serialWrite (&sp, bufferAscii.data, bufferAscii.size);
        bufferAscii.reset ();
    }


    static inline void processPresetMultipleRegistersPacket (SerialPort& sp)
    {
        uint16 index = 0;

        // Packet as defined in the Modbus protocol
        uint8  slaveAddress   = get8  (index);
        uint8  cmd            = get8  (index);
        uint16 address        = get16 (index);
        uint16 numberOfUint16 = get16 (index);
        uint16 byteCount      = (uint16) get8 (index);
        uint8* data           = getX  (index, (uint16) byteCount);
        uint8  crc8           = get8  (index);

        // Check that packet has the correct size
        if (bufferAscii.size != 2 * (8 + byteCount))
            return;

        // Verify the packet CRC
        if (crc8 != computeCRC8 (index-1))
            return;

        // Check validity of byteCount
        if (byteCount > numberOfUint16 * 2)
        {
            writeResponse (sp, address);
            return;
        }
            
        // Check if address is in the R/W memory
        if (address >= BUFFER_ADDRESS_START && // 0x200
            address <= BUFFER_ADDRESS_END) // 0x2fe
        {
            uint16 index = address - BUFFER_ADDRESS_START;
            uint16 writeCount = 0;
            if ((index + byteCount) <= BUFFER_SIZE)
            {
                for (uint16 u = 0; u < byteCount; u++)
                    buffer.data [index + u] = *data++;
                writeCount = numberOfUint16; // Not optimal but as specified!
            }
            writeResponse (sp, address, writeCount);
            return;
        }

        // Check if address is in the option memory
        if (address >= OPTIONS_ADDRESS_START && // 0x300
            address <= OPTIONS_ADDRESS_END) // dynamic
        {
            if (byteCount != 2)
            {
                writeResponse (sp, address); // Error
                return;
            }
                
            uint16 value = ((uint16) data[0] << 8) | (uint16) data[1];

            writeResponse (sp, address, 
                setOptionValueAtAddress (address, value) ? 1: 0);

            return;
        }

        switch (address)
        {
            case CMD_HARD_RESET:
                if ((slaveAddress == configuration.slaveId ||
                     slaveAddress == 0) && byteCount == 0)
                    hardReset ();
                return;
                
            case CMD_RESET_SLAVE_ID:
                if ((slaveAddress == configuration.slaveId ||
                    slaveAddress == 0) && byteCount == 4)
                    checkSlaveId (data);
                return;
        }
        
        // Unknown command
        writeResponse (sp, address);

        //char string [64]; // !!!
        //sprintf (string, "cmd: %d %d %d\n", (int) index, (int) crc8, (int) computeCRC8 (index-1)); //":deadbeaf\r\n";
        //serialWrite (&sp, string, strlen(string));
    }

    // Process packet given header and footer are removed
    static inline void processPacket (SerialPort& sp)
    {
        if (!convertAsciiToBinary ())
            return; // Packet structure incorrect

        if (bufferBin.size < MIN_CMD_SIZE)
            return; // Packet size too small

        switch (bufferBin.data [1]) // Check command
        {
            case READ_INPUT_REGISTERS_CMD:
                processReadInputRegisterPacket (sp);
                break;
            case PRESET_MULT_REGISTERS_CMD:
                processPresetMultipleRegistersPacket (sp);
                break;

            default:
                // GMA !!!
                char data [64];
                sprintf (data, "command: %d\n", (int) bufferBin.data [1]); //":deadbeaf\r\n";
                serialWrite (&sp, data, strlen(data));
                break;
        }
    }

    // State machine used to cature the correct
    // formatting of the packet
    enum StateMachine
    {
        TRASH, // Unrecognized state 
        INIT,  // The packet header is detected 
        DATA,  // Packet content
        END    // Packet footer is detected
    } state = TRASH;

    // Read ongoing serial data and process packet if they are correctly composed.
    // Return the number of characters read on the serial line.
    // In case of error (e.g. SERIAL_BUFFER_OVERFLOW) return a negative number.
    // Insert this function in a loop
    int processIncomingSerialData (SerialPort* sp)
    {
        assert (sp != NULL);

        int processed = 0;
        while (serialAvailable (sp) > 0)
        {
            int raw = serialRead (sp);
            
            if (raw == SERIAL_NO_CHARACTER_AVAILABLE)
                return processed;
            processed++;

            char data = (char) raw;

            switch (data)
            {
                case ':':
                    bufferAscii.reset ();
                    state = INIT;
                    break;

                case '\n':
                    if (bufferAscii.size > 1 &&
                        bufferAscii.data[bufferAscii.size - 1] == '\r')
                    {
                        bufferAscii.size--;
                        processPacket (*sp);
                        bufferAscii.reset ();
                        state = END;
                        return processed;
                    }
                    break;
                  
                default:
                    {
                        switch (state)
                        {
                            case INIT:
                                state = DATA;
                            case DATA:
                                if (bufferAscii.add (data))
                                    break;
                                state = TRASH;
                                return SERIAL_BUFFER_OVERFLOW;
                            default:
                                // Do nothing
                                break;
                        }
                    }
                    break;
            }
        }
    }
}
