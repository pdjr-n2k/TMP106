/**
 * @file defines.h
 * @author Paul Reeve (preeve@pdjr.eu)
 * @brief Define directives for TMP106.
 * @version 0.1
 * @date 2023-01-18
 * @copyright Copyright (c) 2023
 */

/**
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

#define NMEA_TRANSMIT_MESSAGE_PGNS { 130316L, 0 }

/**
 * @brief  Alias for GPIO pin used as OneWire bus connection. 
 */
#define GPIO_ONE_WIRE_BUS GPIO_D23

/**
 * @brief Number of supported sensors.
 */
#define NUMBER_OF_SUPPORTED_SENSORS 6

/**
 * @brief Configuration details.
 */
#define CM_SIZE (CM_OFFSET_TO_FIRST_SENSOR_BLOCK + (NUMBER_OF_SUPPORTED_SENSORS * CM_SENSOR_BLOCK_SIZE))
#define CM_OFFSET_TO_FIRST_SENSOR_BLOCK 1
#define CM_SENSOR_BLOCK_SIZE 5
#define CM_SENSOR_INSTANCE_OFFSET 0
#define CM_SENSOR_SAMPLE_INTERVAL_OFFSET 1
#define CM_SENSOR_TEMPERATURE_SOURCE_OFFSET 2
#define CM_SENSOR_SET_POINT_HI_BYTE_OFFSET 3
#define CM_SENSOR_SET_POINT_LO_BYTE_OFFSET 4
#define CM_INSTANCE_DEFAULT 0xff
#define CM_TEMPERATURE_SOURCE_DEFAULT 0x04
#define CM_SET_POINT_HI_BYTE_DEFAULT 0xff
#define CM_SET_POINT_LO_BYTE_DEFAULT 0xff

/**
 * @brief OneWireAddressTable is persisted at this EEPROM location.
 */
#define DEVICE_ADDRESSES_EEPROM_ADDRESS 100

/**
 * @brief Queue for buffering temperature readings awaiting transmission.
 */
#define TEMPERATURE_READING_QUEUE_LENGTH 20

/**
 * @brief Interval at which to read temperature sensors. 
 * 
 * This specifies the frequency at which all temperature sensors will
 * be commanded to refresh their current temperature reading. A sensor
 * is actually read only at the interval defined in its configuration. 
 */
#define TEMPERATURE_SENSOR_REFRESH_INTERVAL 1000

/**
 * @brief The frequency at which to poll the message transmission
 *        queue. 
 * 
 * This corresponds to the maximum transmission rate supported by the
 * module. The NMEA2000 specification sets this at 500ms for PGN
 * 130316.
 */
#define PGN130316_MAX_TRANSMIT_INTERVAL 500UL
