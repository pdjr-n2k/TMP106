
#ifndef ONE_WIRE_ADDRESS_TABLE
#define ONE_WIRE_ADDRESS_TABLE

class OneWireAddressTable {

  public:

    /******************************************************************
     * Create a new OneWireAddressTable with <tableSize> 64-bit entries
     * all bytes initialised to 0xff. <indexBase> specifies the offset
     * applied by the client application to the table base index.
     */
    OneWireAddressTable(int tableSize, int indexBase = 0);

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
     * <index> is out of range 
     */
    unsigned char *getAddress(unsigned int index);

    /******************************************************************
     * Return true if the table contains <address>. 
     */
    bool contains(unsigned char *address);
    
    /******************************************************************
     * Save the contents of the table to EEPROM starting at <address>.
     */
    void saveToEeprom(int address);
    
    /******************************************************************
     * Load the contents of EEPROM starting at <address> into the
     * table.
     */
    void loadFromEeprom(int address);

  private:
    int tableSize;
    int indexBase;
    unsigned char *table;

};

#endif