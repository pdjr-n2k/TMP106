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
 * Start sensor operation on the OneWire bus.
 */
sensors.begin();

 