#ifndef _EVTMQTT_h
#define _EVTMQTT_h

#include <WiFi.h>
#include "PubSubClient.h"
#include <LinkedList.h>

#define MQTT_STACK_SIZE_KEEPCONNECTED 5000
#define MQTT_STACK_SIZE_PUBLISH 5000
#define MQTT_CHECK_FOR_CONNECTION_EVERY 5   // Seconds
#define MQTT_QUEUE_LENGTH 10
#define MQTT_VALUE_LENGTH 10
#define MQTT_TOPIC_LENGTH 50
#define MQTT_PUBLISH_EVERY 100   //ms
#define MQTT_BOOL_ON {"on", "true", "1", "high"}
#define MQTT_BOOL_OFF {"off", "false", "0", "low"}
#define NUMITEMS(arg) ((unsigned int) (sizeof (arg) / sizeof (arg [0]))) // Used to find the number of values in the on/off list


// Define the callback functions.
typedef void(*SubscribeCbFuncBool) (char* topic, bool value);
typedef void(*SubscribeCbFuncInt) (char* topic, int value); 
typedef void(*SubscribeCbFuncFloat) (char* topic, float value);


// The type of callback function in the subscriptionlist
enum SubscribeCbType {
	CB_BOOL,
	CB_INT, 
	CB_FLOAT
};


// A single entry of the mqtt subscription list. We need this list in case mqtt gets disconnected. Then we need to resubscribe to topic again
struct Subscription {
	char topic[MQTT_TOPIC_LENGTH];
	void* subscribeCbFunc;
	SubscribeCbType subscribeCbType;
};


// An entry to send the the mqtt publish queue.
struct PublishItem {
	char topic[MQTT_TOPIC_LENGTH];
	char value[MQTT_VALUE_LENGTH];
};



class EvtMqtt
{
 private:
	 WiFiClient net;
	 PubSubClient *mqttClient;
	 static LinkedList<Subscription> mqttSubscriptionList;
	 static QueueHandle_t mqttPublishQueue;
	 static void TaskKeepConnected(void *pvParameters);
	 static void TaskPublishQueue(void *pvParameters);
	 static void messageReceived(char* topic, byte* payload, unsigned int length);
	 static void handleCallBacks(Subscription subscription, char* strPayload);
	 void subscribe(char* topic, void* cbFunction, SubscribeCbType type);
	 void subscribeAll();

	 char* _mqttServer;
	 uint16_t _mqttPort;
	 char* _mqttClientId;
	 char* _mqttUser; 
	 char* _mqttPassword;
 public:
	 void begin(char* mqttServer, uint16_t mqttPort, char* mqttClientId, char* mqttUser, char* mqttPassword);
	 void subscribe(char* topic, SubscribeCbFuncBool cbFunction);
	 void subscribe(char* topic, SubscribeCbFuncInt cbFunction);
	 void subscribe(char* topic, SubscribeCbFuncFloat cbFunction);
	 void publish(char* topic, bool value, const char* onName, const char* offName);
	 void publish(char* topic, int value);
	 void publish(char* topic, float value, uint8_t decimals);
};

#endif