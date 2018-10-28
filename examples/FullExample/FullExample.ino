/* This is a simple temperature regulator that can be controlled and watched via mqtt. Hardware: 
	1) A relay(with a heater) connected to pin RELAY_PIN. 
	2) A Dallas DS18B20 sensor connected to pin TEMPERATURE_SENSOR_PIN
	All temperature changes are reported to mqtt topic MQTT_TOPIC_TEMPERATURE. All changes on the relay are reported to MQTT_TOPIC_RELAY
	When temperature drops below a setpoint (initially 23C) the relay/heater turns on. If it's under it turns off
	The setpoint can be changed from the server by publishing the wanted temperature to the mqtt topic MQTT_TOPIC_SETPOINT_TEMPERATURE
*/


#include "_EXAMPLE_SETUP.h"   // wifi, mqtt credentials and mqtt topic setup here

#include "EvtLogger.h"
#include "EvtWiFi.h"
#include "EvtTimeNet.h"
#include "EvtMqtt.h"
#include "EvtDS18B20.h"
#include "EvtIO.h"

EvtTimeNet evtTime;
EvtWiFi evtWiFi;
EvtMqtt evtMqtt;
EvtDS18B20 evtDS18B20;
EvtIO evtIO;


float setpointTemperature = 23;   // The initial temperature setpoint for the relay


void setup(void)
{
	logger.setup(INFO, true);   // We want to timestamp logentries with our real time from RTC. And not too much log info.

	evtWiFi.begin(WIFI_SSID, WIFI_PASSWORD);	// Connect to wifi and keep connected forever
	evtTime.begin(0, 0, NTP_SERVER);			// Connect to the NTP server for getting time

	evtMqtt.begin(MQTT_SERVER, MQTT_PORT, MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS);		//Connect to Mqtt and keep connected forever
	evtMqtt.subscribe(MQTT_TOPIC_SETPOINT_TEMPERATURE, receivedSetpointTemperature);	// Subscribe to a topic to receive setpoint temperature from server.

	evtIO.outputSetup(RELAY_PIN, false, relayOutputChanged);				// Setup a pin for the output relay.
	evtDS18B20.addBus(TEMPERATURE_SENSOR_PIN, 12, 5, temperatureChanged);	// Configure temperature sensor. We want readings every 5 seconds.
}


void loop(void)
{
	delay(1000);   // Do nothing forever
}


/*	This callback function is called whenever a change in temperature is registred. It publishes the temperature to the Mqtt server
	If it's under the global setpoint value it turns on the relay for the heater
	If it's over it turns off the relay for the heater.
	So this is a very simple heating regulator 
*/
void temperatureChanged(uint8_t pinNumber, uint8_t sensorIndex, char* sensorAddress, float temperature) {
	logger.send(NOTICE, "REG", "Temperature changed on sensor %s to: %.2f C", sensorAddress, temperature);
	evtMqtt.publish(MQTT_TOPIC_TEMPERATURE, temperature, 2);
	if (temperature > setpointTemperature) {
		evtIO.outputSet(RELAY_PIN, false);
	}
	else {
		evtIO.outputSet(RELAY_PIN, true);
	}
}


/*	This callback function just sends the state of the heater relay to the mqtt server.
	It's called whenever the output pin of the relay changes. This means that we will be able to datalog every change on the server
*/
void relayOutputChanged(uint8_t pinNumber, bool pinState, unsigned long triggerCount) {
	evtMqtt.publish(MQTT_TOPIC_RELAY, pinState, "On", "Off");
	logger.send(NOTICE, "REG", "Relay turned %s because setpoint of %.2f C is crossed", pinState ? "On" : "Off", setpointTemperature);
}


/*	If we receive a mqtt message in the setpoint-topic, we change the setpoint to this.
	This means that we can change the regulators behaveour via MQTT
*/
void receivedSetpointTemperature(char* topic, float temperature) {
	logger.send(NOTICE, "REG", "Received a new setpoint temperature from MQTT: %.2f C", temperature);
	setpointTemperature = temperature;
}