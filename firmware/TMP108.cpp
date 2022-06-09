/**********************************************************************
 * tsense-1.0.0.cpp - TSENSE firmware version 1.0.0.
 * Copyright (c) 2021 Paul Reeve, <preeve@pdjr.eu>
 *
 * This firmware provides an 8-channel temperature senor interface
 * that reports sensor state over NMEA 2000 using PGN 130316
 * Temperature, Extended Range.
 * 
 * The firmware supports LM335Z sensors.
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
 * Define DEBUG_SERIAL to enable debug messages on serial output.
 */

#define DEBUG_SERIAL
#define DEBUG_SERIAL_START_DELAY 4000
#define DEBUG_SERIAL_INTERVAL 1000UL

/**********************************************************************
 * MCU EEPROM (PERSISTENT) STORAGE
 * 
 * SOURCE_ADDRESS_EEPROM_ADDRESS: storage address for the device's
 * 1-byte N2K source address.
 * SENSORS_EEPROM_ADDRESS: storage address for SENSOR congigurations.
 * The length of this is variable, so make sure it remains as the last
 * item.
 */

#define SOURCE_ADDRESS_EEPROM_ADDRESS 0
#define SENSORS_EEPROM_ADDRESS 1

/**********************************************************************
 * MCU PIN DEFINITIONS
 * 
 * GPIO pin definitions for the Teensy 3.2 MCU
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
#define GPIO_BOARD_LED 13
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
#define GPIO_OUTPUT_PINS { GPIO_BOARD_LED, GPIO_POWER_LED, GPIO_INSTANCE_LED, GPIO_SOURCE_LED, GPIO_SETPOINT_LED }
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
#define PRODUCT_FIRMWARE_VERSION "1.0.0 (Sep 2020)"
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

#define DEFAULT_SOURCE_ADDRESS 22         // Seed value for address claim
#define INSTANCE_UNDEFINED 255            // Flag value
#define STARTUP_SETTLE_PERIOD 5000        // Wait this many ms before processing switch inputs
#define SWITCH_PROCESS_INTERVAL 250       // Process switch inputs evety n ms
#define LED_MANAGER_HEARTBEAT 200         // Number of ms on / off
#define LED_MANAGER_INTERVAL 10           // Number of heartbeats between repeats
#define PROGRAMME_TIMEOUT_INTERVAL 20000  // Allow 20s to complete each programme step
#define TRANSMIT_QUEUE_PROCESS_INTERVAL 500  // Number of ms between possible N2K transmits
#define DEFAULT_TRANSMIT_INTERVAL 2000    // Sensor transmit interval
#define SENSOR_VOLTS_TO_KELVIN 3.3        // Conversion factor for LM335 temperature sensors
#define ANALOG_READ_AVAERAGE 10           // Number of ADC samples that average to on read value
#define ANALOG_RESOLUTION 1024            // ADC maximum return value
#define TRANSMIT_QUEUE_LENGTH 20          // Max number of entries in the transmit queue

/**********************************************************************
 * Declarations of local functions.
 */
#ifdef DEBUG_SERIAL
void dumpSensorConfiguration();
#endif
void messageHandler(const tN2kMsg&);
bool processProgrammeSwitchMaybe();
void processSensors();
bool processSwitches();
bool revertMachineStateMaybe();
void transmitPgn130316(Sensor sensor);
void processTransmitQueue();
void processMachineState();

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

/**********************************************************************
 * State machine definitions
 */
enum MACHINE_STATES { NORMAL, PRG_START, PRG_ACCEPT_INSTANCE, PRG_ACCEPT_SOURCE, PRG_ACCEPT_SETPOINT, PRG_FINALISE, PRG_CANCEL };
static MACHINE_STATES MACHINE_STATE = NORMAL;
unsigned long MACHINE_RESET_TIMER = 0UL;

/**********************************************************************
 * SID for clustering N2K messages by sensor process cycle.
 */
unsigned char SID = 0;

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

  analogReadAveraging(ANALOG_READ_AVAERAGE);

  // Initialise SENSORS array.
  for (unsigned int i = 0; i < ELEMENTCOUNT(SENSOR_PINS); i++) SENSORS[i].invalidate(SENSOR_PINS[i]); 
  
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
  LED_MANAGER.operate(GPIO_BOARD_LED, 0, n);

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
  // and calling processMachineState(). Changes to MACHINE_STATE are
  // only made by processSwitches() and revertMachineStateMaybe(), both
  // of which return true if they make a change.
  if (!JUST_STARTED) {
    if (processProgrammeSwitchMaybe() || revertMachineStateMaybe()) {
      DIL_SWITCH.sample();
      processMachineState();
    }
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
    processSensors();
    processTransmitQueue();
  }

  // Update the states of connected LEDs
  LED_MANAGER.loop();
}

/**********************************************************************
 * processSensors() should be called directly from loop(). The function
 * iterates through all sonsors. If it finds an enabled sensor whose
 * transmission interval has expired then it updates  the sensor
 * temperature from the ADC and queues the sensor for transmission on
 * the N2K bus.
 */
void processSensors() {
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
 * switchPressed() should be called directly from loop(). The function
 * uses a simple elapse timer to ensure that processing is only invoked
 * once every SWITCH_PROCESS_INTERVAL milliseconds.
 * 
 * The function will then recover the state of GPIO_PROGRAMME_SWITCH
 * from DEBOUNCER and if the switch is depressed the value of
 * MACHINE_STATE will be advanced and the function will return true.
 */
boolean processProgrammeSwitchMaybe() {
  static unsigned long deadline = 0UL;
  unsigned long now = millis();
  unsigned retval = false;
  if (now > deadline) {
    retval = (DEBOUNCER.channelState(GPIO_PROGRAMME_SWITCH) == 0);
    if (retval) {
      switch (MACHINE_STATE) {
        case NORMAL: MACHINE_STATE = PRG_START; break;
        case PRG_START: MACHINE_STATE = PRG_ACCEPT_INSTANCE; break;
        case PRG_ACCEPT_INSTANCE: MACHINE_STATE = PRG_ACCEPT_SOURCE; break;
        case PRG_ACCEPT_SOURCE: MACHINE_STATE = PRG_ACCEPT_SETPOINT; break;
        case PRG_ACCEPT_SETPOINT: MACHINE_STATE = PRG_FINALISE; break;
        default: break;
      }
    }
    deadline = (now + SWITCH_PROCESS_INTERVAL);
  }
  return(retval);
}

/**********************************************************************
 * processTransmitQueue() should be called directly from loop and will
 * seek to process a single item from the head of TRANSMIT_QUEUE once
 * every TRANSMIT_QUEUE_PROCESS_INTERVAL milliseconds.
 */ 
void processTransmitQueue() {
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
    deadline = (now + TRANSMIT_QUEUE_PROCESS_INTERVAL);
  }
}

/**********************************************************************
 * revertMachineStateMaybe() should be called directly from loop().
 * If the programming mode time window has elapsed because of user
 * inactivity, then MACHINE_STATE will be set to PRG_CANCEL and true
 * will be returned.
 */
boolean revertMachineStateMaybe() {
  boolean retval = false;
  if ((MACHINE_RESET_TIMER != 0UL) && (millis() > MACHINE_RESET_TIMER)) {
    MACHINE_STATE = PRG_CANCEL;
    retval = true;
  }
  return(retval);
}

/**********************************************************************
 * Should be called each time MACHINE_STATE is updated to implement any
 * necessary state change processing.
 */
void processMachineState() {
  static int selectedSensorIndex = -1;

  switch (MACHINE_STATE) {
    case NORMAL:
      break;
    case PRG_START:
      #ifdef DEBUG_SERIAL
      Serial.print("PRG_START: starting to programme sensor ");
      #endif
      if (DIL_SWITCH.selectedSwitch()) {
        selectedSensorIndex = (DIL_SWITCH.selectedSwitch() - 1);
        LED_MANAGER.operate(GPIO_INSTANCE_LED, 0, -1);
        MACHINE_RESET_TIMER = (millis() + PROGRAMME_TIMEOUT_INTERVAL);
        #ifdef DEBUG_SERIAL
        Serial.println(selectedSensorIndex + 1);
        #endif
      } else {
        MACHINE_STATE = NORMAL;
        MACHINE_RESET_TIMER = 0UL;
      }
      break;
    case PRG_ACCEPT_INSTANCE:
      #ifdef DEBUG_SERIAL
      Serial.print("PRG_ACCEPT_INSTANCE: assigning instance ");
      #endif
      SENSORS[selectedSensorIndex].setInstance(DIL_SWITCH.value());
      LED_MANAGER.operate(GPIO_INSTANCE_LED, 1);
      LED_MANAGER.operate(GPIO_SOURCE_LED, 0, -1);
      MACHINE_RESET_TIMER = (millis() + PROGRAMME_TIMEOUT_INTERVAL);
      #ifdef DEBUG_SERIAL
      Serial.println(SENSORS[selectedSensorIndex].getInstance());
      #endif
      break;
    case PRG_ACCEPT_SOURCE:
      #ifdef DEBUG_SERIAL
      Serial.print("PRG_ACCEPT_SOURCE: assigning source ");
      #endif
      SENSORS[selectedSensorIndex].setSource(DIL_SWITCH.value());
      LED_MANAGER.operate(GPIO_SOURCE_LED, 1);
      LED_MANAGER.operate(GPIO_SETPOINT_LED, 0, -1);
      MACHINE_RESET_TIMER = (millis() + PROGRAMME_TIMEOUT_INTERVAL);
      #ifdef DEBUG_SERIAL
      Serial.println(SENSORS[selectedSensorIndex].getSource());
      #endif
      break;
    case PRG_ACCEPT_SETPOINT:
      #ifdef DEBUG_SERIAL
      Serial.print("PRG_ACCEPT_SETPOINT: assigning set point ");
      #endif
      SENSORS[selectedSensorIndex].setSetPoint((double) (DIL_SWITCH.value() + 173));
      LED_MANAGER.operate(GPIO_SETPOINT_LED, 1);
      #ifdef DEBUG_SERIAL
      Serial.println(SENSORS[selectedSensorIndex].getSetPoint());
      #endif
    case PRG_FINALISE:
      // Save in-memory configuration to EEPROM, flash LEDs to confirm
      // programming and return to normal operation.
      #ifdef DEBUG_SERIAL
      Serial.println("PRG_FINALISE: saving new configuration");
      SENSORS[selectedSensorIndex].setTransmissionInterval(DEFAULT_TRANSMIT_INTERVAL);
      dumpSensorConfiguration();
      #endif
      SENSORS[selectedSensorIndex].save(SENSORS_EEPROM_ADDRESS + (selectedSensorIndex * SENSORS[selectedSensorIndex].getConfigSize()));
      MACHINE_STATE = NORMAL;
      MACHINE_RESET_TIMER = 0UL;
      LED_MANAGER.operate(GPIO_INSTANCE_LED, 0, 3);
      LED_MANAGER.operate(GPIO_SOURCE_LED, 0, 3);
      LED_MANAGER.operate(GPIO_SETPOINT_LED, 0, 3);
      break;
    case PRG_CANCEL:
      // Restore in-memory configuration from EEPROM and return to
      // normal operation.
      #ifdef DEBUG_SERIAL
      Serial.println("PRG_CANCEL: discarding changes, restoring previous configuration");
      dumpSensorConfiguration();
      #endif
      SENSORS[selectedSensorIndex].load(SENSORS_EEPROM_ADDRESS + (selectedSensorIndex * SENSORS[selectedSensorIndex].getConfigSize()));
      MACHINE_STATE = NORMAL;
      MACHINE_RESET_TIMER = 0UL;
      LED_MANAGER.operate(GPIO_INSTANCE_LED, 0);
      LED_MANAGER.operate(GPIO_SOURCE_LED, 0);
      LED_MANAGER.operate(GPIO_SETPOINT_LED, 0);
      break;
  }
}

/**********************************************************************
 * Return the integer value represented by the state of the digital
 * inputs passed in the <pins> array. Pin addresses are assumed to be
 * in the order lsb through msb.
 */
unsigned char getEncodedByte(int *pins) {
  unsigned char retval = 0x00;
  for (unsigned int i = 0; i < ELEMENTCOUNT(pins); i++) {
    retval = retval + (digitalRead(pins[i] << i));
  }
  return(retval);
}

/**********************************************************************
 * Transmit the temperature data in <sensor> over the host NMEA bus and
 * flash the power LED to indicate it has been done. 
 */
void transmitPgn130316(Sensor sensor) {
  tN2kMsg N2kMsg;
  SetN2kPGN130316(N2kMsg, SID, sensor.getInstance(), sensor.getSource(), sensor.getTemperature(), sensor.getSetPoint());
  NMEA2000.SendMsg(N2kMsg);
  LED_MANAGER.operate(GPIO_POWER_LED, 0, 1);
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
  Serial.println();
  for (unsigned int i = 0; i < ELEMENTCOUNT(SENSORS); i++) {
    Serial.println();
    Serial.print("Sensor "); Serial.print(i + 1); Serial.print(" ");
    Serial.print("(gpio "); Serial.print(SENSORS[i].getGpio()); Serial.print("): ");
    if (SENSORS[i].getInstance() == 0xFF) {
      Serial.print("disabled");
    } else {
      Serial.print("source: "); Serial.print(SENSORS[i].getSource()); Serial.print(", ");
      Serial.print("instance: "); Serial.print(SENSORS[i].getInstance()); Serial.print(", ");
      Serial.print("setPoint: "); Serial.print(SENSORS[i].getSetPoint());
    }
  }
  Serial.println();
}

#endif
