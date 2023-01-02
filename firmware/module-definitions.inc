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
struct TransmitQueueEntry{ unsigned char instance; float kelvin };

OneWire oneWire(GPIO_ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);

OneWireAddressTable DEVICE_ADDRESSES(SENSOR_CHANNEL_COUNT, SENSOR_CHANNEL_INDEX_BASE);

ModuleConfiguration MODULE_CONFIGURATION(CONFIGURATION_SIZE, CONFIGURATION_EEPROM_ADDRESS, SENSOR_CHANNEL_INDEX_BASE);

ArduinoQueue<TransmitQueueEntry> TRANSMIT_QUEUE(TRANSMIT_QUEUE_LENGTH);

void sampleSensorMaybe() {
  static unsigned long deadline = 0UL;
  static unsigned long deadlines[SENSOR_CHANNEL_COUNT] = { 0UL, 0UL, 0UL, 0UL, 0UL, 0UL };
  unsigned long now = millis();

  if (now > deadline) {
    sensors.requestTemperatures();
    for (unsigned int sensor = 0; sensor < SENSOR_CHANNEL_COUNT; sensor++) {
      unsigned char *address = DEVICE_ADDRESSES.getAddress(sensor + 1);
      if ((address) && (now > deadlines[sensor])) {
        float tempc = (sensors.getTempC(address) + 273.0);
        // create a PGN and queue it.
        deadlines[sensor] = (now + (1000UL * MODULE_CONFIGURATION.getByte((sensor * CONFIGURATION_SENSOR_BLOCK_SIZE) + CONFIGURATION_SENSOR_SAMPLE_INTERVAL_OFFSET)));
      }
    }
    deadline = (now + SENSOR_SAMPLE_INTERVAL);
}

/**********************************************************************
 * setConfigurationDefaults initialises SENSOR_CONFIGURATION with a
 * set of default values.
 */
void setConfigurationDefaults() {
  for (int sensor = 0; sensor < SENSOR_CHANNEL_COUNT; sensor++) {
    MODULE_CONFIGURATION.setByte((sensor * CONFIGURATION_SENSOR_BLOCK_SIZE) + CONFIGURATION_SENSOR_INSTANCE_OFFSET) = 0xFF;
    MODULE_CONFIGURATION.setByte((sensor * CONFIGURATION_SENSOR_BLOCK_SIZE) + CONFIGURATION_SENSOR_SAMPLE_INTERVAL_OFFSET) = (sensor < 2)?0x03:((sensor < 5)?0x07:0x0D);
  }
}

/**********************************************************************
 * getNewAddressFromBus reads the addresses of devices on the OneWire
 * bus looking for an address that does not exist in the
 * DEVICE_ADDRESSES array. If such an address is found then true is
 * returned and the discovered address is written to <address> else
 * the function returns false and address is unmodified.
 */
bool getNewAddressFromBus(unit8_t *address) {
  bool retval = false;
  uint8_t deviceCount = getDeviceCount();
  
  for (int i = 0; i < deviceCount; i++) {
    if (getAddress(address, i)) {
      if (!DEVICE_ADDRESSES->contains(address)) return(true);
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
    switch (longpress) {
      case true:
        switch (OPERATING_MODE) {
          case normal: // In normal mode a long press signals entry of a parameter address
            address = value;
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
                switch (value) {}
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
                  DEVICE_ADDRESSES.saveToEeprom(DEVICE_ADDRESSES_EEPROM_ADDRESS);
                }
                break;
              case 0x10: case 0x20: case 0x30: case 0x40: case 0x50: case 0x60:
                DEVICE_ADDRESSES.clearAddress(value >> 4);
                DEVICE_ADDRESSES.saveToEeprom(DEVICE_ADDRESSES_EEPROM_ADDRESS);
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
