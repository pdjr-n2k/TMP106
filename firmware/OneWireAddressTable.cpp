#include <cstdlib.h>
#include "OneWireAddressTable.h"

OneWireAddressTable::OneWireAddressTable(unsigned int size) {
  this->tableSize = size;
  this->table = (unsigned char *) malloc(sizeof(unsigned char) * size);

  for (int i = 0; i < this->tableSize; i++) {
    for (int j = 0; j < 8; j++) {
      this->table[i][j] = 0xff;
    }
  }
}

void OneWireAddressTable::setAddress(unsigned int index, unsigned char *address) {
  if (index < this->tableSize) {
    for (int i = 0; i < 8; i++) this->table[index][i] = address[i];
  }
}

void OnewWireAddressTable::clearAddress(unsigned int) {
  if (index < this->tableSize) {
    for (int i = 0; i < 8; i++) this->table[index][i] = 0xff;
  }
}

unsigned char *OneWireAddressTable::getAddress(int index) {
  if (index < this->tableSize ) {
    return(this->table[index]);
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
