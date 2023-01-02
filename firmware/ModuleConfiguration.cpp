#include <cstdlib.h>
#include <EEPROM.h>
#include "ModuleConfiguration.h"

ModuleConfiguration::ModuleConfiguration(unsigned int size, unsigned int eepromAddress) {
  this->size = size;
  this->eepromAddress = eepromAddress;
  this->startIndex = startIndex;
  this->configuration = (unsigned char *) malloc(size);
  for (int i = 0; i < this->size; i++) this->configuration[i] = 0xff;
}

void ModuleConfiguration::setByte(unsigned int index, unsigned char value) {
  if ((index - this->startIndex) < this->size) {
    this->configuration[(index - this->startIndex)] = value;
  }
}

unsigned char ModuleConfiguration::getByte(unsigned int index) {
  if ((index - this->startIndex) < this->size) {
    return(this->configuration[(index - this->startIndex)]);
  }
  return(0xff);
}

bool ModuleConfiguration::inUse() {
  bool retval = false;

  for (int i = 0; i < this->size; i++) if (this->configuration[i] != 0xff) retval = true;
  return(retval);
}

void ModuleConfiguration::save() {
  EEPROM.put(this->eepromAddress, this->configuration);
}

void ModuleConfiguration::load() {
  EEPROM.get(this->eepromAddress, this->configuration);
}
