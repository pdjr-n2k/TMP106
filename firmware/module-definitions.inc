/**********************************************************************
 * module-definitions.inc
 *
 * The contents of this file are included into the body of the host
 * firmware after static definitions ans should be used to furnish
 * declarations of types, variables, functions, etc. that are required
 * for implementation of a particular module application.
 *
 * For the NOP100 module this file is intentionally empty.
 */

enum OperatingMode { normal, registration } OPERATING_MODE = normal;
struct TemperatureReading { unsigned int sensor; unsigned char sid; float temperature; };
void transmitPGN130316(TemperatureReading temperatureReading);

/**********************************************************************
 * Create a OneWire bus instance on the designated GPIO pin and pass
 * then create a DallasTemperature instance for operating DS18B20
 * devices on the bus.
 */
OneWire oneWire(GPIO_ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);



OneWireAddressTable DEVICE_ADDRESSES(SENSOR_CHANNEL_COUNT, DEVICE_ADDRESSES_EEPROM_ADDRESS, SENSOR_CHANNEL_INDEX_BASE);

ModuleConfiguration MODULE_CONFIGURATION(CONFIGURATION_SIZE, CONFIGURATION_EEPROM_ADDRESS);

/**********************************************************************
 * Create a ProcessQueue instance for buffering and processing
 * temperature sensor readings: sensor readings are taken at the rates
 * specified in the module configurtion and placed into the queue;
 * readings are removed from the queue and processed for immediate
 * transmission on the NMEA bus.
 * 
 * TRANSMIT_QUEUE_LENGTH specifies the number of readings that can be
 * buffered;
 * PGN130316_MAX_TRANSMIT_INTERVAL specifies the processing interval
 * and since processing equates to transmission of a PGN 130316 message
 * we set this to that PGNs maximum rate as defined in the NMEA 2000
 * specification.
 * transmitPGN230326 is the callback function which arranges output of
 * temperatue readings onto the NMEA bus.
 */
ProcessQueue<TemperatureReading> TEMPERATURE_READING_PROCESS_QUEUE(TRANSMIT_QUEUE_LENGTH, PGN130316_MAX_TRANSMIT_INTERVAL, transmitPGN130316);

/**********************************************************************
 * A control buffer which allows the application to set the required
 * state of the sensor channel LEDs. The buffer is processed at regular
 * intervals by the getStatusLedsStatus() callback which updates the
 * LEDs to the condition of the buffer.
 */
LedState SENSOR_LED_CONTROL[6] = { off, off, off, off, off, off };

/**********************************************************************
 * This callback function is registered with the ProcessQueue instance
 * that queues temperature readings. The function is called regularly
 * by the queue process() function and works to translate a passed
 * temperature reading into a PGN 130316 message before promptly
 * transmitting it onto the NMEA bus, flashing the transmit LED
 * appropriately.
 */
void transmitPGN130316(TemperatureReading temperatureReading) {
  tN2kMsg message;
  SetN2kPGN130316(
    message,
    temperatureReading.sid, 
    MODULE_CONFIGURATION.getByte((temperatureReading.sensor * CONFIGURATION_SENSOR_BLOCK_SIZE) + CONFIGURATION_SENSOR_INSTANCE_OFFSET),
    (tN2kTempSource) MODULE_CONFIGURATION.getByte((temperatureReading.sensor * CONFIGURATION_SENSOR_BLOCK_SIZE) + CONFIGURATION_SENSOR_TEMPERATURE_SOURCE_OFFSET),
    (double) temperatureReading.temperature,
    (double) ((MODULE_CONFIGURATION.getByte((temperatureReading.sensor * CONFIGURATION_SENSOR_BLOCK_SIZE) + CONFIGURATION_SENSOR_SET_POINT_HI_BYTE) * 255) + MODULE_CONFIGURATION.getByte((temperatureReading.sensor * CONFIGURATION_SENSOR_BLOCK_SIZE) + CONFIGURATION_SENSOR_SET_POINT_LO_BYTE))
  );
  NMEA2000.SendMsg(message);
  TRANSMIT_LED_STATE = once;
}

/**********************************************************************
 * sampleSensorsMaybe should be called from loop().
 *
 * Every SENSOR_SAMPLE_INTERVAL it will broadcast a temperature request
 * on the OneWire bus asking all sensors to update their data.
 *
 * Each available sensor is then checked to see if it has reached its
 * sample interval and if so the sensor temperature is read and a
 * PGN130316 temperature report message created and placed on the
 * PGN transmit queue.
 */
void sampleSensorMaybe() {
  static unsigned long deadline = 0UL;
  static unsigned long deadlines[SENSOR_CHANNEL_COUNT] = { 0UL, 0UL, 0UL, 0UL, 0UL, 0UL };
  static unsigned char sid = 0;
  unsigned long now = millis();
  TemperatureReading temperatureReading;

  if (now > deadline) {
    sensors.requestTemperatures();
    sid++;
    for (unsigned int sensor = 0; sensor < SENSOR_CHANNEL_COUNT; sensor++) {
      unsigned char *address = DEVICE_ADDRESSES.getAddress(sensor + 1);
      if ((address) && (now > deadlines[sensor])) {
        SENSOR_LED_CONTROL[sensor] = once;
        temperatureReading.sensor = sensor;
        temperatureReading.sid = sid;
        temperatureReading.temperature = (sensors.getTempC(address) + 273.0);
        TEMPERATURE_READING_PROCESS_QUEUE.enqueue(temperatureReading);
        deadlines[sensor] = (now + (1000UL * MODULE_CONFIGURATION.getByte((sensor * CONFIGURATION_SENSOR_BLOCK_SIZE) + CONFIGURATION_SENSOR_SAMPLE_INTERVAL_OFFSET)));
      }
    }
    deadline = (now + SENSOR_SAMPLE_INTERVAL);
  }
}

/**********************************************************************
 * setConfigurationDefaults initialises SENSOR_CONFIGURATION with a
 * set of default values.
 */
void setConfigurationDefaults() {
  for (int sensor = 0; sensor < SENSOR_CHANNEL_COUNT; sensor++) {
    MODULE_CONFIGURATION.setByte((sensor * CONFIGURATION_SENSOR_BLOCK_SIZE) + CONFIGURATION_SENSOR_INSTANCE_OFFSET, 0xFF);
    MODULE_CONFIGURATION.setByte((sensor * CONFIGURATION_SENSOR_BLOCK_SIZE) + CONFIGURATION_SENSOR_SAMPLE_INTERVAL_OFFSET, (sensor < 2)?0x03:((sensor < 5)?0x07:0x0D));
    MODULE_CONFIGURATION.setByte((sensor * CONFIGURATION_SENSOR_BLOCK_SIZE) + CONFIGURATION_SENSOR_TEMPERATURE_SOURCE_OFFSET, 0x02);
    MODULE_CONFIGURATION.setByte((sensor * CONFIGURATION_SENSOR_BLOCK_SIZE) + CONFIGURATION_SENSOR_SET_POINT_HI_BYTE, 0xFF);
    MODULE_CONFIGURATION.setByte((sensor * CONFIGURATION_SENSOR_BLOCK_SIZE) + CONFIGURATION_SENSOR_SET_POINT_LO_BYTE, 0xFF);
  }
}

/**********************************************************************
 * getNewAddressFromBus reads the addresses of devices on the OneWire
 * bus looking for an address that does not exist in the
 * DEVICE_ADDRESSES array. If such an address is found then true is
 * returned and the discovered address is written to <address> else
 * the function returns false and address is unmodified.
 */
bool getNewAddressFromBus(unsigned char *address) {
  unsigned char deviceCount = sensors.getDeviceCount();
  
  for (int i = 0; i < deviceCount; i++) {
    if (sensors.getAddress(address, i)) {
      if (!DEVICE_ADDRESSES.contains(address)) return(true);
    }
  }
  return(false);
}

#define CONFIGURE_MODULE_SETTING_MAYBE
void configureModuleSettingMaybe(int value, bool longPress) {
  static long resetDeadline = 0UL;
  static int address = 0xFF;
  unsigned char deviceAddress[8];
  long now = millis();

  if (value == 0xffff) { // Perhaps cancel a timed-out protocol.
    if ((resetDeadline != 0UL) && (now > resetDeadline)) {
      resetDeadline = 0UL;
      address = 0xFF;
    }
  } else {
    switch (longPress) {
      case true:
        switch (OPERATING_MODE) {
          case normal: // In normal mode a long press signals entry of a parameter address
            address = value;
            TRANSMIT_LED_STATE = flash;
            resetDeadline = (now + CONFIGURATION_INACTIVITY_TIMEOUT);
            break;
          case registration: // In registration mode a long press has no meaning
            break;
        }
        break;
      case false: // short button press
        switch (OPERATING_MODE) {
          case normal: // short button press in normal mode
            switch (address) {
              case 0xFF: // if address not set -> then this is a special action.
                switch (value) {
                  case 0x00: // mode change requires value to be zero
                    OPERATING_MODE = registration;
                    TRANSMIT_LED_STATE = flash;
                    break;
                  default:
                    break;
                }
                break;
              case 0xFE: // set instance number block
                if (value <= 247) {
                  for (unsigned int sensor = 0; sensor < SENSOR_CHANNEL_COUNT; sensor++) {
                    MODULE_CONFIGURATION.setByte((sensor * CONFIGURATION_SENSOR_BLOCK_SIZE) + CONFIGURATION_SENSOR_INSTANCE_OFFSET, value++);
                  }
                  MODULE_CONFIGURATION.save();
                }
                address = 0xFF;
                break;
              default: // address is set, so this is a configuration parameter value
                if ((address >= 0) && (address < CONFIGURATION_SIZE)) {
                  MODULE_CONFIGURATION.setByte(address, value);
                  MODULE_CONFIGURATION.save();
                  TRANSMIT_LED_STATE = off;
                }
                address = 0xFF;
                break;
            }
            break;
          case registration: // short button press in registration mode
            switch (value) {
              case 0x00:
                OPERATING_MODE = normal;
                TRANSMIT_LED_STATE = off;
                break;
              case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06:
                if (getNewAddressFromBus(deviceAddress)) {
                  DEVICE_ADDRESSES.setAddress(value, deviceAddress);
                  DEVICE_ADDRESSES.save();
                }
                break;
              case 0x10: case 0x20: case 0x30: case 0x40: case 0x50: case 0x60:
                DEVICE_ADDRESSES.clearAddress(value >> 4);
                DEVICE_ADDRESSES.save();
                break;
              default:
                break;
            }
            break;
        }
        break;
    }
  }
}

/**********************************************************************
 * Overrides getStatusLedsStatus() defined in NOP100. Returns a single
 * byte suitable for direct transfer to the SIPO device used to drive
 * the module status LEDs. The status byte value is derived from the
 * control values in the SENSOR_LED_CONTROL array.
 */
#define GET_STATUS_LEDS_STATUS
unsigned char getStatusLedsStatus() {
  unsigned char status = 0x00;

  for (unsigned int sensor = 0; sensor < SENSOR_CHANNEL_COUNT; sensor++) {
    switch (SENSOR_LED_CONTROL[sensor]) {
      case on:
        status |= (0x01 << sensor);
        break;
      case off:
        status &= ~(0x01 << sensor);
        break;
      case once:
        status |= (0x01 << sensor);
        SENSOR_LED_CONTROL[sensor] = off;
        break;
      case flash:
        SENSOR_LED_CONTROL[sensor] = flashOn;
        break;
      case flashOn:
        status |= (0x01 << sensor);
        SENSOR_LED_CONTROL[sensor] = flashOff;
      case flashOff:
        status &= ~(0x01 << sensor);
        SENSOR_LED_CONTROL[sensor] = flashOn;
    }
  }
  return(status);
}