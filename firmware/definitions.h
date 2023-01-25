/**********************************************************************
 * module-definitions.inc
 */

/**
 * @brief Create a scheduler instance for transmission of PGN 127501.
 */
tN2kSyncScheduler PGN130316Schedulers[] = {
  tN2kSyncScheduler(false),
  tN2kSyncScheduler(false),
  tN2kSyncScheduler(false),
  tN2kSyncScheduler(false),
  tN2kSyncScheduler(false),
  tN2kSyncScheduler(false),
};

struct TemperatureReading { float temperature; unsigned char sid; };
TemperatureReading  TEMPERATURE_READINGS[6] = {
  { 0.0, 0 },
  { 0.0, 0 },
  { 0.0, 0 },
  { 0.0, 0 },
  { 0.0, 0 },
  { 0.0, 0 }
};


/**********************************************************************
 * Create a OneWire bus instance on the designated GPIO pin and pass
 * then create a DallasTemperature instance for operating DS18B20
 * devices on the bus.
 */
OneWire oneWire(GPIO_ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);


/**********************************************************************
 * Object for storing and persisting a collection of OneWire bus
 * hardware addresses (retrieved from temperature sensors).
 */
OneWireAddressTable DEVICE_ADDRESSES(NUMBER_OF_SUPPORTED_SENSORS, DEVICE_ADDRESSES_EEPROM_ADDRESS);

unsigned int configurationIndex(unsigned int sensor, unsigned int offset) {
  return(MODULE_CONFIGURATION_OFFSET_TO_FIRST_SENSOR_BLOCK + (sensor * MODULE_CONFIGURATION_SENSOR_BLOCK_SIZE) + offset);
}

/**
 * @brief Callback with actions to perform on CAN address claim.
 * 
 * Set the period and offset for transmission of PGN 127501 from module
 * configuration data. The SetPeriodAndOffset() function alos starts the
 * scheduler.
 */
void onN2kOpen() {
  for (unsigned int i = 0; i < NUMBER_OF_SUPPORTED_SENSORS; i++) {
    PGN130316Schedulers[i].SetPeriodAndOffset(
      (uint32_t) (MODULE_CONFIGURATION.getByte(configurationIndex(i, MODULE_CONFIGURATION_PGN130316_TRANSMIT_PERIOD_OFFSET) * 1000)),
      (uint32_t) (MODULE_CONFIGURATION.getByte(configurationIndex(i, MODULE_CONFIGURATION_PGN130316_TRANSMIT_OFFSET_OFFSET) * 10))
    );
  }
}

/**
 * @brief Scan the OneWire bus looking for an unused hardware address
 * and, if found, assign it to the identified sensor. 
 * 
 * @param sensorIndex - the sensor to which a found address should be
 *                      assigned.
 * @return true       - an address was found and assigned.
 * @return false      - no address found or other error.
 */
bool assignDeviceAddress(unsigned char unused, unsigned char sensorIndex) {
  bool retval = false;
  unsigned char deviceAddress[8];
  
  if (sensorIndex < NUMBER_OF_SUPPORTED_SENSORS) {
    for (unsigned int i = 0; i < sensors.getDeviceCount(); i++) {
      if (sensors.getAddress(deviceAddress, i)) {
        if (!DEVICE_ADDRESSES.contains(deviceAddress)) {
          DEVICE_ADDRESSES.setAddress(sensorIndex, deviceAddress);
          DEVICE_ADDRESSES.save();
          retval = true;
        }
        break;
      }
    }
  }
  return(retval);
}

/**
 * @brief Delete any hardware address associated with a specified
 * sensor.
 * 
 * @param sensorIndex - the sensor whose address should be deleted.
 * @return true       - address deleted successfully.
 * @return false      - address deletion failed (bad sensorIndex).
 */
bool deleteDeviceAddress(unsigned char unused, unsigned char sensorIndex) {
  bool retval = false;

  if (sensorIndex < NUMBER_OF_SUPPORTED_SENSORS) {
    DEVICE_ADDRESSES.clearAddress(sensorIndex);
    DEVICE_ADDRESSES.save();
    retval = true;
  }
  return(retval);
}

/**
 * @brief Assign a block of consecution instance addresses to sensors.
 * 
 * @param startValue - the first instance address in the block (must be
 *                     in the range 0..247). 
 * @return true      - the specified address was valid an all sensors
 *                     have been updated.
 * @return false     - the specified address was invalid and no sensors
 *                     have been updated.
 */
bool assignAllInstanceAddresses(unsigned char unused, unsigned char startValue) {
  bool retval = false;
  
  if (startValue < 247) {
    for (unsigned int sensor = 0; sensor < NUMBER_OF_SUPPORTED_SENSORS; sensor++) {
      MODULE_CONFIGURATION.setByte(configurationIndex(sensor, MODULE_CONFIGURATION_INSTANCE_OFFSET), startValue++);
    }
    retval = true;
  }
  return(retval);
}

/**********************************************************************
 * Override of the NOP100 configurationValidator() function.
 */

bool configurationValidator(unsigned int index, unsigned char value) {
  bool retval = false;
  
  switch (index) {
    case 0: // CAN source address
      retval = false;
      break;
    case 1: case 7: case 13: case 19: case 25: case 31: // Sensor instance
      retval = (value >= 2);
      break;
    case 2: case 8: case 14: case 20: case 26: case 32: // NMEA temperature source
      retval = (value < 16);
      break;
    case 3: case 9: case 15: case 21: case 27: case 33: // PGN130316 transmit period
      retval = true;
      break;
    case 4: case 10: case 16: case 22: case 28: case 34: // PGN130316 transmit offset
      retval = true;
      break;
    case 5: case 11: case 17: case 23: case 29: case 35: // Set point hi byte
      retval = true;
      break;
    case 6: case 12: case 18: case 24: case 30: case 36: // Set point lo byte
      retval = true;
      break;
    default:
      break;
  }
  return(retval);
}

/**********************************************************************
 * This callback function is registered with the ProcessQueue instance
 * that queues temperature readings. The function is called regularly
 * by the queue process() function and works to translate a passed
 * temperature reading into a PGN 130316 message before promptly
 * transmitting it onto the NMEA bus, flashing the transmit LED
 * appropriately.
 */
void transmitPGN130316(unsigned int sensorIndex) {
  tN2kMsg message;
  
  if (TEMPERATURE_READINGS[sensorIndex].temperature > 0) {
    SetN2kPGN130316(
      message,
      TEMPERATURE_READINGS[sensorIndex].sid, 
      MODULE_CONFIGURATION.getByte(configurationIndex(sensorIndex, MODULE_CONFIGURATION_INSTANCE_OFFSET)),
      (tN2kTempSource) MODULE_CONFIGURATION.getByte(configurationIndex(sensorIndex, MODULE_CONFIGURATION_SOURCE_OFFSET)),
      (double) TEMPERATURE_READINGS[sensorIndex].temperature,
      (double) ((MODULE_CONFIGURATION.getByte(configurationIndex(sensorIndex, MODULE_CONFIGURATION_SET_POINT_HI_BYTE_OFFSET)) * 255) + MODULE_CONFIGURATION.getByte(configurationIndex(sensorIndex, MODULE_CONFIGURATION_SET_POINT_LO_BYTE_OFFSET)))
    );
    NMEA2000.SendMsg(message);
    TRANSMIT_LED.setLedState(0, LedManager::once);
  }
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
void sampleSensorsMaybe() {
  static unsigned long deadline = 0UL;
  static unsigned char sid = 0;
  unsigned long now = millis();

  if (now > deadline) {
    for (unsigned int sensor = 0; sensor < NUMBER_OF_SUPPORTED_SENSORS; sensor++) {
      unsigned char *address = DEVICE_ADDRESSES.getAddress(sensor);
      TEMPERATURE_READINGS[sensor].sid = (address)?sid:0;
      TEMPERATURE_READINGS[sensor].temperature = (address)?(sensors.getTempC(address) + 273.0):0.0;
      if (address) STATUS_LEDS.setLedState(sensor, LedManager::once);
    }
    sid++;
    sensors.requestTemperatures();
    deadline = (now + TEMPERATURE_SENSOR_REFRESH_INTERVAL);
  }
}
