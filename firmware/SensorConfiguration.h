#ifndef MODULE_CONFIGURATION
#define MODULE_CONFGURATION

class ModuleConfiguration {

  public:
    ModuleConfiguration(unsigned int size, unsigned int eepromAddress = 0, unsigned int startIndex = 0);
    void setByte(unsigned int index, unsigned char value);
    unsigned char getByte(unsigned int index);

    void save();
    void load();

  private:
    unsigned int size;
    unsigned int eepromAddress;
    unsigned int startIndex;
    unsigned char *configuration;
    
};

#endif