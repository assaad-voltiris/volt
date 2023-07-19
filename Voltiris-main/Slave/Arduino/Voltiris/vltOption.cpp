#include "vltHelpers.hpp"
#include "vltOption.hpp"

namespace voltiris
{
    static Option* options [MAX_OPTIONS];
    static uint16 optionsCount = 0;

    uint16 OPTIONS_ADDRESS_END = OPTIONS_ADDRESS_START;

    bool addOption (Option& option)
    {
        if (optionsCount + 1 >= MAX_OPTIONS)
            return false;

        // Initialize address (no shuffle)
        if (optionsCount == 0)
            option.address = OPTIONS_ADDRESS_START;
        else
            option.address = OPTIONS_ADDRESS_END;
        OPTIONS_ADDRESS_END = option.address + option.getTypeSize () * (uint16) option.dimension;
            
        options [optionsCount++] = &option;
        return true;
    }

    Option* getOption (uint16 index)
    {
        if (index >= optionsCount)
            return NULL;
        return options [index];
    }

    static Option* getOptionAtAddress (const uint16 address, uint16& index)
    {
        for (uint16 i = 0; i < optionsCount; i++)
        {
            Option* opt = options [i];
            uint16 end = opt->address + opt->getTypeSize () * (uint16) opt->dimension;
            if (address >= opt->address && address < end)
            {
                if ((address - opt->address) % opt->getTypeSize () != 0)
                    return NULL;
                index = (address - opt->address) / opt->getTypeSize ();
                return opt;
            }
        }
        return NULL;
    }

    bool getOptionValueAtAddress (uint16 address, uint16& out)
    {
        uint16 index;
        Option* opt = getOptionAtAddress (address, index);
        if (opt == NULL || opt->getValue == NULL)
            return false;

        Option::Value val = opt->getValue (*opt, index);
        switch (opt->type)
        {
            case Option::INT_16: // Implicit typecast!
            case Option::UINT_16:
                out = val.UINT_16;
                break;
            default:
                assert (false);
                return false;
        }
        return true;
    }

    bool setOptionValueAtAddress (uint16 address, uint16 value)
    {
        uint16 index;
        Option* opt = getOptionAtAddress (address, index);
        if (opt == NULL || opt->setValue == NULL)
            return false;

        Option::Value val;
        val.UINT_16 = value; // Implicit typecast!
    
        return opt->setValue (*opt, index, val);
    }

    // Inspired by https://stackoverflow.com/questions/3919995/determining-sprintf-buffer-size-whats-the-standard
    static bool addToBuffer(uint8* buffer, const uint16 bufferCapacity,
                            uint16& bufferIndex, const char* format, ...)
    {
        va_list args;
        va_start(args, format);
            int count = vsnprintf (buffer + bufferIndex, bufferCapacity - bufferIndex, format, args);
        va_end(args);

        if (count < 0 || bufferIndex + count + 1 /* safe byte for \0 */ >= bufferCapacity)
            return false; // Buffer overflow
        
        bufferIndex += (uint16) count;
        return true; 
    }

    bool Option::convertToJson (uint8* buffer, uint16& bufferSize) const
    {
        assert (bufferSize > 0);
        assert (name != NULL);

        uint16 bufferCapacity = bufferSize;
        bufferSize = 0;
        
        if (!addToBuffer (buffer, bufferCapacity, bufferSize, "{\"name\":\"%s\",", name)) return false;
        switch (type)
        {
            case UINT_16:
                if (!addToBuffer (buffer, bufferCapacity, bufferSize, "\"type\":\"uint16\",")) return false;
                if (!addToBuffer (buffer, bufferCapacity, bufferSize, "\"min\":%d,", (int) min.UINT_16)) return false;
                if (!addToBuffer (buffer, bufferCapacity, bufferSize, "\"max\":%d,", (int) max.UINT_16)) return false;
                if (!addToBuffer (buffer, bufferCapacity, bufferSize, "\"scale\":%d,", (int) scale.UINT_16)) return false;
                break;
            case INT_16:
                if (!addToBuffer (buffer, bufferCapacity, bufferSize, "\"type\":\"int16\",")) return false;
                if (!addToBuffer (buffer, bufferCapacity, bufferSize, "\"min\":%d,", (int) min.INT_16)) return false;
                if (!addToBuffer (buffer, bufferCapacity, bufferSize, "\"max\":%d,", (int) max.INT_16)) return false;
                if (!addToBuffer (buffer, bufferCapacity, bufferSize, "\"scale\":%d,", (int) scale.INT_16)) return false;
                break;
            default:
                assert (false); // Unsupported type
                break;
        }

        char accessString [3];
        uint16 accessIndex = 0;
        if (getValue != NULL)
            accessString [accessIndex++] = 'R';
        if (setValue != NULL)
            accessString [accessIndex++] = 'W';
        assert (accessIndex != 0);
        accessString [accessIndex] = 0;
        if (!addToBuffer (buffer, bufferCapacity, bufferSize, "\"access\":\"%s\",", accessString)) return false;

        switch (dimension)
        {
            case DIM_1:
            case DIM_2:
            case DIM_4:
                if (!addToBuffer (buffer, bufferCapacity, bufferSize, "\"dim\":%d,", (int) dimension)) return false;
                break;
            default:
                assert (false); // Unsupported dimension
                break;
        }
        switch (unit)
        {
            case NO_UNIT:
                break; 
            case MILLIMETERS:
                if (!addToBuffer (buffer, bufferCapacity, bufferSize, "\"unit\":\"mm\",")) return false;
                break;
            case VOLTS:
                if (!addToBuffer (buffer, bufferCapacity, bufferSize, "\"unit\":\"v\",")) return false;
                break;
            case MM_PER_SEC:
                if (!addToBuffer (buffer, bufferCapacity, bufferSize, "\"unit\":\"mm/s\",")) return false;
                break;
            default:
                assert (false); // Unsupported unit
                break;
        }

        if (!addToBuffer (buffer, bufferCapacity, bufferSize, "\"address\":%d,", (int) address)) return false;
 
        if (broadcast)
        {
            if (!addToBuffer (buffer, bufferCapacity, bufferSize, "\"broadcase\":\"true\"}")) return false;
        }
        else
        {
            if (!addToBuffer (buffer, bufferCapacity, bufferSize, "\"broadcase\":\"false\"}")) return false;
        }

        return true;
    }

}
