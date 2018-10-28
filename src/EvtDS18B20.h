#ifndef _EVTDS18B20_h
#define _EVTDS18B20_h

#include "EvtLogger.h"
#include <OneWire.h> // use the one for ESP32 here: https://github.com/stickbreaker/OneWire. See this thread: https://github.com/espressif/arduino-esp32/issues/755
#include <DallasTemperature.h>
#include <LinkedList.h>

#define TEMPERATURE_STACK_SIZE 5000
#define MAX_BUS_SEARCHES 5   // Number of times to probe a bus for sensors before giving up
#define BUS_SEARCH_RETRY_TIME 1000   // in ms



// Definition of the callback function that handles a new incoming temperature
typedef void(*TempCbFunc) (uint8_t pinNumber, uint8_t sensorIndex, char* sensorAddress, float temperature); 



// Each thermometer on the bus is controlled with this struct. 
// A linked list of this struct keeps track of all the thermometers on the bus that the class instance maintains.
struct Thermometer {
	uint8_t address[8];
	char addressStr[17] ="";
	float lastTemperature = 0;
};


struct BusSetup {
	uint8_t pinNumber;
	uint8_t precision;
	uint16_t fetchInterval;
	TempCbFunc callBackFunc;
	OneWire *oneWire;
	DallasTemperature *sensors;
	LinkedList<Thermometer*> thermometerList;   // A dynamic list of all the thermometers found on the bus
	unsigned long lastFetched = 0;
};


class EvtDS18B20 {
private:
	static LinkedList<BusSetup*> busList;
	static void TaskGetTemperature(void *pvParameters);

	uint8_t _pinNumber;
	uint8_t _precision;
	int _fetchInterval;
	TempCbFunc _callBackFunc = nullptr;
public:
	EvtDS18B20();
	bool addBus(uint8_t pinNumber, uint8_t precision, uint16_t fetchInterval, TempCbFunc callBackFunc);
};

#endif
