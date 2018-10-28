#include "EvtDS18B20.h"


LinkedList<BusSetup*> EvtDS18B20::busList;



EvtDS18B20::EvtDS18B20() {
	logger.send(DEBUG, "TMP", "Starting task for ds18b20 temperature sensors");

	// A task is created that is responsible of constantly getting data from thermometers on the busses
	xTaskCreate(
		TaskGetTemperature,				// Task to run
		"TemperatureTask",				// Name of the task
		TEMPERATURE_STACK_SIZE,	// Stack size in words
		NULL,							// Parameters for the task
		1,								// Priority of the task
		NULL);
}



/*	Add a onewire bus. The bus will be scanned for temperature sensors and they will be added to the list for probing. Parameters:
	pinNumber:		physical IO pin where sensors are connected. one or more sensors can be connected in parallel on the pin
	precision:		Precision on temperature sensors in bits (9 to 12). 9 is coarse but fast, 12 is fine but slow.
	fetchInterval:	How often the task will check for temperature changes in seconds
	callBackFunc:	The function that should be called when a new tempereture is measured 
*/
bool EvtDS18B20::addBus(uint8_t pinNumber, uint8_t precision, uint16_t fetchInterval, TempCbFunc callBackFunc) {
	logger.send(DEBUG, "TMP", "Setup ds18b20 bus on pin=%d, prec=%dbit, upd interval=%dsec", pinNumber, precision, fetchInterval);
	// Make a bus entry that will be filled with all setup information
	BusSetup *bus = new BusSetup;
	bus->pinNumber = pinNumber;
	bus->precision = precision;
	bus->fetchInterval = fetchInterval;
	bus->callBackFunc = callBackFunc;
	bus->oneWire = new OneWire(pinNumber);
	bus->sensors = new DallasTemperature(bus->oneWire);

	for (int searchAttempts = 0; searchAttempts < MAX_BUS_SEARCHES; searchAttempts++) {
		// Start the dallas library and count devices on the bus
		bus->sensors->begin();
		uint8_t devicesFound = bus->sensors->getDeviceCount();

		if (devicesFound > 0) {
			// Traverse all devices found on the bus
			for (uint8_t dev = 0; dev < devicesFound; dev++) {
				Thermometer *thermometer = new Thermometer;

				if (bus->sensors->getAddress(thermometer->address, dev)) {   // A found sensor address is stored in a thermometer struct.
					bus->sensors->setResolution(thermometer->address, bus->precision); // And precision is setup on the sensor

					// Calculate the hex string representation of the address and also put it in the thermometer struct.
					for (uint8_t i = 0; i < 8; i++) sprintf(thermometer->addressStr + i * 2, "%02X", thermometer->address[i]);

					logger.send(INFO, "TMP", "Found sensor on bus %d:%d with address %s", bus->pinNumber, dev, thermometer->addressStr);
					bus->thermometerList.add(thermometer); // A found device is added to the linked list "thermometerList" (resides inside the bus struct).
				}
				else {
					logger.send(ERR, "TMP", "Could not find sensor at %d:%d", bus->pinNumber, dev);
				}
			}

			busList.add(bus); // Add the bus with it's found thermometers to the busList. This is used by the "TaskGetTemperature"
			return(true);
		} else {   // No devices found on the bus
			logger.send(WARN, "TMP", "No sensors found on bus/pin number %d", bus->pinNumber);
			vTaskDelay(BUS_SEARCH_RETRY_TIME / portTICK_PERIOD_MS);   // Give some time to other tasks for one second
		}
	}
	logger.send(ERR, "TMP", "Giving up finding sensors on bus/pin number %d", bus->pinNumber);
	return(false);   // No sensors found on the bus
}



/*	A task that periodically gets temperature for the busses. 
	This has to be a static method because eps32 tasks can't call an instance member of a class! 
*/
void EvtDS18B20::TaskGetTemperature(void *pvParameters) {
	while (true) {
		for (uint b = 0; b < busList.size(); b++) {   // Go through one bus at the time
			BusSetup *bus = busList.get(b);
			if (millis() - bus->lastFetched > 1000UL * bus->fetchInterval) { // If it's time to fetch another temperature
				bus->sensors->requestTemperatures();   // Request temperatures from all the devices on the bus

				for (uint8_t deviceIndex = 0; deviceIndex < bus->thermometerList.size(); deviceIndex++) {   // Traverse one thermometer on the bus at the time
					Thermometer *thermometer = bus->thermometerList.get(deviceIndex);

					float temperature = bus->sensors->getTempC(thermometer->address);   // Get the temperature

					if (temperature != thermometer->lastTemperature) {   // If the temperature has changed since last reading
						if (temperature > -127) {   // And we got a good valid temperature reading
							logger.send(DEBUG, "TMP", "Device %d:%d changed temperature to %fC. Doing Callback", bus->pinNumber, deviceIndex, temperature);
							bus->callBackFunc(bus->pinNumber, deviceIndex, thermometer->addressStr, temperature);   // Do the callback
							thermometer->lastTemperature = temperature;
						}
						else {
							logger.send(ERR, "TMP", "Invalid temperature from device %d:%d", bus->pinNumber, deviceIndex);
						}
					}
				}
				bus->lastFetched = millis();
			}
		}
		vTaskDelay(100 / portTICK_PERIOD_MS);   // Give some time to other tasks
	}
}