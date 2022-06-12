# Installing a TMP108 module

To install a TMP108

Field configuration of the module must be performed using the switches
mounted on the PCB.

Each sensor channel is configured independently and channels can be
configured before or after temperature sensor installation.

## Information required for configuring a channel

Before performing configuration of a sensor channel you need to know:

1. The number of the sensor you want to configure.

   *
   Sensors are numbered 1 through 8 and sensor numbers are printed on
   the PCB next to the temperature sensor connection terminals.
   *

2. The NMEA temperature instance address that you want to assign to
   your chosen sensor.

   *
   Values must be unique within the module and be in the range 0..252.
   The special value 255 can be used to delete an existing
   configuration for your chosen sensor, in which case no other
   configuration data need be supplied
   
   Unless there is a good reason not to, it makes sense to simply use
   the sensor number as the temperature instance address.
   *

3. The NMEA temperature source code (see table below).

   *
   Choose one of the defined codes in the range 0..15 or, pick your
   own code in the range 129 through 252.
   *

4. The set temperature for this sensor channel in degrees Kelvin.

   *
   The set temperature is used in some systems to trigger a thermostat
   or alarm. You can configure a set temperature in the range 0K
   through 510K in two degree increments, so choose an even number as
   your set temperature and divide it by two. Be aware that the
   operating range of your temperature sensor will be much narrower
   than these limits.

   If you don't need to use the sensor set temperature then it makes
   programming simpler if you just re-use the value chosen at (3).
   *

5. The rate at which you want to transmit this sensor's data over the
   NMEA bus in seconds.

   *
   This value must be in the range 2 through 255. Consider that most
   thermal systems change temperature relatively slowly so rapid
   reporting will just clutter the NMEA bus with unnecessary data.
   Note that the N2K specification sets limits on the rate the module
   can transmit and setting more than four sensors to a 2 second
   transmission rate will result in data loss.
   *

Items 2 through 5 must be encoded in binary. Here's an example:

| Item | Value | Binary | Comment |
|------|-------|--------|---------|
| Sensor number        |   1 | 00000001 | |  
| Temperature instance |   1 | 00000001 | |
| Temperature source   |   8 | 00001000 | Heating System Temperature |
| Set temperature      | 367 | 10110111 | 367 div 2 -> 183 | 
| Transmission rate    |   4 | 00000100 | Once every four seconds |

## Programming the module with the configuration





Temperature source = 4 (Heating System Temperature)



### Preparing for configuration

For each sensor recorded in the installation table on page NNN, insert
a value into the "Instance", "Source" and "Set-point" columns that
reflects your installation requirements and satisfies the following
constraints.

INSTANCE. The value you choose must be in the range 0 through 255 and
must be a unique identifier for each temperature sensor across the
vessel's entire NMEA installation.
The value 255 is used to indicate that a sensor connection is disabled.

SOURCE. The value you choose must be in the range 0 through 255 and
should be selected from the table included as "Annex 1: NMEA
temperature source codes".
Some standard values are defined in NMEA and will either be appropriate
or should be avoided.

SET-POINT. The value you choose must be in the range 0 through 255 and
should specify a set-point or alarm temperature in degrees Celsius.
Value below 100 are considered to be below zero Celsius, values above
100 are considered to be above zero Celsius and the value 100 specifies
0 Celsius.
Thus, the value 53 says "-53 Celsius", 147 says "+47 Celsius".

### Programming the module

The values you have defined above are programmed into the module by
setting up the required value as a binary-encoded number on the module
DIL switch and then pressing PRG to commit the DIL switch value to
the module configuration.

The programming sequence is: (1) select sensor to be programmed; (2)
enter instance; (3) enter state; (4) enter set-point.

Let us imagine we have the following temperature sensor settings:

Sensor id: 0
Instance:  0
Source:    11 (Gearbox temperature)
Set point: 85C

We begin programming by selecting the sensor we wish to configure on
the DIL switch.
The left-most switch corresponds to sensor 0, the right-most switch
to sensor 7.
We are programming sensor 0, so we set the DIL switch to [ON-OFF-OFF-OFF-OFF-OFF-OFF-OFF]
and then press PRG.

If our entry is accepted then the INST LED will flash to indicate that
the module is awaiting entry of a sensor instance value.
At this point, we have 20 seconds in which to set up the requested
instance value on the DIL switch and press PRG to confirm the entry.

We are setting an instance value of 0, so we set the DIL switch to
[OFF-OFF-OFF-OFF-OFF-OFF-OFF-OFF] and press PRG.

If our entry is accepted, then the INST LED will become steadily
illuminated and the SRCE LED will begin to flash, requesting entry of a
source value within a new 20s timeout period.

We are setting a source value of 11, so we set the DIL switch to
[OFF-OFF-OFF-OFF-ON-OFF-ON-ON] and press PRG.

If our entry is accepted then the SRCE LED will become steadily
illuminated and the SETP LED will begin to flash, requesting entry of a
set-point / alarm value within a new 20s timeout period.

We are setting a set-point value of 83, so we set the DIL switch to
[OFF-ON-OFF-ON-OFF-ON-OFF-ON] and press PRG.

If our entry is accepted then the INST, SRCE and SETP LEDs will flash
three times to indicate that all enries have been accepted and the
configuration has been saved to the module's EEPROM.

The protocol described above can be repeated to programme another
sensor channel or to amend an incorrect or inappropriate existing
entry.

If an error is made in the programming sequence or the 20s timeout
expires before an entry is made, then the all LEDS will extinquish and
the programming sequence must be restarted with the selection of the
sensor to be programmed.