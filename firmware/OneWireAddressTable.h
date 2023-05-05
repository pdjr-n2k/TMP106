#ifndef ONE_WIRE_ADDRESS_TABLE
#define ONE_WIRE_ADDRESS_TABLE

#include "OneWireAddress.h"

class OneWireAddressTable {

  public:

    /******************************************************************
     * Create a new OneWireAddressTable with size 64-bit entries and
     * initialise all bytes to 0xff. A persistent copy should be saved
     * to EEPROM at eepromAddress.
     */
    OneWireAddressTable(unsigned int size, int eepromAddress = -1);

    /******************************************************************
     * Copy the 8-byte <address> to this address table at entry
     * position <index>.
     */
    void setAddress(unsigned int index, unsigned char *address);

    /******************************************************************
     * Clear the 8-byte address at table position <index> by setting
     * all bytes to 0xff.
     */
    void clearAddress(unsigned int index);

    /******************************************************************
     * Return a pointer to address at table position <index> or 0 if
     * <index> is out of range or address is invalid.
     */
    unsigned char *getAddress(unsigned int index);

    /******************************************************************
     * Return true if the table contains <address>. 
     */
    bool contains(unsigned char *address);
    
    /******************************************************************
     * Save the contents of the table to EEPROM.
     */
    void save();
    
    /******************************************************************
     * Load the contents of the table from EEPROM.
     */
    void load();

  private:
    unsigned int size;
    int eepromAddress;
    tOneWireAddress *table;

};

#endif