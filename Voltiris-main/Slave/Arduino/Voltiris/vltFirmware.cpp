#include "vltFirmware.hpp"

namespace voltiris
{
    Configuration configuration;

    void initialize ()
    {
        buffer.reset ();
        configuration.slaveId = (randomByte () % MAX_SLAVE_ID) + 1;
        customSetup ();
    }
}