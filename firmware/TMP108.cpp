/**********************************************************************
 * TMP108.cpp (c) 2021-22 Paul Reeve <preeve@pdjr.eu>
 *
 * Target platform: Teensy 3.2
 * Supported temperature sensors: LM335Z
 * 
 * This firmware implements an 8-channel temperature sensor interface
 * that reports sensor state over NMEA 2000 using
 * PGN 130316 Temperature, Extended Range.
 * 
 * The firmware is designed as a simple state machine. At any one time
 * the device is in either its production state (in which it is reading
 * sensor data and transmitting it over NMEA), or in one of a handful
 * of configuration states associated with user-mediated configuration
 * of the module.
 * 
 * Transition from production state into a configuration state and
 * transitions between configuration states are triggered by a debounced
 * signal on an MCU input pin which will typically be connected to a
 * momentary push button.
 * 
 * Transition back to the production state derives from the user advancing
 * through configuration states which are part of a configuration protocol
 * or by a configuration protocol timing out.
 */

#include <Arduino.h>
#include <ArduinoQueue.h>
#include <EEPROM.h>
#include <NMEA2000_CAN.h>
#include <N2kTypes.h>
#include <N2kMessages.h>
#include <Debouncer.h>
#include <LedManager.h>
#include <DilSwitch.h>
#include <Sensor.h>
#include <arraymacros.h>

/**********************************************************************
 * SERIAL DEBUG
 * 
 * The firmware can be built so that it writes copious trace and debug
 * data to the MCU's serial output by defining DEBUG_SERIAL. When the
 * Teensy reboots it switches its USB port to serial emulation and it
 * can take a few seconds for a connected host computer to recognise
 * the switch: - DEBUG_SERIAL_START_DELAY introduces a delay that can
 * be used to prevent loss of early debug output.
 */
#define DEBUG_SERIAL
#define DEBUG_SERIAL_START_DELAY 4000

/**********************************************************************
 * MCU EEPROM (PERSISTENT) STORAGE
 * 
 * Module configuration is persisted to Teensy EEPROM storage.
 * 
 * SOURCE_ADDRESS_EEPROM_ADDRESS is the storage address for the
 * module's 1-byte N2K/CAN source address.
 * 
 * SENSORS_EEPROM_ADDRESS is the start address of storage used to hold
 * SENSOR congigurations. The length of this area will vary if the
 * Sensor ADT is redefined, so it is simplest to make sure the start
 * address remains as the last item in storage.
 */
#define SOURCE_ADDRESS_EEPROM_ADDRESS 0
#define SENSORS_EEPROM_ADDRESS 1

/**********************************************************************
 * MCU PIN DEFINITIONS
 * 
 * GPIO pin definitions for the Teensy 3.2 MCU and some collections
 * that can be used as array initialisers
 */
#define GPIO_SETPOINT_LED 0
#define GPIO_SOURCE_LED 1
#define GPIO_INSTANCE_LED 2
#define GPIO_ENCODER_BIT7 5
#define GPIO_ENCODER_BIT6 6
#define GPIO_ENCODER_BIT5 7
#define GPIO_ENCODER_BIT4 8
#define GPIO_ENCODER_BIT3 9
#define GPIO_ENCODER_BIT2 10
#define GPIO_ENCODER_BIT1 11
#define GPIO_ENCODER_BIT0 12
#define GPIO_INTERVAL_LED 13
#define GPIO_SENSOR0 A0
#define GPIO_SENSOR1 A1
#define GPIO_SENSOR2 A2
#define GPIO_SENSOR3 A3
#define GPIO_SENSOR4 A4
#define GPIO_SENSOR5 A5
#define GPIO_SENSOR6 A6
#define GPIO_SENSOR7 A7
#define GPIO_PROGRAMME_SWITCH 22
#define GPIO_POWER_LED 23
#define GPIO_SENSOR_PINS { GPIO_SENSOR0, GPIO_SENSOR1, GPIO_SENSOR2, GPIO_SENSOR3, GPIO_SENSOR4, GPIO_SENSOR5, GPIO_SENSOR6, GPIO_SENSOR7 } 
#define GPIO_ENCODER_PINS { GPIO_ENCODER_BIT0, GPIO_ENCODER_BIT1, GPIO_ENCODER_BIT2, GPIO_ENCODER_BIT3, GPIO_ENCODER_BIT4, GPIO_ENCODER_BIT5, GPIO_ENCODER_BIT6, GPIO_ENCODER_BIT7 }
#define GPIO_INPUT_PINS { GPIO_PROGRAMME_SWITCH, GPIO_ENCODER_BIT0, GPIO_ENCODER_BIT1, GPIO_ENCODER_BIT2, GPIO_ENCODER_BIT3, GPIO_ENCODER_BIT4, GPIO_ENCODER_BIT5, GPIO_ENCODER_BIT6, GPIO_ENCODER_BIT7 }
#define GPIO_OUTPUT_PINS { GPIO_INTERVAL_LED, GPIO_POWER_LED, GPIO_INSTANCE_LED, GPIO_SOURCE_LED, GPIO_SETPOINT_LED }
#define SENSOR_TRANSMIT_DEADLINE_INITIALISER { 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL }

/**********************************************************************
 * DEVICE INFORMATION
 * 
 * Because of NMEA's closed standard, most of this is fiction. Maybe it
 * can be made better with more research. In particular, even recent
 * releases of the NMEA function and class lists found using Google
 * don't discuss anchor systems, so the proper values for CLASS and
 * FUNCTION in this application are not known.  At the moment they are
 * set to 25 (network device) and 130 (PC gateway).
 * 
 * INDUSTRY_GROUP we can be confident about (4 says maritime). However,
 * MANUFACTURER_CODE is only allocated to subscribed NMEA members and,
 * unsurprisingly, an anonymous code has not been assigned: 2046 is
 * currently unused, so we adopt that.  
 * 
 * MANUFACTURER_CODE and UNIQUE_NUMBER together must make a unique
 * value on any N2K bus and an easy way to achieve this is just to
 * bump the unique number for every software build and this is done
 * automatically by the build system.
 */
#define DEVICE_CLASS 75
#define DEVICE_FUNCTION 130
#define DEVICE_INDUSTRY_GROUP 4
#define DEVICE_MANUFACTURER_CODE 2046
#define DEVICE_UNIQUE_NUMBER 849

/**********************************************************************
 * PRODUCT INFORMATION
 * 
 * This poorly structured set of values is what NMEA expects a product
 * description to be shoe-horned into.
 */
#define PRODUCT_CERTIFICATION_LEVEL 1
#define PRODUCT_CODE 002
#define PRODUCT_FIRMWARE_VERSION "1.1.0 (Jun 2022)"
#define PRODUCT_LEN 1
#define PRODUCT_N2K_VERSION 2101
#define PRODUCT_SERIAL_CODE "002-849" // PRODUCT_CODE + DEVICE_UNIQUE_NUMBER
#define PRODUCT_TYPE "TSENSE"
#define PRODUCT_VERSION "1.0 (Mar 2021)"

/**********************************************************************
 * Include the build.h header file which can be used to override any or
 * all of the above  constant definitions.
 */
#include "build.h"



#define DEFAULT_SOURCE_ADDRESS 22         // Seed value for source address claim
#define INSTANCE_UNDEFINED 255            // Flag value
#define STARTUP_SETTLE_PERIOD 5000        // Wait this many ms after boot before entering production
#define SWITCH_PROCESS_INTERVAL 250       // Process switch inputs every n ms
#define LED_MANAGER_HEARTBEAT 200         // Number of ms on / off
#define LED_MANAGER_INTERVAL 10           // Number of heartbeats between repeats
#define CONFIG_TIMEOUT_INTERVAL 20000     // Allow 20s to complete each programme step
#define SENSOR_VOLTS_TO_KELVIN 3.3        // Conversion factor for LM335 temperature sensors
#define ANALOG_READ_AVAERAGE 10           // Number of ADC samples that average to make a read value
#define ANALOG_RESOLUTION 1024            // ADC maximum return value
#define TRANSMIT_QUEUE_LENGTH 20          // Max number of entries in the transmit queue

/**********************************************************************
 * NMEA transmit configuration. Modules that transmit PGN 130316 are
 * required to honour a 0.5s cycle limit - this means that the max rate
 * at which the module can transmit PGN 130316 is once every 500ms. At
 * the same time, the maximum transmit rate for a single sensor
 * instance is once every 2 seconds. These values are defined below as
 * defaults.
 * 
 * However, this module has 8 sensors. If every sensor attempts to
 * transmit at this maximum rate, then the 8 messages will hit the
 * output buffer every two seconds whilst only four messages can
 * actually be transmitted.
 * 
 * Make sure that when you configure sensor channels you choose an
 * appropriate transmit interval so that buffer overrun does not
 * become an issue.
 */
#define MINIMUM_TRANSMIT_INTERVAL 2000UL    // N2K defined fastest allowed transmit rate for a PGN instance
#define MINIMUM_TRANSMIT_CYCLE 500UL        // N2K defined fastest allowed transmit cycle rate

/**********************************************************************
 * This firmware operates as a state machine. At any moment in time the
 * system is either operating normally (i.e. processing/transmitting
 * temperature readings) or it is stepping through a user-mediated,
 * multi-state, configuration process. This enumerates all possible
 * machine states.
 */
enum MACHINE_STATES { NORMAL, CHANGE_CHANNEL_INSTANCE, CHANGE_CHANNEL_SOURCE, CHANGE_CHANNEL_SETPOINT, CHANGE_CHANNEL_INTERVAL, CANCEL_CONFIGURATION };

/**********************************************************************
 * Declarations of local functions.
 */
#ifdef DEBUG_SERIAL
void dumpSensorConfiguration();
#endif
void messageHandler(const tN2kMsg&);

void processSensorsMaybe();
void processProgrammeSwitchMaybe();
void processTransmitQueueMaybe();
void performConfigurationTimeoutMaybe();

enum MACHINE_STATES performMachineStateTransition(enum MACHINE_STATES state);
void confirmDialogCompletion(int flashes);
void transmitPgn130316(Sensor sensor, bool flash = true);

/**********************************************************************
 * PGNs of messages transmitted by this program.
 * 
 * PGN 130316 Temperature, Extended Range is used to broadcast sensed
 * temperatures.
 */
const unsigned long TransmitMessages[] PROGMEM={ 130316L, 0 };

/**********************************************************************
 * PGNs of messages handled by this program.
 * 
 * There are none.
 */
typedef struct { unsigned long PGN; void (*Handler)(const tN2kMsg &N2kMsg); } tNMEA2000Handler;
tNMEA2000Handler NMEA2000Handlers[]={ {0, 0} };

/**********************************************************************
 * DIL_SWITCH switch decoder.
 */
int ENCODER_PINS[] = GPIO_ENCODER_PINS;
DilSwitch DIL_SWITCH (ENCODER_PINS, ELEMENTCOUNT(ENCODER_PINS));

/**********************************************************************
 * DEBOUNCER for the programme switch.
 */
int SWITCHES[DEBOUNCER_SIZE] = { GPIO_PROGRAMME_SWITCH, -1, -1, -1, -1, -1, -1, -1 };
Debouncer DEBOUNCER (SWITCHES);

/**********************************************************************
 * LED_MANAGER for all system LEDs.
 */
LedManager LED_MANAGER (LED_MANAGER_HEARTBEAT, LED_MANAGER_INTERVAL);

/**********************************************************************
 * SENSORS array of Sensor objects.
 */
unsigned char SENSOR_PINS[] = GPIO_SENSOR_PINS;
Sensor SENSORS[ELEMENTCOUNT(SENSOR_PINS)];

static MACHINE_STATES MACHINE_STATE = NORMAL;
unsigned long CONFIGURATION_TIMEOUT_COUNTER = 0UL;

/**********************************************************************
 * SID for clustering N2K messages by sensor process cycle.
 */
unsigned char SID = 0;

/**********************************************************************
 * The index number of sensor data that should be transmitted is placed
 * in a queue for subsequent transmission.
 */
ArduinoQueue<int> TRANSMIT_QUEUE(TRANSMIT_QUEUE_LENGTH);

/**********************************************************************
 * MAIN PROGRAM - setup()
 */
void setup() {
  #ifdef DEBUG_SERIAL
  Serial.begin(9600);
  delay(DEBUG_SERIAL_START_DELAY);
  #endif

  // Set the mode of all digital GPIO pins.
  int ipins[] = GPIO_INPUT_PINS;
  int opins[] = GPIO_OUTPUT_PINS;
  for (unsigned int i = 0 ; i < ELEMENTCOUNT(ipins); i++) pinMode(ipins[i], INPUT_PULLUP);
  for (unsigned int i = 0 ; i < ELEMENTCOUNT(opins); i++) pinMode(opins[i], OUTPUT);
  for (unsigned int i = 0 ; i < ELEMENTCOUNT(SENSOR_PINS); i++) pinMode(SENSOR_PINS[i], INPUT);

  // Configure the ADC.
  analogReadAveraging(ANALOG_READ_AVAERAGE);

  // Initialise SENSORS array, assigning GPIO numbers.
  for (unsigned int i = 0; i < ELEMENTCOUNT(SENSORS); i++) SENSORS[i].invalidate(SENSOR_PINS[i]); 
  
  // We assume that a new host system has its EEPROM initialised to all
  // 0xFF. We test by reading a byte that in a configured system should
  // never be this value and if it indicates a scratch system then we
  // set EEPROM memory up in the following way.
  //
  // Address | Value                                    | Size in bytes
  // --------+------------------------------------------+--------------
  // 0x00    | N2K source address                       | 1
  // 0x10    | Sensor configuration (the SENSORS array) | Lots
  //
  //EEPROM.write(SOURCE_ADDRESS_EEPROM_ADDRESS, 0xff);
  if (EEPROM.read(SOURCE_ADDRESS_EEPROM_ADDRESS) == 0xff) {
    EEPROM.write(SOURCE_ADDRESS_EEPROM_ADDRESS, DEFAULT_SOURCE_ADDRESS);
    for (unsigned int i = 0; i < ELEMENTCOUNT(SENSORS); i++) SENSORS[i].save(SENSORS_EEPROM_ADDRESS + (i * SENSORS[i].getConfigSize()));
  }

  // Load sensor configurations from EEPROM  
  for (unsigned int i = 0; i < ELEMENTCOUNT(SENSORS); i++) SENSORS[i].load(SENSORS_EEPROM_ADDRESS + (i * SENSORS[i].getConfigSize()));

  // Flash the board LED n times (where n = number of configured sensors)
  int n = 0;
  for (unsigned int i = 0; i < ELEMENTCOUNT(SENSORS); i++) if (SENSORS[i].getInstance() != 0xff) n++;
  LED_MANAGER.operate(GPIO_INTERVAL_LED, 0, n);

  // Initialise and start N2K services.
  NMEA2000.SetProductInformation(PRODUCT_SERIAL_CODE, PRODUCT_CODE, PRODUCT_TYPE, PRODUCT_FIRMWARE_VERSION, PRODUCT_VERSION);
  NMEA2000.SetDeviceInformation(DEVICE_UNIQUE_NUMBER, DEVICE_FUNCTION, DEVICE_CLASS, DEVICE_MANUFACTURER_CODE);
  NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode, EEPROM.read(SOURCE_ADDRESS_EEPROM_ADDRESS)); // Configure for sending and receiving.
  NMEA2000.EnableForward(false); // Disable all msg forwarding to USB (=Serial)
  NMEA2000.ExtendTransmitMessages(TransmitMessages); // Tell library which PGNs we transmit
  NMEA2000.SetMsgHandler(messageHandler);
  NMEA2000.Open();
}

/**********************************************************************
 * MAIN PROGRAM - loop()
 * 
 * With the exception of NMEA2000.parseMessages() all of the functions
 * called from loop() implement interval timers which ensure that they
 * will mostly return immediately, only performing their substantive
 * tasks at intervals defined by program constants.
 * 
 * The global constant JUST_STARTED is used to delay acting on switch
 * inputs until a newly started system has stabilised and the GPIO
 * inputs have been debounced.
 */ 
void loop() {
  static bool JUST_STARTED = true;

  if (JUST_STARTED && (millis() > STARTUP_SETTLE_PERIOD)) {
    #ifdef DEBUG_SERIAL
    Serial.println();
    Serial.print("Starting. N2K Source address is "); Serial.print(NMEA2000.GetN2kSource()); Serial.println();
    dumpSensorConfiguration();
    #endif
    JUST_STARTED = false;
  }

  // Debounce all switch inputs.
  DEBOUNCER.debounce();

  // If the system has settled (had time to debounce) then handle any
  // changes to MACHINE_STATE by reading the current value of DIL_SWITCH
  // and calling performMachineStateTransition(). Changes to MACHINE_STATE are
  // only made by processSwitches() and performConfigurationTimeoutMaybe(), both
  // of which return true if they make a change.
  if (!JUST_STARTED) {
    performConfigurationTimeoutMaybe();
    processProgrammeSwitchMaybe();
  }

  // Before we transmit anything, let's do the NMEA housekeeping and
  // process any received messages.
  NMEA2000.ParseMessages();
  // The above call may have resulted in acquisition of a new source
  // address, so we check if there has been a change and if so save the
  // new address to EEPROM for future re-use.
  if (NMEA2000.ReadResetAddressChanged()) EEPROM.update(SOURCE_ADDRESS_EEPROM_ADDRESS, NMEA2000.GetN2kSource());

  // If the device isn't currently being programmed, then process
  // temperature sensors and transmit readings on N2K. 
  if ((!JUST_STARTED) && (MACHINE_STATE == NORMAL)) {
    processSensorsMaybe();
    processTransmitQueueMaybe();
  }

  // Update the states of connected LEDs
  LED_MANAGER.loop();
}

/**********************************************************************
 * processSensorsMaybe() should be called directly from loop(). The
 * function iterates through all sensors looking for any that have an
 * expired transmission. Sunch sensors have their temperature value
 * updated bt a read from the ADC and their index queued for subsequent
 * transmission on the N2K bus.
 */
void processSensorsMaybe() {
  unsigned long now = millis();

  for (unsigned int sensor = 0; sensor < ELEMENTCOUNT(SENSORS); sensor++) {
    if (SENSORS[sensor].getInstance() != 0xff) {
      if (now > SENSORS[sensor].getTransmissionDeadline()) {
        if (!TRANSMIT_QUEUE.isFull()) {
          int value = analogRead(SENSORS[sensor].getGpio());
          double kelvin = ((value * SENSOR_VOLTS_TO_KELVIN) / ANALOG_RESOLUTION) * 100;
          SENSORS[sensor].setTemperature(kelvin);
          #ifdef DEBUG_SERIAL
          Serial.println();
          Serial.print("Queueing reading for sensor "); Serial.print(sensor + 1); Serial.print(": ");
          Serial.print(kelvin - 273.0); Serial.print("C ");
          #endif
          TRANSMIT_QUEUE.enqueue(sensor);
        } else {
          #ifdef DEBUG_SERIAL
          Serial.println("ERROR: cannot queue sensor reading; transmit queue is full");
          #endif
        } 
        SENSORS[sensor].setTransmissionDeadline(now + SENSORS[sensor].getTransmissionInterval());
      }
    }
  }
  SID++;
}

/**********************************************************************
 * processProgrammeSwitchMaybe() should be called directly from loop().
 * The function uses an elapse timer to ensure that processing is only
 * invoked once every SWITCH_PROCESS_INTERVAL milliseconds.
 * 
 * The function will then checkpoint the debounced state of the switch
 * on the GPIO_PROGRAMME_SWITCH pin and, if it is active, will call
 * performMachineStateTransition() to modify the device's
 * MACHINE_STATE.
 */
void processProgrammeSwitchMaybe() {
  static unsigned long deadline = 0UL;
  unsigned long now = millis();
  if (now > deadline) {
    if (DEBOUNCER.channelState(GPIO_PROGRAMME_SWITCH) == 0) {
      DIL_SWITCH.sample();
      MACHINE_STATE = performMachineStateTransition(MACHINE_STATE);
    }
    deadline = (now + SWITCH_PROCESS_INTERVAL);
  }
}

/**********************************************************************
 * processTransmitQueue() should be called directly from loop. The
 * function uses an elapse timer to ensure that processing is only
 * invoked once every MINIMUM_TRANSMIT_CYCLE milliseconds. This value
 * should be set to the maximum transmit frequencyy for PGN130316.
 * 
 * Each time around the function removes a sensor index (if one is
 * available) from TRANSMIT_QUEUE and transmits the referenced sensor
 * data over the NMEA bus.
 */ 
void processTransmitQueueMaybe() {
  static unsigned long deadline = 0UL;
  unsigned long now = millis();

  if (now > deadline) {
    if (!TRANSMIT_QUEUE.isEmpty()) {
      int sensor = TRANSMIT_QUEUE.dequeue();
      #ifdef DEBUG_SERIAL
      Serial.println(); Serial.print("Dequeueing and transmitting sensor "); Serial.print(sensor + 1);
      #endif
      transmitPgn130316(SENSORS[sensor]);
    }
    deadline = (now + MINIMUM_TRANSMIT_CYCLE);
  }
}

/**********************************************************************
 * performConfigurationTimeoutMaybe() should be called directly from
 * loop(). The function uses an elapse timer to detect whether or not
 * module configuration should be cancelled because of an absence of
 * user input and the module returned to normal operation.
 */
void performConfigurationTimeoutMaybe() {
  if ((CONFIGURATION_TIMEOUT_COUNTER != 0UL) && (millis() > CONFIGURATION_TIMEOUT_COUNTER)) {
    MACHINE_STATE = performMachineStateTransition(CANCEL_CONFIGURATION);
  }
}

/**********************************************************************
 * cancelConfigurationTimeout() is a utility function that simply
 * zeroes CONFIGURATION_TIMEOUT_COUNTER so that a configuration
 * timeout processing is disabled. The function always returns a NORMAL
 * machine state value. 
 */
MACHINE_STATES cancelConfigurationTimeout() {
  CONFIGURATION_TIMEOUT_COUNTER = 0UL;
  return(NORMAL);
}

/**********************************************************************
 * extendConfigurationTimeout(state) is a utility function that simply
 * sets CONFIGURATION_TIMEOUT_COUNTER to its operational value. The
 * function always returns <state>.
 */
MACHINE_STATES extendConfigurationTimeout(MACHINE_STATES state) {
  CONFIGURATION_TIMEOUT_COUNTER = (millis() + CONFIG_TIMEOUT_INTERVAL);
  return(state);
}

/**********************************************************************
 * performMachineStateTransition(state) performs all of the processing
 * required to transition from <state> to some new machine state which
 * it returns as its result.
 * 
 * As a side effect, this function implements all of the UI associated
 * with configuration change and any consequent actions.
 */
MACHINE_STATES performMachineStateTransition(MACHINE_STATES state) {
  static int selectedSensorIndex = 0;
  unsigned char selectedValue = DIL_SWITCH.value();
  Sensor scratch;

  switch (state) {
    case NORMAL: // Start configuration process
      switch (selectedValue) {
        case 1 : case 2: case 3: case 4: case 5: case 6: case 7: case 8:
          selectedSensorIndex = selectedValue;
          state = extendConfigurationTimeout(CHANGE_CHANNEL_INSTANCE);
          LED_MANAGER.operate(GPIO_POWER_LED, 0, 1);
          LED_MANAGER.operate(GPIO_INSTANCE_LED, 0, -1);
          #ifdef DEBUG_SERIAL
          Serial.print("Starting configuration dialoge for channel "); Serial.println(selectedSensorIndex + 1);
          #endif
          break;
        case 32:
          // Dump configuration
          LED_MANAGER.operate(GPIO_POWER_LED, 0, 1);
          dumpSensorConfiguration();
          confirmDialogCompletion(1);
          break;
        case 64:
          // Transmit test outout on all channels
          LED_MANAGER.operate(GPIO_POWER_LED, 0, 1);
          for (unsigned int i = 0; i < ELEMENTCOUNT(SENSORS); i++) {
            scratch.setInstance(i); scratch.setSource(i); scratch.setSetPoint(i); scratch.setTemperature(i);
            transmitPgn130316(scratch, false);
            delay(MINIMUM_TRANSMIT_CYCLE);
            #ifdef DEBUG_SERIAL
            Serial.print("Transmitting test PGN130316 with instance "); Serial.println(i);
            #endif
          }
          break;
        case 128: 
          // Foctory reset - delete all channel configurations
          LED_MANAGER.operate(GPIO_POWER_LED, 0, 1);
          for (unsigned int i = 0; i < ELEMENTCOUNT(SENSORS); i++) {
            SENSORS[i].setInstance(255);
            SENSORS[i].save(SENSORS_EEPROM_ADDRESS + (i * SENSORS[i].getConfigSize()));
          }
          state = cancelConfigurationTimeout();
          #ifdef DEBUG_SERIAL
          Serial.println("Deleting all channel configurations");
          #endif
          break;
        default:
          LED_MANAGER.operate(GPIO_POWER_LED, 0, 1);
          // Unrecognised entry
          #ifdef DEBUG_SERIAL
          Serial.println("Ignoring invalid entry");
          #endif
          break;
      }
      break;
    case CHANGE_CHANNEL_INSTANCE:
      switch (selectedValue) {
        case 255:
          // Disable channel configuration
          LED_MANAGER.operate(GPIO_POWER_LED, 0, 1);
          SENSORS[selectedSensorIndex].setInstance(selectedValue);
          SENSORS[selectedSensorIndex].save(SENSORS_EEPROM_ADDRESS + (selectedSensorIndex * SENSORS[selectedSensorIndex].getConfigSize()));
          state = cancelConfigurationTimeout();
          #ifdef DEBUG_SERIAL
          Serial.print("Channel "); Serial.print(selectedSensorIndex + 1); Serial.print(": deleting configuration");
          dumpSensorConfiguration();
          #endif
          break;
        case 254: case 253:
          // Invalid entry
          LED_MANAGER.operate(GPIO_POWER_LED, 0, 2);
          state = extendConfigurationTimeout(CHANGE_CHANNEL_INSTANCE);
          #ifdef DEBUG_SERIAL
          Serial.print("Rejecting invalid temperature instance");
          #endif
          break;
        default:
          // Accept valid instance value
          if (!instanceInUse(selectedSensorIndex, selectedValue)) { 
            LED_MANAGER.operate(GPIO_POWER_LED, 0, 1);
            SENSORS[selectedSensorIndex].setInstance(selectedValue);
            state = extendConfigurationTimeout(CHANGE_CHANNEL_SOURCE);
            LED_MANAGER.operate(GPIO_INSTANCE_LED, 1);
            LED_MANAGER.operate(GPIO_SOURCE_LED, 0, -1);
            #ifdef DEBUG_SERIAL
            Serial.print("Channel "); Serial.print(selectedSensorIndex + 1); Serial.print(": temperature instance set to ");
            Serial.println(SENSORS[selectedSensorIndex].getInstance());
            #endif
          } else {
            LED_MANAGER.operate(GPIO_POWER_LED, 0, 3);
            state = extendConfigurationTimeout(CHANGE_CHANNEL_INSTANCE);
            #ifdef DEBUG_SERIAL
            Serial.print("Rejecting duplicate temperature instance");
            #endif
          }
          break;
      }
      break;
    case CHANGE_CHANNEL_SOURCE:
      if ((selectedValue <= 14) || ((selectedValue >= 129) && (selectedValue <= 252))) {
        LED_MANAGER.operate(GPIO_POWER_LED, 0, 1);
        SENSORS[selectedSensorIndex].setSource(selectedValue);
        state = extendConfigurationTimeout(CHANGE_CHANNEL_SETPOINT);
        LED_MANAGER.operate(GPIO_SOURCE_LED, 1);
        LED_MANAGER.operate(GPIO_SETPOINT_LED, 0, -1);
        #ifdef DEBUG_SERIAL
        Serial.print("Channel "); Serial.print(selectedSensorIndex + 1); Serial.print(": temperature source set to ");
        Serial.println(SENSORS[selectedSensorIndex].getSource());
        #endif
      } else {
        LED_MANAGER.operate(GPIO_POWER_LED, 0, 2);
        state = extendConfigurationTimeout(CHANGE_CHANNEL_SOURCE);
        #ifdef DEBUG_SERIAL
        Serial.print("Rejecting invalid temperature source");
        #endif
      }
      break;
    case CHANGE_CHANNEL_SETPOINT:
      LED_MANAGER.operate(GPIO_POWER_LED, 0, 1);
      SENSORS[selectedSensorIndex].setSetPoint((double) (selectedValue * 2));
      state = extendConfigurationTimeout(CHANGE_CHANNEL_INTERVAL);
      LED_MANAGER.operate(GPIO_SETPOINT_LED, 1);
      LED_MANAGER.operate(GPIO_INTERVAL_LED, 0, -1);
      #ifdef DEBUG_SERIAL
      Serial.print("Channel "); Serial.print(selectedSensorIndex + 1); Serial.print(": temperature set point set to ");
      Serial.println(SENSORS[selectedSensorIndex].getSetPoint());
      #endif
      break;
    case CHANGE_CHANNEL_INTERVAL:
      if (((unsigned long) 1000UL * selectedValue) >= MINIMUM_TRANSMIT_INTERVAL) {
        LED_MANAGER.operate(GPIO_POWER_LED, 0, 1);
        SENSORS[selectedSensorIndex].setTransmissionInterval((unsigned long) (selectedValue * 1000UL));
        SENSORS[selectedSensorIndex].save(SENSORS_EEPROM_ADDRESS + (selectedSensorIndex * SENSORS[selectedSensorIndex].getConfigSize()));
        state = cancelConfigurationTimeout();
        confirmDialogCompletion(1);
        #ifdef DEBUG_SERIAL
        Serial.print("Channel "); Serial.print(selectedSensorIndex + 1); Serial.print(": transmission interval set to ");
        Serial.println(SENSORS[selectedSensorIndex].getTransmissionInterval());
        Serial.print("Channel "); Serial.print(selectedSensorIndex + 1); Serial.println(": saving configuration");
        dumpSensorConfiguration();
        #endif
      } else {
        LED_MANAGER.operate(GPIO_POWER_LED, 0, 2);
        state = extendConfigurationTimeout(CHANGE_CHANNEL_INTERVAL);
        #ifdef DEBUG_SERIAL
        Serial.print("Rejecting invalid transmission interval");
        #endif
      }
      break;
    case CANCEL_CONFIGURATION:
      // Restore in-memory configuration from EEPROM and return to
      // normal operation.
      #ifdef DEBUG_SERIAL
      Serial.println("Discarding configuration changes, restoring previous configuration");
      dumpSensorConfiguration();
      #endif
      SENSORS[selectedSensorIndex].load(SENSORS_EEPROM_ADDRESS + (selectedSensorIndex * SENSORS[selectedSensorIndex].getConfigSize()));
      state = cancelConfigurationTimeout();
      confirmDialogCompletion(0);
      break;
    default:
      break;
  }
  return(state);
}

bool instanceInUse(unsigned int ignoreIndex, unsigned char instance) {
  bool retval = false;
  for (unsigned iny i = 0; i < ELEMENTCOUNT(SENSORS); i++) {
    if ((ignoreIndex != 255) && (i != ignoreIndex))
      if (SENSORS[i].getInstance() == instance)
        retval = true;
  }
  return(retval);
}

void confirmDialogCompletion(int flashes) {
      LED_MANAGER.operate(GPIO_INSTANCE_LED, 0, flashes);
      LED_MANAGER.operate(GPIO_SOURCE_LED, 0, flashes);
      LED_MANAGER.operate(GPIO_SETPOINT_LED, 0, flashes);
      LED_MANAGER.operate(GPIO_INTERVAL_LED, 0, flashes);
}

/**********************************************************************
 * Transmit the temperature data in <sensor> over the host NMEA bus and
 * flash the power LED to indicate it has been done. 
 */
void transmitPgn130316(Sensor sensor, bool flash) {
  tN2kMsg N2kMsg;
  SetN2kPGN130316(N2kMsg, SID, sensor.getInstance(), sensor.getSource(), sensor.getTemperature(), sensor.getSetPoint());
  NMEA2000.SendMsg(N2kMsg);
  if (flash) LED_MANAGER.operate(GPIO_POWER_LED, 0, 1);
}  

/**********************************************************************
 * Field an incoming NMEA message to our defined handler (there aren't
 * any!).
 */
void messageHandler(const tN2kMsg &N2kMsg) {
  int iHandler;
  for (iHandler=0; NMEA2000Handlers[iHandler].PGN!=0 && !(N2kMsg.PGN==NMEA2000Handlers[iHandler].PGN); iHandler++);
  if (NMEA2000Handlers[iHandler].PGN != 0) {
    NMEA2000Handlers[iHandler].Handler(N2kMsg); 
  }
}

#ifdef DEBUG_SERIAL

void dumpSensorConfiguration() {
  for (unsigned int i = 0; i < ELEMENTCOUNT(SENSORS); i++) {
    Serial.println();
    Serial.print("Sensor "); Serial.print(i + 1); Serial.print(": ");
    SENSORS[i].dump(Serial);
  }
}

#endif
