
#ifndef ONE_WIRE_ADDRESS_TABLE
#define ONE_WIRE_ADDRESS_TABLE

class OneWireAddressTable {

  public:
    OneWireAddressTable(int tableSize);

    void setAddress(int index, unsigned char *address);
    void clearAddress(int index);
    unsigned char *getAddress(int index);
    bool contains(unsigned char *address);
    
    void saveToEeprom(int address);
    void loadFromEeprom(int address);

  private:
    int tableSize;
    unsigned char *table;

};

#endif