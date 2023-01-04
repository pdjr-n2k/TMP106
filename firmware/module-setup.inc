/**********************************************************************
 * module-setup.inc
 *
 * The contents of this file are included into the body of the host
 * firmware setup() function.
 * 
 * For the NOP100 module this file is intentionally empty.
 */

/**********************************************************************
 * Load configured DS18B20 device addresses from EEPROM.
 */
DEVICE_ADDRESSES.load();

/**********************************************************************
 * Load sensor NMEA configuration from EEPROM. If the configuration is
 * un-initialised then populate it with default values and save this
 * now initialised data back to EEPROM.
 */
MODULE_CONFIGURATION.load();
if (!MODULE_CONFIGURATION.inUse()) {
    setConfigurationDefaults();
    MODULE_CONFIGURATION.save();
}

/**********************************************************************
 * Start sensor operation on the OneWire bus.
 */
sensors.begin();

 