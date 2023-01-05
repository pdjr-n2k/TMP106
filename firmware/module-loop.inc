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
sampleSensorMaybe();

/**********************************************************************
 * Process the transmit queue transmitting PGN 130316 messages for any
 * queued temperature readings.
 */
TEMPERATURE_READING_PROCESS_QUEUE.process();
