/**********************************************************************
 * OneWireAddress.h
 * Paul Reeve <preeve@pdjr.eu>
 */ 
 
#ifndef ONE_WIRE_ADDRESS
#define ONE_WIRE_ADDRESS

/**
 * @brief ADT representing a OneWire device address. 
 */
class tOneWireAddress {

  public:

    /**
     * @brief Construct a new tOneWireAddress object.
     * 
     * If @ref address is not supplied, then the newly instantiated
     * address is set to [ 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff ].
     * 
     * @param address - byte array used to specify an initial address.
     */
    tOneWireAddress(unsigned char *address = 0);
    
    /**
     * @brief Set the address object to a specified value.
     * 
     * @param address  - byte array used to specify an initial address.
     */
    void setAddress(unsigned char *address);

    /**
     * @brief Get the address object value. 
     * 
     * @return unsigned char*  - pointer to the address value as an
     * eight byte array.
     */
    unsigned char *getAddress();
    
    /**
     * @brief Clear the address object the default value.
     */
    void clearAddress();

    /**
     * @brief Set a specified byte of the address to a specified value.
     * 
     * @param index - index of the byte to be written.
     * @param value - the value to be assigned.
     */
    void setByte(int index, unsigned char value);

    /**
     * @brief Get a specified byte from the address.
     * 
     * @param index - index of the byte to be retrieved.
     * @return unsigned char - the value the specified byte.
     */
    unsigned char getByte(int index);

    /**
     * @brief Compare two OneWire address objects.
     * 
     * @param address - the address to be compared.
     * @return int - 0 if the addresses are equal.
     */
    int compare(tOneWireAddress *address);

  private:
    unsigned char address[8];

};

#endif