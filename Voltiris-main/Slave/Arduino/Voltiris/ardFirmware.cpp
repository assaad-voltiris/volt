#ifdef ARDUINO

    #include "vltHelpers.hpp"
    #include "vltOption.hpp"
    #include "vltFirmware.hpp"

    #include <Arduino.h>

    long random(long);

    namespace voltiris
    {   
        // -------------------------------------------------
        // Assertion function (make the main led blink)
        // -------------------------------------------------

        void assert (bool condition)
        {
            if (!condition)
            {
                // Make the built-in led blink  
                while (1)
                {
                    digitalWrite(LED_BUILTIN, HIGH); 
                    delay(250);                      
                    digitalWrite(LED_BUILTIN, LOW);  
                    delay(250);                     
                }
            }
        }

        // -------------------------------------------------
        // Perform a hard reset
        // -------------------------------------------------

        void (*resetFunc) (void) = 0; //declare reset function at address 0

        void hardReset ()
        {
            resetFunc ();
        }

        // -------------------------------------------------
        // Get 64 bits serial number
        // -------------------------------------------------

        uint8 serial [] = {0xde, 0xad, 0xbe, 0xef, 0xc0, 0xfe, 0xba, 0xbe};

        uint8* getSerialNumber ()
        {
            return serial;
        }

        // -------------------------------------------------
        // Get a random number between 0 and 0xff
        // -------------------------------------------------

        uint8 randomByte ()
        {
            return (uint8) ::random (0xff);
        }

        // ---------------
        // Options section
        // ---------------
    
        static Option optPosition_b1;
        static Option optPosition_b2;
        static Option optSpeed_Levels_b1;
        static Option optSpeed_Levels_b2;
        static Option optVthresh_b1;
        static Option optVthresh_b2;
        static Option optRescale_b1;
        static Option optRescale_b2;
        
        static Option::Value valPositionsB1 [2];
        static Option::Value valPositionsB2 [2];
        static Option::Value valSpeedLevelsB1 [4];
        static Option::Value valSpeedLevelsB2 [4];
        static Option::Value valVthreshB1 [4];
        static Option::Value valVthreshB2 [4];
        static Option::Value valRescaleB1 [2];
        static Option::Value valRescaleB2 [2];

        static inline void setUint16ValueWithChecks (const Option& option, const Option::Value in, Option::Value& out)
        {
            if (in.UINT_16 < option.min.UINT_16)
                out.UINT_16 = option.min.UINT_16;
            else
            {
                if (in.UINT_16 > option.max.UINT_16)
                    out.UINT_16 = option.max.UINT_16;
                else
                    out.UINT_16 = in.UINT_16;
            }
        }

        static inline void setInt16ValueWithChecks (const Option& option, const Option::Value in, Option::Value& out)
        {
            if (in.INT_16 < option.min.INT_16)
                out.INT_16 = option.min.INT_16;
            else
            {
                if (in.INT_16 > option.max.INT_16)
                    out.INT_16 = option.max.INT_16;
                else
                    out.INT_16 = in.INT_16;
            }
        }

        static void setValueWithChecks (const Option& option, const Option::Value in, Option::Value& out)
        {
            switch (option.type)
            {
                case Option::UINT_16:
                    setUint16ValueWithChecks (option, in, out);
                    break;
                case Option::INT_16:
                    setInt16ValueWithChecks (option, in, out);
                    break;
                default:
                    assert (false);
            }
        }

        static Option::Value get (Option& option, uint16 index)
        {
            assert (option.userData != NULL);
            Option::Value* values = (Option::Value*) option.userData;
            return values [index];
        }

        static bool set (Option& option, uint16 index, Option::Value value)
        {
            assert (option.userData != NULL);
            Option::Value* values = (Option::Value*) option.userData;
            setValueWithChecks (option, value, values[index]);
            return true;
        }

        Option::Value getPosition (Option& option, uint16 index)
        {
            return get (option, index);
        }

        bool setPosition (Option& option, uint16 index, Option::Value value)
        {
            return set (option, index, value);
        }

        Option::Value getSpeed (Option& option, uint16 index)
        {
            return get (option, index);
        }

        bool setSpeed (Option& option, uint16 index, Option::Value value)
        {
            return set (option, index, value);
        } 

        Option::Value getVThresh (Option& option, uint16 index)
        {
            return get (option, index);
        }

        bool setVThresh (Option& option, uint16 index, Option::Value value)
        {
            return set (option, index, value);
        }

        Option::Value getRescale (Option& option, uint16 index)
        {
            return get (option, index);
        }

        bool setRescale (Option& option, uint16 index, Option::Value value)
        {
            return set (option, index, value);
        } 

        // -------------------------------------------------
        // Is called during initialization to perform custom setup
        // -------------------------------------------------

        void customSetup ()
        {
            // Set the slave to one for debug convenience!
            configuration.slaveId = 1;
            
            // Initialize options

            optPosition_b1.init ("Position_b1", (uint16) 0, (uint16) 6000, (uint16) 10, Option::Dimension::DIM_2, Option::Unit::MILLIMETERS, false);
            optPosition_b1.setValue = setPosition;
            optPosition_b1.getValue = getPosition;
            optPosition_b1.userData = (void*) valPositionsB1;
            valPositionsB1[0].UINT_16 = optPosition_b1.min.UINT_16;
            valPositionsB1[1].UINT_16 = optPosition_b1.min.UINT_16;
            assert (addOption (optPosition_b1));

            optPosition_b2.init ("Position_b2", (uint16) 0, (uint16) 6000, (uint16) 10, Option::Dimension::DIM_2, Option::Unit::MILLIMETERS, false);
            optPosition_b2.setValue = setPosition;
            optPosition_b2.getValue = getPosition;
            optPosition_b2.userData = (void*) valPositionsB2;
            valPositionsB2[0].UINT_16 = optPosition_b2.min.UINT_16;
            valPositionsB2[1].UINT_16 = optPosition_b2.min.UINT_16;
            assert (addOption (optPosition_b2));

            optSpeed_Levels_b1.init ("Speed_Levels_b1", (uint16) 0, (uint16) 990, (uint16) 10, Option::Dimension::DIM_4, Option::Unit::MM_PER_SEC, true);
            optSpeed_Levels_b1.setValue = setSpeed;
            optSpeed_Levels_b1.getValue = getSpeed;
            optSpeed_Levels_b1.userData = (void*) valSpeedLevelsB1;
            valSpeedLevelsB1[0].UINT_16 = optSpeed_Levels_b1.min.UINT_16;
            valSpeedLevelsB1[1].UINT_16 = optSpeed_Levels_b1.min.UINT_16;
            valSpeedLevelsB1[2].UINT_16 = optSpeed_Levels_b1.min.UINT_16;
            valSpeedLevelsB1[3].UINT_16 = optSpeed_Levels_b1.min.UINT_16;
            assert (addOption (optSpeed_Levels_b1));

            optSpeed_Levels_b2.init ("Speed_Levels_b2", (uint16) 0, (uint16) 990, (uint16) 10, Option::Dimension::DIM_4, Option::Unit::MM_PER_SEC, true);
            optSpeed_Levels_b2.setValue = setSpeed;
            optSpeed_Levels_b2.getValue = getSpeed;
            optSpeed_Levels_b2.userData = (void*) valSpeedLevelsB2;
            valSpeedLevelsB2[0].UINT_16 = optSpeed_Levels_b2.min.UINT_16;
            valSpeedLevelsB2[1].UINT_16 = optSpeed_Levels_b2.min.UINT_16;
            valSpeedLevelsB2[2].UINT_16 = optSpeed_Levels_b2.min.UINT_16;
            valSpeedLevelsB2[3].UINT_16 = optSpeed_Levels_b2.min.UINT_16;
            assert (addOption (optSpeed_Levels_b2));

            optVthresh_b1.init ("Vthresh_b1", (int16) -250, (int16) 250, (int16) 10, Option::Dimension::DIM_4, Option::Unit::VOLTS, true);
            optVthresh_b1.setValue = setVThresh;
            optVthresh_b1.getValue = getVThresh;
            optVthresh_b1.userData = (void*) valVthreshB1;
            valVthreshB1[0].INT_16 = optVthresh_b1.min.INT_16;
            valVthreshB1[1].INT_16 = optVthresh_b1.min.INT_16;
            valVthreshB1[2].INT_16 = optVthresh_b1.min.INT_16;
            valVthreshB1[3].INT_16 = optVthresh_b1.min.INT_16;
            assert (addOption (optVthresh_b1));

            optVthresh_b2.init ("Vthresh_b2", (int16) -250, (int16) 250, (int16) 10, Option::Dimension::DIM_4, Option::Unit::VOLTS, true);
            optVthresh_b2.setValue = setVThresh;
            optVthresh_b2.getValue = getVThresh;
            optVthresh_b2.userData = (void*) valVthreshB2;
            valVthreshB2[0].INT_16 = optVthresh_b2.min.INT_16;
            valVthreshB2[1].INT_16 = optVthresh_b2.min.INT_16;
            valVthreshB2[2].INT_16 = optVthresh_b2.min.INT_16;
            valVthreshB2[3].INT_16 = optVthresh_b2.min.INT_16;
            assert (addOption (optVthresh_b2));

            optRescale_b1.init ("Rescale_b1", (uint16) 0, (uint16) 1000, (uint16) 100, Option::Dimension::DIM_2, Option::Unit::NO_UNIT, true);
            optRescale_b1.setValue = setRescale;
            optRescale_b1.getValue = getRescale;
            optRescale_b1.userData = (void*) valRescaleB1;
            valRescaleB1[0].UINT_16 = optRescale_b1.min.UINT_16;
            valRescaleB1[1].UINT_16 = optRescale_b1.min.UINT_16;
            assert (addOption (optRescale_b1));

            optRescale_b2.init ("Rescale_b2", (uint16) 0, (uint16) 1000, (uint16) 100, Option::Dimension::DIM_2, Option::Unit::NO_UNIT, true);
            optRescale_b2.setValue = setRescale;
            optRescale_b2.getValue = getRescale;
            optRescale_b2.userData = (void*) valRescaleB2;
            valRescaleB2[0].UINT_16 = optRescale_b2.min.UINT_16;
            valRescaleB2[1].UINT_16 = optRescale_b2.min.UINT_16;
            assert (addOption (optRescale_b2));
        }
    }

#endif