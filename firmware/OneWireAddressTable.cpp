#include <cstdlib>
#include <Arduino.h>
#include <EEPROM.h>
#include "OneWireAddressTable.h"

OneWireAddressTable::OneWireAddressTable(unsigned int size, unsigned int indexBase) {
  this->tableSize = size;
  this->indexBase = indexBase;
  this->table = new OneWireAddress [this->tableSize];

  for (unsigned int i = 0; i < this->tableSize; i++) {
    this->table[i] = OneWireAddress();
  }
}

void OneWireAddressTable::setAddress(unsigned int index, unsigned char *address) {
  if ((index - this->indexBase) < this->tableSize) {
    for (int i = 0; i < 8; i++) {
      this->table[((index - this->indexBase) + i)].setAddress(address);
    }
  }
}

void OneWireAddressTable::clearAddress(unsigned int index) {
  if ((index - this->indexBase) < this->tableSize) {
    for (unsigned int i = 0; i < 8; i++) this->table[((index - this->indexBase) + i)].clearAddress();
  }
}

unsigned char *OneWireAddressTable::getAddress(unsigned int index) {
  if ((index - this->indexBase) < this->tableSize ) {
    if (this->table[(index - this->indexBase)].getByte(0) != 0xff) {
      return(this->table[(index - this->indexBase)].getAddress());
    }
  }
  return(0);
}

bool OneWireAddressTable::contains(unsigned char *address) {
  bool retval = false;

  for (unsigned int i = 0; i < this->tableSize; i++) {
    if (!memcmp(address, this->table[i].getAddress(), 8)) {
      retval = true;
    } 
  }
  return(retval);
}

void OneWireAddressTable::saveToEeprom(int address) {
  EEPROM.put(address, this->table);
}

void OneWireAddressTable::loadFromEeprom(int address) {
  EEPROM.get(address, this->table);
}
