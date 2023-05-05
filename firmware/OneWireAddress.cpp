#include <string.h>
#include "OneWireAddress.h"

tOneWireAddress::tOneWireAddress(unsigned char *address) {
  if (address) {
    this->setAddress(address);
  } else {
    this->clearAddress();
  }
}

void tOneWireAddress::setAddress(unsigned char *address) {
  memcpy(this->address, address, 8);
}

void tOneWireAddress::clearAddress() {
  for (int i = 0; i < 8; i++) {
    this->address[i] = 0xff;
  }
}

unsigned char *tOneWireAddress::getAddress() {
  return(this->address);
}

void tOneWireAddress::setByte(int index, unsigned char value) {
  if ((index > 0) && (index < 8)) {
    this->address[index] = value;
  }
}

unsigned char tOneWireAddress::getByte(int index) {
  return(((index >= 0) && (index < 8))?this->address[index]:0xff);
}

int tOneWireAddress::compare(tOneWireAddress *address) {
  return(memcmp(this->address, address, 8));
}
