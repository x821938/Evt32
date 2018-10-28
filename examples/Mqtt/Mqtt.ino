/*	This example connects to wifi and a mqtt server. It subscribes to 3 topics (expecting a bool, a float and an integer). 
	Whenever it receives a value it publishes it back to the mqtt server in another topic. 
	Try turning your wifi on and off to see how everything magically just reconnects and sends it's messages
*/

#include "_EXAMPLE_SETUP.h"   // Login credentials for wifi, mqtt is here

#include "EvtLogger.h"
#include "EvtWiFi.h"
#include "EvtMqtt.h"

EvtWiFi evtWiFi;
EvtMqtt evtMqtt;// Our Mqtt class instance


void setup(void)
{
	logger.setup(INFO, false);   // We don't want too much log info

	// Connect to wifi and mqtt
	evtWiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	evtMqtt.begin(MQTT_SERVER, MQTT_PORT, MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS);

	// Subscribe to 3 different topics, expecting 3 different variable types. Int, float and bool
	evtMqtt.subscribe(MQTT_TOPIC_INT, cbInt);
	evtMqtt.subscribe(MQTT_TOPIC_FLOAT, cbFloat);
	evtMqtt.subscribe(MQTT_TOPIC_BOOL, cbBool);
}

void loop(void)
{
	delay(1000);   // Do nothing forever
}


// Whenever something is received in the topic this callback function is called
void cbInt(char* topic, int value) {   
	logger.send(NOTICE, "TST", "Received an integer value %d in topic %s", value, topic);
	evtMqtt.publish(MQTT_TOPIC_INT_REPLY, value);   // The value we just received is just published back via mqtt
}


// Whenever something is received in the topic this callback function is called
void cbFloat(char* topic, float value) {
	logger.send(NOTICE, "TST", "Received an float value %f in topic %s", value, topic);
	evtMqtt.publish(MQTT_TOPIC_FLOAT_REPLY, value, 2);   // The value we just received is just published back via mqtt. We want 2 digit precision sent back
}


/*	Boolean values accepted and interpreted correctly are false=off,false,0,low and true=on,true,1,high. All case insensitive
	If you want to add others it can be done in EvtMqtt.h
	Whenever something is received in the topic this callback function is called
*/
void cbBool(char* topic, bool value) { 
	logger.send(NOTICE, "TST", "Received a bool value %d in topic %s", value, topic);

	/* The value we just received is just sent back via mqtt. Because it's a boolean value we need to choose a textual
		representation of true and false. Here I chose On and Off. */
	evtMqtt.publish(MQTT_TOPIC_BOOL_REPLY, value, "On", "Off");
}