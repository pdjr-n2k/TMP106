/**********************************************************************
 * module-setup.inc
 *
 * The contents of this file are included into the body of the host
 * firmware setup() function.
 * 
 * For the NOP100 module this file is intentionally empty.
 */

DEVICE_ADDRESSES.loadFromEeprom(DEVICE_ADDRESSES_EEPROM_ADDRESS);

/**********************************************************************
 * Load SENSOR_CONFIGURATION from
SENSOR_CONFIGURATION.load()
if (!SENSOR_CONFIGURATION.inUse()) {
    setConfigurationDefaults();
    SENSOR_CONFIGURATION.save();
}

sensors.begin();

 