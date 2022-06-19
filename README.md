# TMP108 - NMEA 2000 temperature sensor module

**TMP108** is an NMEA 2000 sensor module that supports the connection
of up to eight
[National Semiconductor LM335Z](https://datasheet.octopart.com/LM335Z-NOPB-Texas-Instruments-datasheet-7836729.pdf)
temperature sensors.

Readings from connected sensors are transmitted over NMEA 2000 using 
[PGN 130316 Temperature, Extended Range](https://www.nmea.org/Assets/nmea%202000%20pgn%20130316%20corrigenda%20nmd%20version%202.100%20feb%202015.pdf).

Multiple __TMP108__ modules can be installed on a single NMEA bus.

## State of development

A complete, functional, implementation is available as
[TMP108.2]().
Key features of the design/implementation are: 

1. Easy bus connection by a standard M12 5-pin plug.
2. Installer selectable 120 Ohm termination resistor allows
   connection as either a bus drop or a bus termination node.
3. Powered directly from the NMEA bus with an LEN of 1.
4. Supports home-brew and commercially available LM335Z temperature
   sensors.
5. Operating status indicated by externally visible LED.
6. Fully field configurable through a simple DIL-switch based
   configuration protocol.
7. Easy assembly afforded by PCB with well marked component
   locations and 100% through-hole mounting.
8. Support for remote configuration of the module is not currently
   available, but is a work in progress.

## Hardware

### PCB

The module PCB is a 75mm x 75mm square. 

![Fig 2: PCB layout](images/TMP108.2-brd.svg)

### Electronic components

| Component   | Description                                     | Further information
|------------ |------------------------------------------------ |--------------------- |
| C1          | 1000uF aluminium capacitor                      | [711-1148](https://uk.rs-online.com/web/p/aluminium-capacitors/7111148)
| C2,C3       | 100nF ceramic capacitor]                        | [538-1427](https://uk.rs-online.com/web/p/mlccs-multilayer-ceramic-capacitors/5381427)
| D1,D2,D3,D4 | 2V 1.8mm rectangular LED                        | [229-2425](https://uk.rs-online.com/web/p/leds/2292425)
| D5          | 2V 3.0mm circular LED                           | [228-5916](https://uk.rs-online.com/web/p/leds/2285916)
| F1          | ECE BU135 1.35A polymer fuse                    | [ECE](https://www.ece.com.tw/images/cgcustom/file020170930043926.pdf)
| J1,J2       | Phoenix Contact FK-MPT terminal block 1x8 3.5mm | [229-2425](https://uk.rs-online.com/web/p/pcb-terminal-blocks/8020169)
| J3          | Phoenix Contact MPT terminal block 1x5 2.54"    | [220-4298](https://uk.rs-online.com/web/p/pcb-terminal-blocks/2204298)
| R1,R10-R13  | 390R 0.25W resistor                             | [707-7634](https://uk.rs-online.com/web/p/through-hole-resistors/7077634)
| R3-R9       | 2K2 0.25W resistor                              | [707-7690](https://uk.rs-online.com/web/p/through-hole-resistors/7077690)
| R14         | 120R 0.25W resistor                             | [707-7599](https://uk.rs-online.com/web/p/through-hole-resistors/7077599)
| SW1         | 6mm momentary push button                       | Sourced from eBay
| SW2         | 2-way SPST DIP switch                           | [177-4261](https://uk.rs-online.com/web/p/dip-sip-switches/1774261)
| SW3         | 8-way SPST DIP switch                           | [756-1347](https://uk.rs-online.com/web/p/dip-sip-switches/7561347)
| U1          | PJRC Teensy 3.2 MCU                             | [PJRC](https://www.pjrc.com/store/teensy32.html)
| U2          | TracoPower TMR-1-1211 DC-DC converter           | [781-3190](https://uk.rs-online.com/web/p/dc-dc-converters/7813190)
| U3          | MCP2551-I/P CAN transceiver                     | [040-2920](https://uk.rs-online.com/web/p/can-interface-ics/0402920)
| SENSORS     | LM335Z - if you choose to make your own sensors | [159-4685](https://uk.rs-online.com/web/p/temperature-humidity-sensor-ics/1594685)

### Suggested hardware

| Component   | Description                                     | Further information
|------------ |------------------------------------------------ |--------------------- |
| ENCLOSURE   | Plastic, general purpose, flange mount box      | [919-0391](https://uk.rs-online.com/web/p/general-purpose-enclosures/9190391)
| J4          | M12 5-pin male NMEA bus connector               | [877-1154](https://uk.rs-online.com/web/p/industrial-circular-connectors/8771154)
| CLIP        | 3mm LED panel clip                              | Sourced from eBay

### Assembly

All components need to be placed and soldered with care taken to
ensure correct orientation and polarity.

The host NMEA bus can be wired directly to J3 or (and preferably)
ENCLOSURE can be drilled to accommodate J4 and J4's flying leads
connected to J3.
A good position for J4 is directly over the bottom right-hand PCB
mounting screw.

D5 can be soldered with long leads and a hole drilled in the
ENCLOSURE to expose the LED or (and preferably) preferably, D5 can
be mounted with CLIP to ENCLOSURE and trailing leads used to
connect D5 back to the PCB mounting location.
The latter approach means that exact positioning of the hole which
exposes the PCB mounted LED is not required except, of course, that
the LED must not foul a PCB component or cable path.

## Module configuration

It will almost always be simpler to configure the module on the bench
and then install it in its normal operating location.

Begin configuration by exposing the module PCB.

Make sure that the module is properly terminated for its method of
connection to the NMEA bus by performing any required hardware
configuration (see below).

Connect the module to the host NMEA bus: the module will boot and the
PWR LED will flash once.

You can now continue with firmware configuration.

### Hardware configuration

The BUS switch labelled 'T' allows a 120 Ohm terminating resistor to be
connected across the NMEA data bus.
Switch BUS.T OFF if you install the module via a T-connector and drop
cable or switch it ON if you install the module as a terminating device
on your NMEA bus backbone.

The BUS switch lagelled 'G' connects the NMEA bus shield to the module
GND when it is in the ON position.
Usually it is appropriate to leave this OFF.

### Firmware configuration

The module is configured by the application of one or more *protocol*s
each of which defines a sequence of one or more *step*s that are
required to perform a single configuration task.
A step involves setting up a configuration parameter on the PRG-VALUE
DIL switch and then entering it by briefly pressing the PRG button.

In a multi-step protocol after pressing PRG, you have 20 seconds to
complete the subsequent step, otherwise the protocol is abandoned and
the module will revert to normal operation.

The PWR LED and the user-interaction LEDs (INST, SRCE, SETP and IVAL
help guide you through the multi-step protocols that relate to channel
configuration.

The PWR LED will flash one or more times after you press the PRG button.
A single flash means that your entry has been successfuly validated or
processed whilst more than one flash indicates that you entered an
invalid value on PRG-VALUE.
In the latter case you should correct the error by setting a new value
on PRG-VALUE and pressing PRG again to re-validate your entry.

The user-interaction LEDs flash to indicate whan a particular value should
be entered and become steady when an entry has been accepted.
When a protocol is successfully completed all four leds will flash
together.

#### PROTOCOL 128: Clear Module EEPROM

This single step protocol deletes all existing module settings, essentially
performing a 'factory reset'.

| PRG-VALUE  | DIL switch |Description |
|------------|------------|------------|
| 128        | [10000000] | Clear module EEPROM (deleting all channel configuration) |

#### PROTOCOL 64: Transmit Test Messages

This is a single step protocol which transmits a single, dummy, PGN
130316 message for each sensor channel.
This is especially helpful when commissioning a new installation
since it allows you to use an NMEA monitor application or instrument
to confirm that the module and its NMEA bus connection are operating
without having to perform any channel configuration.

| PRG-VALUE  | DIL switch |Description |
|------------|------------|------------|
| 64         | [01000000] | Transmit a single, dummy, PGN 130316 for each channel |

#### PROTOCOL 1..8: Delete Sensor Channel

This two-step protocol deletes any existing configuration for a
specified channel.

| PRG-VALUE  | DIL switch |Description |
|------------|------------|------------|
| 1..8       | [0000XXXX] | Number of the sensor channel that should be deleted. |
| 255        | [11111111] | Delete sensor channel configuration. |

#### PROTOCOL 1..8: Configure Sensor Channel

This five-step protocol configures a sensor channel and enables its
transmission on the NMEA bus.

Its a good idea to refer to
[this NMEA document](https://www.nmea.org/Assets/nmea%202000%20pgn%20130316%20corrigenda%20nmd%20version%202.100%20feb%202015.pdf)
which discusses the values you will need to enter.

| PRG-VALUE       | DIL switch |Description |
|-----------------|------------|------------|
| 1..8            | [0000XXXX] | Number of the sensor channel to be configured. |
| 0..252          | [XXXXXXXX] | NMEA temperature instance. |
| 0..14, 129..252 | [XXXXXXXX] | NMEA temperature source. |
| 1..255          | [XXXXXXXX] | NMEA temperature set point divided by 2. |
| 2..255          | [XXXXXXXX] | Transmission interval in seconds. |

Some things to consider:

1. It is usually sensible to set the temperature instance value to the
   sensor channel number, so STEP 2 simply becomes a press of PRG.

2. The temperature set point is expressed in degrees Kelvin divided by
   two, so to configure a set point for 100C you need to enter
   (100 + 273) / 2 or 186 [10111010] (actually setting represents 99C).

3. Consider reducing the transmission rate of a sensor as much as seems
   reasonable and so avoid consuming bus bandwidth unnecessarily.
   Usually, real world temperature values change slowly.

   Compliance with the NMEA specification means that no more than three
   sensor channels can be used at the minimum transmission interval of
   2s without consequent data loss.

## Temperature sensors

The TMP108 module supports a maximum of eight
[LM335Z](https://www.st.com/resource/en/datasheet/lm335.pdf)
temperature sensor ICs.
Other types of temperature sensor cannot be used and connecting them
to a TMP108 module will almost certainly damage the module beyond
repair.

LM335Z-based temperature sensors packaged for marine use are
commercially available, but you can easily (and inexpensively) make
your own sensor and one method I have used is described below.

### Making a bolt-on temperature sensor

You will require an LM335Z IC in a TO-92 package, a length of two-core
cable with 0.5mm2 colour-coded conductors (two-core telephone cable
works well), a minimum 10mm2 ring terminal with a hole size that suits
your mounting needs and some silicone sealer or other potting compound.

Consult the data sheet referenced above to determine the pin layout of
your sensor IC.

1. Remove the calibrate pin from the LM335Z by cutting or breaking it
   off as close to the IC body as possible.

2. Solder an appropriate length of two-core cable to IC pins G and P.
   Make sure you can identify which pin connects to which cable core!
   Insulate the exposed connection is some way, perhaps using small
   diameter heat-shrink sleeving around each connection.

3. Fill the cable-entry port of the ring terminal with silicone-sealant
   and coat the LM335Z with sealant as well.
   Try to avoid air bubbles.

4. Fully immerse the LM335Z in the ring-terminal cable-entry port so
   that the electrical connections are completely embedded in the
   sealant and making sure that they do not touch the ring-terminal
   body.

5. Allow the sealant to fully cure.

Ordinary silicone-sealant and potting compounds are not wonderful
conductors of heat so it is good to keep the volume of these low.
Thermally conductive silicone-sealant and potting compounds perform
better, but are expensive.


