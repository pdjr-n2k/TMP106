/**********************************************************************
 * module-setup.inc
 *
 * The contents of this file are included into the body of the host
 * firmware setup() function.
 * 
 * For the NOP100 module this file is intentionally empty.
 */

FunctionMapper.addHandler(FUNCTION_MAPPER_CODE_FOR_ASSIGN_SENSOR_DEVICE_ADDRESS, assignDeviceAddress);
FunctionMapper.addHandler(FUNCTION_MAPPER_CODE_FOR_DELETE_SENSOR_DEVICE_ADDRESS, deleteDeviceAddress);
FunctionMapper.addHandler(FUNCTION_MAPPER_CODE_FOR_ASSIGN_ALL_INSTANCE_ADDRESSES, assignAllInstanceAddresses);

/**********************************************************************
 * Load configured DS18B20 device addresses from EEPROM.
 */
DeviceAddresses.load();

/**********************************************************************
 * Start sensor operation on the OneWire bus.
 */
sensors.begin();

 