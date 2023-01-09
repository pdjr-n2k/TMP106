/**********************************************************************
 * module-directives.inc
 *
 * Contains #defines for the constant values which characterise a
 * particular NMEA module.
 *
 * For the NOP100 module these definitions are as vanilla / null as
 * they can be and some, perhaps all, will need to be tweaked to
 * support a real application. Additionally, definitions and function
 * declarations which are specific to the application must be appended
 * to this minimal set.
 */

 /**********************************************************************
 * DEVICE INFORMATION
 * 
 * Because of NMEA's closed standard, most of this is fiction. Maybe it
 * can be made better with more research.
 *
 * DEVICE_CLASS and DEVICE_FUNCTION are explained in the document
 * "NMEA 2000 Appendix B.6 Class & Function Codes".
 * 
 * INDUSTRY_GROUP we can be confident about (4 says maritime).
 * 
 * MANUFACTURER_CODE is only allocated to subscribed NMEA members and,
 * unsurprisingly, an anonymous code has not been assigned: 2046 is
 * currently unused, so we adopt that.  
 * 
 * UNIQUE_NUMBER is combined in some way so that together they define
 * a value which must be unique (is some way) on any N2K bus. An easy
 * way to achieve this is just to bump the unique number for every
 * software build. Really this needs automating.
 */
#define DEVICE_CLASS 75                 // Sendor Communication Interface
#define DEVICE_FUNCTION 130             // Temperature
#define DEVICE_INDUSTRY_GROUP 4         // Maritime
#define DEVICE_MANUFACTURER_CODE 2046   // Currently not allocated.
#define DEVICE_UNIQUE_NUMBER 849        // Bump me?


/**********************************************************************
 * PRODUCT INFORMATIOM
 * 
 * This poorly structured set of values is what NMEA expects a product
 * description to be shoe-horned into.
 */
#define PRODUCT_CERTIFICATION_LEVEL 1
#define PRODUCT_CODE 002
#define PRODUCT_FIRMWARE_VERSION "1.1.0 (Jun 2022)"
#define PRODUCT_LEN 1
#define PRODUCT_N2K_VERSION 2022        // The N2K specification version?
#define PRODUCT_SERIAL_CODE "002-849"   // PRODUCT_CODE + DEVICE_UNIQUE_NUMBER
#define PRODUCT_TYPE "TMP106"           // The product name?
#define PRODUCT_VERSION "1.0 (Mar 2022)"

/**********************************************************************
 * NMEA2000 LIBRARY DEFINITIONS
 *
 * NMEA_TRANSMIT_MESSAGE_PGNS is an initialiser for an array of PGNs
 * which defines the messages transmitted by this firmware. The list
 * must terminate with a zero value.
 *
 * NMEA_PGN_HANDLERS is an initialiser for a jump vector which defines
 * a list of pairs each of which maps a PGN to the function responsible
 * for processing messages of the defined type when they are received
 * over the CAN interface. For example, { 127501, myFunc }. The list
 * must terminate with the special flag value { 0, 0 }.
 */
#define NMEA_TRANSMIT_MESSAGE_PGNS { 130316UL }
#define NMEA_PGN_HANDLERS  { { 0UL, 0 } }

// Alias for GPIO pin used as OneWire bus connection. 
#define GPIO_ONE_WIRE_BUS GPIO_D23

// Number of sensor channels supported by the module.
#define SENSOR_CHANNEL_COUNT 6
// Where does the channel numbering start.
#define SENSOR_CHANNEL_INDEX_BASE 1
// Number of configuration bytes per channel. 
#define SENSOR_CONFIGURATION_BLOCK_SIZE 5
// Byte offsets within a sensor configuration block.
#define SENSOR_INSTANCE_OFFSET 0
#define SENSOR_SAMPLE_INTERVAL_OFFSET 1
#define SENSOR_TEMPERATURE_SOURCE_OFFSET 2
#define SENSOR_SET_POINT_HI_BYTE_OFFSET 3
#define SENSOR_SET_POINT_LO_BYTE_OFFSET 4

#define CONFIGURATION_SIZE (1 + (SENSOR_CHANNEL_COUNT * SENSOR_CONFIGURATION_BLOCK_SIZE))

// EEPROM address where module configuration data is saved.
#define CONFIGURATION_EEPROM_ADDRESS 10

#define DEVICE_ADDRESSES_EEPROM_ADDRESS 100

/**********************************************************************
 * The frequency at which to request sensor temperature readings.
 */
#define SENSOR_SAMPLE_INTERVAL 1000

/**********************************************************************
 * The frequency at which to poll the message transmission queue.
 */
#define PGN130316_MAX_TRANSMIT_INTERVAL 500UL

#define TEMPERATURE_READING_QUEUE_LENGTH 20
