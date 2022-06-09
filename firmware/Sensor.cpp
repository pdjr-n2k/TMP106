/**********************************************************************
 * Sensor.cpp - temperature sensor ADT.
 * 2021 (c) Paul Reeve <preeve@pdjr.eu>
 */

#include <cstddef>
#include <EEPROM.h>
#include <Sensor.h>

Sensor::Sensor() {
  this->config.gpio = 0x00;               // Set default GPIO
  this->config.instance = 0xff;           // Set default instance to undefined as per N2K
  this->config.source = 0xff;             // Set default source to undefined as per N2K
  this->config.setPoint = 0.0;            // Set default set point to minimum
  this->config.transmissionInterval = 2000UL; // Set default transmission rate to 4 seconds
  this->temperature = 0.0;
}

unsigned char Sensor::getGpio() {
  return(this->config.gpio);
}

unsigned char Sensor::getInstance() {
  return(this->config.instance);
}

unsigned char Sensor::getSource() {
  return(this->config.source);
}

double Sensor::getSetPoint() {
  return(this->config.setPoint);
}

double Sensor::getTemperature() {
  return(this->temperature);
}

unsigned long Sensor::getTransmissionInterval() {
  return(this->config.transmissionInterval);
}

unsigned long Sensor::getTransmissionDeadline() {
  return(this->config.transmissionDeadline);
}

void Sensor::setGpio(unsigned char gpio) {
  this->config.gpio = gpio;
}

void Sensor::setInstance(unsigned char instance) {
  this->config.instance = instance;
}

void Sensor::setSource(unsigned char source) {
  this->config.source = source;
}

void Sensor::setSetPoint(double setPoint) {
  this->config.setPoint = setPoint;
}

void Sensor::setTemperature(double temperature) {
  this->temperature = temperature;
}

void Sensor::setTransmissionInterval(unsigned long transmissionInterval) {
  this->config.transmissionInterval = transmissionInterval;
}

void Sensor::setTransmissionDeadline(unsigned long transmissionDeadline) {
  this->config.transmissionDeadline = transmissionDeadline;
}

void Sensor::invalidate(unsigned char gpio) {
  this->config = { gpio, 0xff, 0xff, 0.0 };
  this->temperature = 0.0;
}

void Sensor::save(int eepromAddress) {
  EEPROM.put(eepromAddress, this->config);
}

void Sensor::load(int eepromAddress) {
  EEPROM.get(eepromAddress, this->config);
}

int Sensor::getConfigSize() {
  return(sizeof this->config);
}
