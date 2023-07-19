#pragma once

#include "vltHelpers.hpp"

namespace voltiris
{
    // Provide an option descriptor
    // as well as get(), set() functions
    struct Option
    {
        // Unique name of the option
        char* name = NULL;

        // Type of the option
        enum Type: uint8
        {
            UINT_16,
            INT_16
        } type;

        // Check code if you change this value !
        inline uint16 getTypeSize () {return 2; }

        // Min, max and scale value of the option
        union Value
        {
            uint16 UINT_16;
            int16 INT_16;
        } min, max, scale;

        // Get the value of this option for a specific dimension (index)
        // and alternatively set the option as Readable.
        // Return the effective value of the option.
        Value (*getValue) (Option& option, uint16 index) = NULL;

        // Set the value of this option for a specific dimension (index)
        // and alternatively set the option as Writeable.
        // You need to check that value is within the defined min and max.
        // Return false if the option cannot be set.
        bool  (*setValue) (Option& option, uint16 index, Value value) = NULL;

        // Dimension of the option
        enum Dimension: uint8 
        { 
            DIM_1 = 1,
            DIM_2 = 2,
            DIM_4 = 4
        } dimension;

        // Unit of the option
        enum Unit: uint8 
        {
            NO_UNIT,
            MILLIMETERS,
            MM_PER_SEC,
            VOLTS
        } unit;
        
        // Address will automatically be assigned when adding the option via addOption ()
        uint16 address = 0;
        
        // Set your own data (effective value or custom object)
        void* userData = NULL;

        // Can option be broadcasted
        bool broadcast;

        // Initialzation method for uint16 values
        inline void init (const char* name,
                          uint16 min, uint16 max, uint16 scale,
                          Dimension dimension, Unit unit, bool broadcast)
        {
            this->name = name;
            this->type = UINT_16;
            this->min.UINT_16 = min,
            this->max.UINT_16 = max,
            this->scale.UINT_16 = scale,
            this->dimension = dimension;
            this->unit = unit;
            this->broadcast = broadcast;
        }

        // Initialzation method for int16 values
        inline void init (const char* name,
                          int16 min, int16 max, int16 scale,
                          Dimension dimension, Unit unit, bool broadcast)
        {
            this->name = name;
            this->type = INT_16;
            this->min.INT_16 = min,
            this->max.INT_16 = max,
            this->scale.INT_16 = scale,
            this->dimension = dimension;
            this->unit = unit;
            this->broadcast = broadcast;
        }

        // Convert the option into a json representation
        // Return false if buffer is too small
        bool convertToJson (uint8* buffer, uint16& bufferSize) const;
    };

    // -----------------
    // Options functions
    // -----------------

    // Add an option to the option pool.
    // Return false in case of an overflow (increase MAX_OPTIONS)
    bool addOption (Option& option);

    // Get an option given its index.
    // Return NULL in case index is out of bounds
    Option* getOption (uint16 index);

    // Get operation at address (in option memory)
    // Will retrieve the option that has a specified 
    // address and call attached callback (getValue())
    // Return false in case of an error
    // Note: assume that option size is 2!
    bool getOptionValueAtAddress (uint16 address, uint16& out);

    // Set option at address (in option memory)
    // Will retrieve the option that has a specified 
    // address and call attached callback (setValue())
    // Return false in case of an error
    // Note: assume that option size is 2!
    bool setOptionValueAtAddress (uint16 address, uint16 value);
}
