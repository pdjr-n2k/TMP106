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

enum OperatingMode { normal, registration };

OperatingMode OPERATING_MODE = normal;

OneWire oneWire(GPIO_ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);

byte[8] DEVICE_ADDRESSES[6];

void deleteDeviceAddress(int sensorIndex) {
  DEVICE_ADDRESSES[sensorIndex][0] = 0xff;
}

void assignDeviceAddress(int sensorIndex, byte[8] *address) {
  for (int i = 0; i < 8; i++) DEVICE_ADDRESSES[sensorIndex][i] = address[i];
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
  
  for (uint8_t i = 0; i < deviceCount; i++) {
    if (getAddress(address, i)) {
      if (!addressInUse(address)) return(true);
    }
  }
  return(false);
}

bool addressInUse(uint8_t *address) {
    bool retval = false;
    for (int i = 0; ((i < 6) && (retval == false)); i++) {
      retval = compareAddresses(DEVICE_ADDRESSES[i], address);
    }
    return(retval);
}

bool compareAddresses(uint8_t *addressA, uint8_t *addressB) {
  bool retval = true;
  for (int i = 0; i < 8; i++) {
    if (addressA[i] != addressB[i]) retval = false;
  }
  return(retval);
}
void configureModuleSettingMaybe(int value, bool longPress) {
  static long resetDeadline = 0UL;
  static int address = 0x00;
  long now = millis();

  switch (longpress) {
    case true:
      switch (OPERATING_MODE) {
        case normal: // In normal mode a long press signals entry of a parameter address
          switch (value) {
            case 0x01: 0x02: 0x03: 0x04: 0x05: 0x06: 0x07: 0x08: 0x09: 0x0A: 0x0B: 0x0C: 0x0D:
              address = value;
              break;
            default: // invalid parameter address
              break;
          }
          break;
        case registration: // In registration mode a long press has no meaning
          break;
      }
      break;
    case false: // short button press
      switch (OPERATING_MODE) {
        case normal: // short button press in normal mode
          switch (address) {
            case 0x00: // if address is not set then this is a mode change request.
              switch (value) {
                case 0x00: // mode change requires value to be zero
                  OPERATING_MODE = registration;
                  break;
                default:
                  break;
              }
            default: // address is set, so this is a configuration parameter value
              EEPROM.update(address, value);
              address = 0x00;
              break;
          }
          break;
        case registration: // short button press in registration mode
          switch (value) {
            case 0x00:
              OPERATING_MODE = normal
              break;
            case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06:
              if (getNewAddressFromBus(deviceAddress)) {
                
              }  scanBusForNewDevice();
              break;
            case 0x10: case 0x20: case 0x30: case 0x40: case 0x50: case 0x60:
              deleteDeviceAddress(value >> 4);
              break;
            default:
              break;
          }
          break;
      }






  // If settingAddress is something other than the default value then we
  // are in a configuration protocol waiting for a setting value and we
  // indicate this by flashing the transmit LED.
  if (settingAddress != MODULE_INSTANCE_EEPROM_ADDRESS) TRANSMIT_LED_STATE = 1;

  if (value == 0xffff) { // Perhaps cancel a timed-out protocol.
    if ((resetDeadline != 0UL) && (now > resetDeadline)) {
      resetDeadline = 0UL;
      settingAddress = MODULE_INSTANCE_EEPROM_ADDRESS;
    }
  } else if (!longPress) { // This is a short press (param is a value)
    setModuleSetting(settingAddress, (uint8_t) value);
    if (settingAddress == MODULE_INSTANCE_EEPROM_ADDRESS) MODULE_INSTANCE = EEPROM.read(settingAddress);
    settingAddress = MODULE_INSTANCE_EEPROM_ADDRESS;
  } else { // This is a long press (param is an address)
    if ((value >= FIRST_AVAILABLE_APPLICATION_EEPROM_ADDRESS) && (value < EEPROM.length())) {
      // <param> contains a valid address
      settingAddress = value;
      resetDeadline = (now + CONFIGURATION_INACTIVITY_TIMEOUT);
    } else {
      // Invalid application address
      settingAddress = MODULE_INSTANCE_EEPROM_ADDRESS;
    }
  }
}

