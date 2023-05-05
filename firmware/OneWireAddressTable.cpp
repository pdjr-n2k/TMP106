#include <cstdlib>
#include <Arduino.h>
#include <EEPROM.h>
#include "OneWireAddressTable.h"

OneWireAddressTable::OneWireAddressTable(unsigned int size, int eepromAddress) {
  this->size = size;
  this->eepromAddress = eepromAddress;
  this->table = new tOneWireAddress [this->size];

  for (unsigned int i = 0; i < this->size; i++) {
    this->table[i] = tOneWireAddress();
  }
}

void OneWireAddressTable::setAddress(unsigned int index, unsigned char *address) {
  if (index < this->size) this->table[index].setAddress(address);
}

void OneWireAddressTable::clearAddress(unsigned int index) {
  if (index < this->size) this->table[index].clearAddress();
}

unsigned char *OneWireAddressTable::getAddress(unsigned int index) {
  return((index < this->size)?this->table[index].getAddress():0);
}

bool OneWireAddressTable::contains(unsigned char *address) {
  bool retval = false;

  for (unsigned int i = 0; i < this->size; i++) {
    if (!memcmp(address, this->table[i].getAddress(), 8)) {
      retval = true;
    } 
  }
  return(retval);
}

void OneWireAddressTable::save() {
  if (this->eepromAddress != -1) EEPROM.put(this->eepromAddress, this->table);
}

void OneWireAddressTable::load() {
  if (this->eepromAddress != -1) EEPROM.get(this->eepromAddress, this->table);
}
