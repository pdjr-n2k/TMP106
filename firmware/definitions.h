/**
 * @file defines.h
 */

/**
 * @brief Create scheduler instances for the transmission of six
 * independent PGN130316 temperature reports.
 */
tN2kSyncScheduler PGN130316Schedulers[] = {
  tN2kSyncScheduler(false),
  tN2kSyncScheduler(false),
  tN2kSyncScheduler(false),
  tN2kSyncScheduler(false),
  tN2kSyncScheduler(false),
  tN2kSyncScheduler(false),
};

/**
 * @brief Create a structure for holding temperature readings from each
 * sensor prior to transmission.
 */
struct TemperatureReading { float temperature; unsigned char sid; };
TemperatureReading  TemperatureReadings[6] = {
  { 0.0, 0 },
  { 0.0, 0 },
  { 0.0, 0 },
  { 0.0, 0 },
  { 0.0, 0 },
  { 0.0, 0 }
};

/**
 * @brief Create a persistent OneWireAddressTable for holding the
 * hardware addresses of connected aensors.
 */
OneWireAddressTable DeviceAddresses(NUMBER_OF_SUPPORTED_SENSORS, DEVICE_ADDRESSES_EEPROM_ADDRESS);


/**
 * @brief Create a OneWire bus instance on the designated GPIO pin and
 * pass this to a DallasTemperature instance which will operate DS18B20
 * devices on the bus.
 */
OneWire oneWire(GPIO_ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

/**
 * @brief Return an index into the ModuleConfiguration buffer for a
 * particular byte in a sensor configuration block.
 * 
 * @param sensor - index of the sensor configuration block to be
 * accessed.
 * @param offset - offset of the required configuration byte in the
 * selected sensor configuration block.
 * @return unsigned int - the computed index of the specified byte
 * in the ModuleConfiguration.
 */
unsigned int configurationIndex(unsigned int sensor, unsigned int offset) {
  return(MODULE_CONFIGURATION_OFFSET_TO_FIRST_SENSOR_BLOCK + (sensor * MODULE_CONFIGURATION_SENSOR_BLOCK_SIZE) + offset);
}

/**
 * @brief Callback with actions to perform on CAN address claim.
 * 
 * Configure each transmission scheduler from the period and offset
 * values stored in the ModuleConfiguration and start scheduling.
 */
void onN2kOpen() {
  for (unsigned int sensor = 0; sensor < NUMBER_OF_SUPPORTED_SENSORS; sensor++) {
    PGN130316Schedulers[sensor].SetPeriodAndOffset(
      (uint32_t) (ModuleConfiguration.getByte(configurationIndex(sensor, MODULE_CONFIGURATION_PGN130316_TRANSMIT_PERIOD_OFFSET) * 1000)),
      (uint32_t) (ModuleConfiguration.getByte(configurationIndex(sensor, MODULE_CONFIGURATION_PGN130316_TRANSMIT_OFFSET_OFFSET) * 10))
    );
  }
}

/**
 * @brief Scan the OneWire bus looking for an unused hardware address
 * and, if found, assign it to the identified sensor.
 * 
 * This callback is used by the module FunctionHandler.
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
        if (!DeviceAddresses.contains(deviceAddress)) {
          DeviceAddresses.setAddress(sensorIndex, deviceAddress);
          DeviceAddresses.save();
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
 * This callback is used by the module FunctionHandler.
 *
 * @param sensorIndex - the sensor whose address should be deleted.
 * @return true       - address deleted successfully.
 * @return false      - address deletion failed (bad sensorIndex).
 */
bool deleteDeviceAddress(unsigned char unused, unsigned char sensorIndex) {
  bool retval = false;

  if (sensorIndex < NUMBER_OF_SUPPORTED_SENSORS) {
    DeviceAddresses.clearAddress(sensorIndex);
    DeviceAddresses.save();
    retval = true;
  }
  return(retval);
}

/**
 * @brief Assign a block of consecution instance addresses to sensors.
 * 
 * This callback is used by the module FunctionHandler.
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
      ModuleConfiguration.setByte(configurationIndex(sensor, MODULE_CONFIGURATION_INSTANCE_OFFSET), startValue++);
    }
    retval = true;
  }
  return(retval);
}

/**
 * @brief Configuration validator for TMP106.
 * 
 * Overrides the default NOP100 validator function.
 * 
 * @param index - the configuration index to be v
 * @param value 
 * @return true 
 * @return false 
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

/**
 * @brief Transmit a PGN130316 message for a specified sensor.
 * 
 * If a valid reading is available for the sensor selected by
 * sensorIndex then compose a PGN 130316 message and send it onto the
 * NMEA bus. Flash the module transmit LED to confirm.
 * 
 * @param sensorIndex - the sensor whose data should be transmitted.
 */
void transmitPGN130316(unsigned int sensorIndex) {
  tN2kMsg message;
  
  if (TemperatureReadings[sensorIndex].temperature > 0) {
    SetN2kPGN130316(
      message,
      TemperatureReadings[sensorIndex].sid, 
      ModuleConfiguration.getByte(configurationIndex(sensorIndex, MODULE_CONFIGURATION_INSTANCE_OFFSET)),
      (tN2kTempSource) ModuleConfiguration.getByte(configurationIndex(sensorIndex, MODULE_CONFIGURATION_SOURCE_OFFSET)),
      (double) TemperatureReadings[sensorIndex].temperature,
      (double) ((ModuleConfiguration.getByte(configurationIndex(sensorIndex, MODULE_CONFIGURATION_SET_POINT_HI_BYTE_OFFSET)) * 255) + ModuleConfiguration.getByte(configurationIndex(sensorIndex, MODULE_CONFIGURATION_SET_POINT_LO_BYTE_OFFSET)))
    );
    NMEA2000.SendMsg(message);
    TransmitLed.setLedState(0, tLedManager::ONCE);
  }
}

/**
 * @brief Take and buffer readings for each temperature sensor.
 * 
 * sampleSensorsMaybe should be called from loop() and at every
 * SENSOR_SAMPLE_INTERVAL it will read the most recent temperature
 * acquired by each sensor, buffer it for subsequent transmission and
 * finally request every sensor to prepare a new reading.
 */
void sampleSensorsMaybe() {
  static unsigned long deadline = 0UL;
  static unsigned char sid = 0;
  unsigned long now = millis();

  if (now > deadline) {
    for (unsigned int sensor = 0; sensor < NUMBER_OF_SUPPORTED_SENSORS; sensor++) {
      unsigned char *address = DeviceAddresses.getAddress(sensor);
      TemperatureReadings[sensor].sid = (address)?sid:0;
      TemperatureReadings[sensor].temperature = (address)?(sensors.getTempC(address) + 273.0):0.0;
      if (address) StatusLeds.setLedState(sensor, tLedManager::ONCE);
    }
    sid++;
    sensors.requestTemperatures();
    deadline = (now + TEMPERATURE_SENSOR_REFRESH_INTERVAL);
  }
}
