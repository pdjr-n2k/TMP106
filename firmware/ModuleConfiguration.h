#ifndef MODULE_CONFIGURATION
#define MODULE_CONFGURATION

class ModuleConfiguration {

  public:
    ModuleConfiguration(unsigned int size, unsigned int eepromAddress = 0, void (*callback)(unsigned int, unsigned char) = 0);
    void setByte(unsigned int index, unsigned char value);
    unsigned char getByte(unsigned int index);

    bool ModuleConfiguration::interact(int value, bool longPress);

    bool inUse();

    void save();
    void load();

  private:
    unsigned int size;
    unsigned int eepromAddress;
    void (*callback)(unsigned int address, unsigned char value);
  
    unsigned char *configuration;
    
};

#endif