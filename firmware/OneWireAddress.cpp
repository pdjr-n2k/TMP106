#include <string.h>
#include "OneWireAddress.h"

OneWireAddress::OneWireAddress() {
  this->clearAddress();
}

void OneWireAddress::setAddress(unsigned char *address) {
  memcpy(this->address, address, 8);
}

void OneWireAddress::clearAddress() {
  for (int i = 0; i < 8; i++) {
    this->address[i] = 0xff;
  }
}

unsigned char *OneWireAddress::getAddress() {
  return(this->address);
}

void OneWireAddress::setByte(int index, unsigned char value) {
  if ((index > 0) && (index < 8)) {
    this->address[index] = value;
  }
}

unsigned char OneWireAddress::getByte(int index) {
  return(((index >= 0) && (index < 8))?this->address[index]:0xff);
}

int OneWireAddress::compare(OneWireAddress *address) {
  return(memcmp(this->address, address, 8));
}
