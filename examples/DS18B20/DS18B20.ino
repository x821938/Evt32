#include "EvtLogger.h"
#include "EvtDS18B20.h"

EvtDS18B20 evtDS18B20;   // Our temperature sensor class instance

#define BUS_A 23   // on IO pin 23
#define BUS_B 22   // on IO pin 22


void setup(void)
{
	logger.setup(INFO, false);   // We don't want to much logging

	/*	We add 2 busses where we have connected at least one Dallas ds18b20 sensor to each.
		We want a resolution of 12 bits (this means it will take about 700ms to read )
		We want a reading every 5 seconds on each bus
	*/
	evtDS18B20.addBus(BUS_A, 12, 5, cbTemperature);
	evtDS18B20.addBus(BUS_B, 12, 5, cbTemperature);
}


void loop(void)
{
	delay(1000);   // Do nothing forever
}


/* This callback is only called whenever there is a change in temperature */
void cbTemperature(uint8_t pinNumber, uint8_t sensorIndex, char* sensorAddress, float temperature) {
	logger.send(NOTICE, "TST", "Temperature change on pin: #%d. Sensor Addr: %s. Temperature: %f", pinNumber, sensorAddress, temperature);
}