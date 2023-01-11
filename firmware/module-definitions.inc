#define CONFIGURATION_INITIALISER

int configurationIndex(int sensor, int offset) {
  return(CONFIGURATION_OFFSET_TO_FIRST_SENSOR_BLOCK + ((sensor * CONFIGURATION_SENSOR_BLOCK_SIZE) + offset));
}

unsigned char* configurationInitialiser(int& size, unsigned int eepromAddress) {
  static unsigned char *buffer = new unsigned char[size = CONFIGURATION_SIZE];
  EEPROM.get(eepromAddress, buffer);
  if (buffer[CAN_SOURCE_INDEX] == 0xff) {
    buffer[CAN_SOURCE_INDEX] = CAN_SOURCE_DEFAULT_VALUE;
    for (int sensor = 0; sensor < SENSOR_CHANNEL_COUNT; sensor++) {
      buffer[configurationIndex(sensor, CONFIGURATION_SENSOR_INSTANCE_OFFSET)] = 0xff;
      buffer[configurationIndex(sensor, CONFIGURATION_SENSOR_SAMPLE_INTERVAL_OFFSET)] = (sensor < 2)?0x03:((sensor < 4)?0x07:0x0d);
      buffer[configurationIndex(sensor, CONFIGURATION_SENSOR_TEMPERATURE_SOURCE_OFFSET)] = 0x02;
      buffer[configurationIndex(sensor, CONFIGURATION_SENSOR_SET_POINT_HI_BYTE_OFFSET)] = 0xff;
      buffer[configurationIndex(sensor, CONFIGURATION_SENSOR_SET_POINT_LO_BYTE_OFFSET)] = 0xff;
    }
    EEPROM.put(eepromAddress, buffer);
  }
  return(buffer);
}

#define CONFIGURATION_CHANGE_HANDLER

bool configurationChangeHandler(unsigned int index, unsigned char value) {
  bool retval = false;
  if (OPERATING_MODE == normal) {
    switch (index) {
      case 0: // CAN source address
        retval = true;
        break;
      case 1: case 6: case 11: case 16: case 21: case 26: // Sensor instance
        retval = (value >= 2);
        break;
      case 2: case 7: case 12: case 17: case 22: case 27: // Sensor sample interval
        retval = true;
        break;
      case 3: case 8: case 13: case 18: case 23: case 28: // NMEA temperature source code
        retval = true;
        break;
      case 4: case 9: case 14: case 19: case 24: case 29: // Set point hi byte
        retval = true;
        break;
      case 5: case 10: case 15: case 20: case 25: case 30: // Set point lo byte
        retval = true;
        break;
      default:
        break;
    }
  }
}

#define PRG_FUNCTION_HANDLER
int prgFunctionHandler(unsigned char code) {
   unsigned char deviceAddress[8];
  int retval = (int) StatusLeds::LedState::off;
  switch (code) {
    case 0: // enter registration mode
      if (OPERATING_MODE == normal) {
        OPERATING_MODE = registration;
        retval = (int) StatusLeds::LedState::flash;
      } else {
        OPERATING_MODE == normal;
        retval = (int) StatusLeds::LedState::off;
      }
      break;
    case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06:
      if (OPERATING_MODE == registration) {
        if (getNewAddressFromBus(deviceAddress)) {
          DEVICE_ADDRESSES.setAddress(value, deviceAddress);
          DEVICE_ADDRESSES.save();
          OPERATING_MODE == normal;
          retval = (int) StatusLeds::LedState::off;
        }
      }
      break;
    case 0x10: case 0x20: case 0x30: case 0x40: case 0x50: case 0x60:
      if (OPERATING_MODE == registration) {
        DEVICE_ADDRESSES.clearAddress(value >> 4);
        DEVICE_ADDRESSES.save();
        OPERATING_MODE == normal;
        retval = (int) StatusLeds::LedState::off;
      }
      break;
    default:
      break;
  }
  return(retval);
}

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



OneWireAddressTable DEVICE_ADDRESSES(SENSOR_CHANNEL_COUNT, DEVICE_ADDRESSES_EEPROM_ADDRESS);

/**********************************************************************
 * Create a ProcessQueue instance for buffering and processing
 * temperature sensor readings: sensor readings are taken at the rates
 * specified in the module configurtion and placed into the queue;
 * readings are removed from the queue and processed for immediate
 * transmission on the NMEA bus.
 * 
 * TEMPERATURE_READING_QUEUE_LENGTH specifies the number of readings
 * that can be buffered;
 * PGN130316_MAX_TRANSMIT_INTERVAL specifies the processing interval
 * and since processing equates to transmission of a PGN 130316 message
 * we set this to that PGNs maximum rate as defined in the NMEA 2000
 * specification.
 * transmitPGN230326 is the queue processing callback function which
 * arranges output of temperatue readings onto the NMEA bus.
 */
ProcessQueue<TemperatureReading> TEMPERATURE_READING_PROCESS_QUEUE(TEMPERATURE_READING_QUEUE_LENGTH, PGN130316_MAX_TRANSMIT_INTERVAL, transmitPGN130316);

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
    MODULE_CONFIGURATION.getByte(configurationIndex(temperatureReading.sensor, CONFIGURATION_SENSOR_INSTANCE_OFFSET)),
    (tN2kTempSource) MODULE_CONFIGURATION.getByte(configurationIndex(temperatureReading.sensor, CONFIGURATION_SENSOR_TEMPERATURE_SOURCE_OFFSET)),
    (double) temperatureReading.temperature,
    (double) ((MODULE_CONFIGURATION.getByte(configurationIndex(temperatureReading.sensor, CONFIGURATION_SENSOR_SET_POINT_HI_BYTE_OFFSET)) * 255) + MODULE_CONFIGURATION.getByte(configurationIndex(temperatureReading.sensor, CONFIGURATION_SENSOR_SET_POINT_LO_BYTE_OFFSET)))
  );
  NMEA2000.SendMsg(message);
  TRANSMIT_LED.setLedState(0, StatusLeds::once);
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
        STATUS_LEDS.setLedState(sensor, StatusLeds::once);
        temperatureReading.sensor = sensor;
        temperatureReading.sid = sid;
        temperatureReading.temperature = (sensors.getTempC(address) + 273.0);
        TEMPERATURE_READING_PROCESS_QUEUE.enqueue(temperatureReading);
        deadlines[sensor] = (now + (1000UL * MODULE_CONFIGURATION.getByte(configurationIndex(sensor, CONFIGURATION_SENSOR_SAMPLE_INTERVAL_OFFSET))));
      }
    }
    deadline = (now + TEMPERATURE_READING_SAMPLE_INTERVAL);
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
            TRANSMIT_LED.setLedState(0, StatusLeds::flash);
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
                    TRANSMIT_LED.setLedState(0, StatusLeds::flash);
                    break;
                  default:
                    break;
                }
                break;
              case 0xFE: // set instance number block
                if (value <= 247) {
                  for (unsigned int sensor = 0; sensor < SENSOR_CHANNEL_COUNT; sensor++) {
                    MODULE_CONFIGURATION.setByte(configurationIndex(sensor, CONFIGURATION_SENSOR_INSTANCE_OFFSET), value++);
                  }
                  MODULE_CONFIGURATION.save();
                }
                address = 0xFF;
                break;
              default: // address is set, so this is a configuration parameter value
                if ((address >= 0) && (address < CONFIGURATION_SIZE)) {
                  MODULE_CONFIGURATION.setByte(address, value);
                  MODULE_CONFIGURATION.save();
                  TRANSMIT_LED.setLedState(0, StatusLeds::off);
                }
                address = 0xFF;
                break;
            }
            break;
          case registration: // short button press in registration mode
            switch (value) {
              case 0x00:
                OPERATING_MODE = normal;
                TRANSMIT_LED.setLedState(0, StatusLeds::off);
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

