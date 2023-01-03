/**********************************************************************
 * OneWireAddress.h (c) Paul Reeve <preeve@pdjr.eu>
 * 
 * ADT representation of a 64-bit OneWire bus device address.
 *
 * CONSTRUCTOR
 * 
 * OneWireAddress()
 * Create a new OneWireAddress instance initialised to a default value
 * of 0xffffffffffffffff.
 *
 * METHODS
 *  
 * setAddress(address)
 * Set this OneWire address by copying eight bytes from the <address>
 * byte array.
 * 
 * getAddress()
 * Return a pointer to the OneWire address eight byte array.
 * 
 * clearAddress()
 * Set this OneWire address to 0xffffffffffffffff.
 * 
 * setByte(index, value)
 * Set byte index of this OneWire address to value.
 * 
 * getByte(index)
 * Return the indexth byte of this OneWire address.
 * 
 * compare(address)
 * Compare this OneWire address with address using the memcmp()
 * function.
 */

#ifndef ONE_WIRE_ADDRESS
#define ONE_WIRE_ADDRESS

class OneWireAddress {

  public:
    OneWireAddress();
    
    void setAddress(unsigned char *address);
    unsigned char *getAddress();
    void clearAddress();

    void setByte(int index, unsigned char value);
    unsigned char getByte(int index);

    int compare(OneWireAddress *address);

  private:
    unsigned char address[8];

};

#endif