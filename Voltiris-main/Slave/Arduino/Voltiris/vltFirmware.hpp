#pragma once

#include "vltHelpers.hpp"

// Contains the callback to be implemented in Firmware
// All function do not rely on dynamic memory allocation.
// As such no poiinter on object should be disposed with delete() keyword

namespace voltiris
{
    // ---------------------------
    // The following functions are
    // IMPLEMENTATION SPECIFIC
    // ---------------------------

    // Get a random number between 0 and 0xff
    // IMPLEMENTATION SPECIFIC
    uint8 randomByte ();

    // Get 64 bits buffer containing serial number
    // IMPLEMENTATION SPECIFIC
    uint8* getSerialNumber ();

    // Perform a hard reset
    // IMPLEMENTATION SPECIFIC
    void hardReset ();

    // Is called during initialization to perform custom setup
    // IMPLEMENTATION SPECIFIC
    void customSetup ();

    // --------------------------
    // Initialization and globals
    // --------------------------

    struct Configuration {
        uint8 slaveId;
    };

    extern Configuration configuration;

    // Should be called once to initialize library
    void initialize ();    
}
