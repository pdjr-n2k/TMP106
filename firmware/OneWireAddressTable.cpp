#include <cstdlib.h>
#include <EEPROM.h>
#include "OneWireAddressTable.h"

OneWireAddressTable::OneWireAddressTable(unsigned int size, int indexBase) {
  this->tableSize = size;
  this->indexBase = indexBase;
  this->table = (unsigned char *) malloc(sizeof(unsigned char) * size);

  for (int i = 0; i < this->tableSize; i++) {
    this->clearAddress(i + this->indexBase);
  }
}

void OneWireAddressTable::setAddress(unsigned int index, unsigned char *address) {
  if ((index - this->indexBase) < this->tableSize) {
    for (int i = 0; i < 8; i++) this->table[(index - this->indexBase)][i] = address[i];
  }
}

void OnewWireAddressTable::clearAddress(unsigned int index) {
  if ((index - this->indexBase) < this->tableSize) {
    for (int i = 0; i < 8; i++) this->table[(index - this.indexBase)][i] = 0xff;
  }
}

unsigned char *OneWireAddressTable::getAddress(unsigned int index) {
  if ((index - this->indexBase) < this->tableSize ) {
    return(this->table[(index - this->indexBase)]);
  }
  return(0);
}

bool OneWireAddressTable::contains(unsigned char *address) {
  bool retval = false;

  for (int i = 0; i < this->tableSize; i++) {
    if (!memcmp(address, this->table[i], 8)) {
      retval = true;
    } 
  }
}

void OneWireAddressTable::saveToEeprom(int address) {
  EEPROM.put(address, this->table);
}

void OneWireAddressTable::loadFromEeprom(int address) {
  EEPROM.get(address, this->table);
}
