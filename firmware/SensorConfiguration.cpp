#include <cstdlib.h>
#include <EEPROM.h>
#include "ModuleConfiguration.h"

ModuleConfifuration::SensorConfifuration(unsigned int size, unsigned int eepromAddress, unsigned int startIndex) {
  this->size = size;
  this->eepromAddress = eepromAddress;
  this->startIndex = startIndex;
  this->configuration = (unsigned char *) malloc(size);
  for (int i = 0; i < sensorCount; i++) {
    this->configuration[i][0] = 0xff;
    this->configuration[i][1] = (i < 2)?0x03:((i < 4)?0x07:0x0D);
  }
}

void SensorConfiguration::setInstance(unsigned int sensorIndex, unsigned char instance) {
  if ((sensorIndex - this->startIndex) < this->sensorCount) {
    this->configuration[(sensorIndex - this->startIndex)][0] = instance;
  }
}

void SensorConfiguration::setSampleInterval(unsigned int sensorIndex, unsigned char sampleInterval) {
  if ((sensorIndex - this->startIndex) < this->sensorCount) {
    this->configuration[(sensorIndex - this->startIndex)][1] = sampleInterval;
  }  
}

unsigned char SensorConfiguration::getInstance(unsigned int sensorIndex) {
  if ((sensorIndex - this->startIndex) < this->sensorCount) {
    return(this->configuration[(sensorIndex - this->startIndex)][0]);
  }
  return(0xff);
}

unsigned char SensorConfiguration::getSampleInterval(unsigned int sensorIndex) {
  if ((sensorIndex - this->startIndex) < this->sensorCount) {
    return(this->configuration[(sensorIndex - this->startIndex)][1]);
  }
  return(0xff);
}

void SensorConfiguration::saveToEeprom(int address) {
  EEPROM.put(address, this->configuration);
}

void SensorConfiguration::loadFromEeprom(int address) {
  EEPROM.get(address, this->configuration);
}
