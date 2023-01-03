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