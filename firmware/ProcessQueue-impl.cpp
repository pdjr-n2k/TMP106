#include <ProcessQueue.cpp>

struct TemperatureReading { unsigned int sensor; unsigned char sid; float temperature; };

template class ProcessQueue<TemperatureReading>;