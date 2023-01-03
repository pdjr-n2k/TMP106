#include <cstdlib>
#include <Arduino.h>
#include <EEPROM.h>
#include "ModuleConfiguration.h"

ModuleConfiguration::ModuleConfiguration(unsigned int size, unsigned int eepromAddress) {
  this->size = size;
  this->eepromAddress = eepromAddress;
  this->configuration = (unsigned char *) malloc(size);
  for (unsigned int i = 0; i < this->size; i++) this->configuration[i] = 0xff;
}

void ModuleConfiguration::setByte(unsigned int index, unsigned char value) {
  if (index < this->size) {
    this->configuration[index] = value;
  }
}

unsigned char ModuleConfiguration::getByte(unsigned int index) {
  if (index < this->size) {
    return(this->configuration[index]);
  }
  return(0xff);
}

bool ModuleConfiguration::inUse() {
  bool retval = false;

  for (unsigned int i = 0; i < this->size; i++) if (this->configuration[i] != 0xff) retval = true;
  return(retval);
}

void ModuleConfiguration::save() {
  EEPROM.put(this->eepromAddress, this->configuration);
}

void ModuleConfiguration::load() {
  EEPROM.get(this->eepromAddress, this->configuration);
}
