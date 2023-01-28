/**********************************************************************
 * @file defines.h
 * @author Paul Reeve (preeve@pdjr.eu)
 * @brief Define directives for TMP106.
 * @version 0.1
 * @date 2023-01-18
 * @copyright Copyright (c) 2023
 */

/**********************************************************************
 * @brief  Alias for GPIO pin used as OneWire bus connection. 
 */
#define GPIO_ONE_WIRE_BUS GPIO_D23

/**********************************************************************
 * @brief Overrides of NOP100 definitions.
 */
#define DEVICE_CLASS 75                 // Sendor Communication Interface
#define DEVICE_FUNCTION 130             // Temperature
#define DEVICE_UNIQUE_NUMBER 849        // Bump me?

#define PRODUCT_CODE 002
#define PRODUCT_FIRMWARE_VERSION "1.1.0 (Jun 2022)"
#define PRODUCT_LEN 1
#define PRODUCT_SERIAL_CODE "002-849"   // PRODUCT_CODE + DEVICE_UNIQUE_NUMBER
#define PRODUCT_TYPE "TMP106"           // The product name?
#define PRODUCT_VERSION "1.0 (Mar 2022)"

#define NMEA_TRANSMITTED_PGNS { 130316L, 0 }

/**********************************************************************
 * @brief Number of supported sensors.
 */
#define NUMBER_OF_SUPPORTED_SENSORS 6

/**********************************************************************
 * @brief Configuration details.
 */
#define MODULE_CONFIGURATION_SIZE 37
#define MODULE_CONFIGURATION_OFFSET_TO_FIRST_SENSOR_BLOCK 1
#define MODULE_CONFIGURATION_SENSOR_BLOCK_SIZE 6

#define MODULE_CONFIGURATION_INSTANCE_OFFSET 0
#define MODULE_CONFIGURATION_INSTANCE_DEFAULT 0xff

#define MODULE_CONFIGURATION_SOURCE_OFFSET 1
#define MODULE_CONFIGURATION_SOURCE_DEFAULT 0x04

#define MODULE_CONFIGURATION_PGN130316_TRANSMIT_PERIOD_OFFSET 2
#define MODULE_CONFIGURATION_PGN130316_TRANSMIT_PERIOD_DEFAULT 0x04 

#define MODULE_CONFIGURATION_PGN130316_TRANSMIT_OFFSET_OFFSET 3
#define MODULE_CONFIGURATION_PGN130316_TRANSMIT_OFFSET_DEFAULT 0x00

#define MODULE_CONFIGURATION_SET_POINT_HI_BYTE_OFFSET 4
#define MODULE_CONFIGURATION_SET_POINT_HI_BYTE_DEFAULT 0xff

#define MODULE_CONFIGURATION_SET_POINT_LO_BYTE_OFFSET 5
#define MODULE_CONFIGURATION_SET_POINT_LO_BYTE_DEFAULT 0xff

#define MODULE_CONFIGURATION_DEFAULT { \
  MODULE_CONFIGURATION_CAN_SOURCE_DEFAULT,\
  MODULE_CONFIGURATION_INSTANCE_DEFAULT,\
  MODULE_CONFIGURATION_SOURCE_DEFAULT,\
  MODULE_CONFIGURATION_PGN130316_TRANSMIT_PERIOD_DEFAULT,\
  MODULE_CONFIGURATION_PGN130316_TRANSMIT_OFFSET_DEFAULT + 0x00,\
  MODULE_CONFIGURATION_SET_POINT_HI_BYTE_DEFAULT,\
  MODULE_CONFIGURATION_SET_POINT_LO_BYTE_DEFAULT,\
  MODULE_CONFIGURATION_INSTANCE_DEFAULT,\
  MODULE_CONFIGURATION_SOURCE_DEFAULT,\
  MODULE_CONFIGURATION_PGN130316_TRANSMIT_PERIOD_DEFAULT,\
  MODULE_CONFIGURATION_PGN130316_TRANSMIT_OFFSET_DEFAULT + 0x10,\
  MODULE_CONFIGURATION_SET_POINT_HI_BYTE_DEFAULT,\
  MODULE_CONFIGURATION_SET_POINT_LO_BYTE_DEFAULT,\
  MODULE_CONFIGURATION_INSTANCE_DEFAULT,\
  MODULE_CONFIGURATION_SOURCE_DEFAULT,\
  MODULE_CONFIGURATION_PGN130316_TRANSMIT_PERIOD_DEFAULT,\
  MODULE_CONFIGURATION_PGN130316_TRANSMIT_OFFSET_DEFAULT + 0x20,\
  MODULE_CONFIGURATION_SET_POINT_HI_BYTE_DEFAULT,\
  MODULE_CONFIGURATION_SET_POINT_LO_BYTE_DEFAULT,\
  MODULE_CONFIGURATION_INSTANCE_DEFAULT,\
  MODULE_CONFIGURATION_SOURCE_DEFAULT,\
  MODULE_CONFIGURATION_PGN130316_TRANSMIT_PERIOD_DEFAULT,\
  MODULE_CONFIGURATION_PGN130316_TRANSMIT_OFFSET_DEFAULT + 0x30,\
  MODULE_CONFIGURATION_SET_POINT_HI_BYTE_DEFAULT,\
  MODULE_CONFIGURATION_SET_POINT_LO_BYTE_DEFAULT,\
  MODULE_CONFIGURATION_INSTANCE_DEFAULT,\
  MODULE_CONFIGURATION_SOURCE_DEFAULT,\
  MODULE_CONFIGURATION_PGN130316_TRANSMIT_PERIOD_DEFAULT,\
  MODULE_CONFIGURATION_PGN130316_TRANSMIT_OFFSET_DEFAULT + 0x40,\
  MODULE_CONFIGURATION_SET_POINT_HI_BYTE_DEFAULT,\
  MODULE_CONFIGURATION_SET_POINT_LO_BYTE_DEFAULT,\
  MODULE_CONFIGURATION_INSTANCE_DEFAULT,\
  MODULE_CONFIGURATION_SOURCE_DEFAULT,\
  MODULE_CONFIGURATION_PGN130316_TRANSMIT_PERIOD_DEFAULT,\
  MODULE_CONFIGURATION_PGN130316_TRANSMIT_OFFSET_DEFAULT + 0x50,\
  MODULE_CONFIGURATION_SET_POINT_HI_BYTE_DEFAULT,\
  MODULE_CONFIGURATION_SET_POINT_LO_BYTE_DEFAULT\
}

/**********************************************************************
 * FunctionMapper configuration.
 */
bool assignDeviceAddress(unsigned char, unsigned char);
bool deleteDeviceAddress(unsigned char, unsigned char);
bool assignAllInstanceAddresses(unsigned char, unsigned char);

#define FUNCTION_MAP_ARRAY {\
  { 0xff, [](unsigned char i, unsigned char v) -> bool { ModuleConfiguration.erase(); return(true); } },\
  { 0x01, assignDeviceAddress },\
  { 0x02, deleteDeviceAddress },\
  { 0x04, assignAllInstanceAddresses },\
  { 0x00, 0 }\
}

/**********************************************************************
 * @brief OneWireAddressTable is persisted at this EEPROM location.
 */
#define DEVICE_ADDRESSES_EEPROM_ADDRESS 100

/**********************************************************************
 * @brief Interval at which to refresh/read temperature sensors. 
 * 
 * This specifies the frequency in milliseconds at which temperature
 * sensors will be read and, by implication, the time allowed for
 * sensors to acquire and convert new data. As soon as readings are
 * taken, a bus command will be issued instructing all sensors to take
 * new readings.
 */
#define TEMPERATURE_SENSOR_REFRESH_INTERVAL 1000

/**********************************************************************
 * @brief We override these functions in NOP100. 
 */
#define ON_N2K_OPEN
#define CONFIGURATION_VALIDATOR
