/**********************************************************************
 * module-loop.inc
 *
 * The contents of this file are included into the body of the host
 * firmware loop() function.
 * 
 * For the NOP100 module this file is intentionally empty.
 */

// Sample the sensors and perhaps queue temperature reports for output.
sampleSensorMaybe();
