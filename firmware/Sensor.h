/**********************************************************************
 * Sensor.h - temperature sensor ADT.
 * 2021 (c) Paul Reeve <preeve@pdjr.eu>
 */

#ifndef SENSOR_H
#define SENSOR_H

class Sensor {
  public:
    Sensor();
    
    void setGpio(unsigned char gpio=0);
    void setInstance(unsigned char instance);
    void setSource(unsigned char source);
    void setSetPoint(double setPoint);
    void setTransmissionInterval(unsigned long transmissionInterval);
    void setTemperature(double temperature);
    void setTransmissionDeadline(unsigned long transmissionDeadline);
    
    unsigned char getGpio();
    unsigned char getInstance();
    unsigned char getSource();
    unsigned long getTransmissionInterval();
    double getSetPoint();
    double getTemperature();
    unsigned long getTransmissionDeadline();
    
    void invalidate(unsigned char gpio);
    void save(int eepromAddress);
    void load(int eepromAddress);
    int getConfigSize();

    void dump(Stream &port);

  private:
    struct Configuration {
      unsigned char gpio;
      unsigned char instance;
      unsigned char source;
      double setPoint;
      unsigned long transmissionInterval;
    };
    struct Sensor::Configuration config;
    double temperature;
    unsigned long transmissionDeadline;
};

#endif
