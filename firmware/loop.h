/**********************************************************************
 * module-loop.inc
 *
 * The contents of this file are included into the body of the host
 * firmware loop() function.
 * 
 * For the NOP100 module this file is intentionally empty.
 */

/**********************************************************************
 * Sample the sensors and perhaps queue temperature reports for future
 * output.
 */
sampleSensorsMaybe();

for (unsigned int i = 0; i < NUMBER_OF_SUPPORTED_SENSORS; i++) {
  if (PGN130316Schedulers[i].IsTime()) { PGN130316Schedulers[i].UpdateNextTime(); transmitPGN130316(i); }
}
