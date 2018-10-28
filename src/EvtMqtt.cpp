#include "EvtLogger.h"
#include "EvtMqtt.h"
#include "PubSubClient.h"


LinkedList<Subscription> EvtMqtt::mqttSubscriptionList;
QueueHandle_t EvtMqtt::mqttPublishQueue;



/*	Creates 2 tasks. One for keeping connected to the mqtt server and one for publishing things from the mqttPublishQueue. Parameters:
	mqttServer: hostname or IP of mqtt server
	mqttPort: portnumber. Normally it is 1883
	mqttClientId: A unique string that identifies this client to the mqtt-server.
	mqttUser: Username
	mqttPassword: Password
*/
void EvtMqtt::begin(char* mqttServer, uint16_t mqttPort, char* mqttClientId, char* mqttUser, char* mqttPassword) {
	mqttPublishQueue = xQueueCreate(MQTT_QUEUE_LENGTH, sizeof(PublishItem));

	_mqttServer = mqttServer;
	_mqttPort = mqttPort;
	_mqttClientId = mqttClientId;
	_mqttUser = mqttUser;
	_mqttPassword = mqttPassword;

	mqttClient = new PubSubClient(_mqttServer, _mqttPort, messageReceived, net);

	logger.send(DEBUG, "MQT", "Starting MQTT connection task");
	xTaskCreate(
		TaskKeepConnected,				// Task function.
		"MQTTKeepConnected",			// Name of task.
		MQTT_STACK_SIZE_KEEPCONNECTED,	// Stack size in words 
		(void*)this,					// We need to give the static method getTemperature a reference to the instance of this class
		1,								// Priority of the task.
		NULL);

	logger.send(DEBUG, "MQT", "Starting MQTT publishing task");
	xTaskCreate(
		TaskPublishQueue,				// Task function.
		"MQTTpublish",					// Name of task.
		MQTT_STACK_SIZE_PUBLISH,		// Stack size in words 
		(void*)this,					// We need to give the static method getTemperature a reference to the instance of this class
		1,								// Priority of the task.
		NULL);
}



/* A task that connects to the mqtt server and reconnects if it gets disconnected */
void EvtMqtt::TaskKeepConnected(void *pvParameters) {
	EvtMqtt inst = *((EvtMqtt*)pvParameters);   // We are inside static method. We need to be able to reference the instance.

	while (true) {
		while (WiFi.status() != WL_CONNECTED) {   // If we have no wifi, there is no need to try connecting mqtt
			vTaskDelay(100 / portTICK_PERIOD_MS);
		}
		logger.send(INFO, "MQT", "Connecting to MQTT server %s", inst._mqttServer);
		while (!inst.mqttClient->connect(inst._mqttClientId, inst._mqttUser, inst._mqttPassword)) {   // Keep reconnecting mqtt until we succeed
			vTaskDelay( 1000UL*MQTT_CHECK_FOR_CONNECTION_EVERY / portTICK_PERIOD_MS);
			logger.send(WARN, "MQT", "No MQTT connection. Reconnecting");
		}
		logger.send(INFO, "MQT", "Connected");
		inst.subscribeAll();   // We need to resubscibe all topics after a reconnection

		while (inst.mqttClient->connected()) {   // While connected we just keep mqtt loop running
			vTaskDelay(10 / portTICK_PERIOD_MS);
			inst.mqttClient->loop();   // Keep MQTT loop running every 10ms.
		}
		logger.send(INFO, "MQT", "We got disonnected");
		
	}
}



/* This task keep looking at the publishing queue and sends the content via MQTT. */
void EvtMqtt::TaskPublishQueue(void *pvParameters) {
	EvtMqtt inst = *((EvtMqtt*)pvParameters);   // We are inside static method. We need to be able to reference the instance.

	while (true) {
		if (inst.mqttClient->connected()) {
			PublishItem publishItem; // To hold an item from the publishing queue
			xQueueReceive(mqttPublishQueue, &publishItem, portMAX_DELAY); // A blocking read from queue. If its empty, we will just sit waiting here
			logger.send(DEBUG, "MQT", "Publishing value \"%s\" to topic \"%s\"", publishItem.value, publishItem.topic);
			inst.mqttClient->publish(publishItem.topic, publishItem.value);
		}
		vTaskDelay(MQTT_PUBLISH_EVERY / portTICK_PERIOD_MS); // To not flood the mqtt server we wait some time until next publish
	}
}



/* Runs through the list of mqtt topics to subscribe to and subscribes them to the MQTT server */
void EvtMqtt::subscribeAll() {
	for (int i = 0; i < mqttSubscriptionList.size(); i++) {
		Subscription subscription = mqttSubscriptionList.get(i);
		mqttClient->subscribe(subscription.topic);
		logger.send(INFO, "MQT", "Subscribed to topic \"%s\"", subscription.topic);
	}
}



/*	Private method for storing a subscription to a mqtt topic. It's stored in a linked list called mqttSubscriptionList. Parameters: 
	topic: mqtt topic
	cbFunction: a pointer to the callback function that should be called when a message is received in the topic
	type: if the callback function should be sent the value as CB_INT, CB_BOOL or CB_FLOAT
*/
void EvtMqtt::subscribe(char* topic, void* cbFunction, SubscribeCbType type) {
	Subscription *subscription = new Subscription();
	strncpy(subscription->topic, topic, sizeof(subscription->topic));
	subscription->subscribeCbFunc = (void*)cbFunction;
	subscription->subscribeCbType = type;
	logger.send(DEBUG, "MQT", "Register subscription to topic \"%s\"", topic);
	mqttSubscriptionList.add(*subscription);
	mqttClient->subscribe(topic);
}



/* Public method to provide a callback function for a certain mqtt topic */
void EvtMqtt::subscribe(char* topic, SubscribeCbFuncBool cbFunction) {
	subscribe(topic, (void*)cbFunction, CB_BOOL);
}



/* Public method to provide a callback function for a certain mqtt topic */
void EvtMqtt::subscribe(char* topic, SubscribeCbFuncInt cbFunction) {
	subscribe(topic, (void*)cbFunction, CB_INT);
}



/* Public method to provide a callback function for a certain mqtt topic */
void EvtMqtt::subscribe(char* topic, SubscribeCbFuncFloat cbFunction) {
	subscribe(topic, (void*)cbFunction, CB_FLOAT);
}



/* Callback function for the pubsub mqtt class. This gets called when a message (payload) is received in a subscribed topic */
void EvtMqtt::messageReceived(char* topic, byte* payload, unsigned int length) {
	char strPayload[MQTT_VALUE_LENGTH];
	if (length < MQTT_VALUE_LENGTH) {
		strncpy(strPayload, (const char*)payload, length);   // Get a local copy of the value if it has a legal length
		strPayload[length] = 0;   // Null terminate the string, because we receive it as a byte array and a length
		logger.send(DEBUG, "MQT", "Received \"%s\" in topic \"%s\"", strPayload, topic);
		for (int i = 0; i < mqttSubscriptionList.size(); i++) {   // Traverse the list of topic subscriptions
			Subscription subscription = mqttSubscriptionList.get(i);
			if (strcmp(subscription.topic, topic) == 0) {   // If it's in the list
				handleCallBacks(subscription, strPayload);   // We do a callback.
			}
		}
	} else {
		logger.send(WARN, "MQT", "Received a message of \"%d\" characters but only \"%d\" is allowed", length, MQTT_VALUE_LENGTH);
	}
}



/*	Does the users callback. It determines what kind of callback function should be called (bool, int, float). Parameters:
	subscription: a struct with topic and callback pointer
	strPayload: the value from pubsub client should be sent to the callback function */
void EvtMqtt::handleCallBacks(Subscription subscription, char* strPayload) {
	if (subscription.subscribeCbType == CB_INT) {
		SubscribeCbFuncInt cbf = (SubscribeCbFuncInt)subscription.subscribeCbFunc; //
		cbf(subscription.topic, atoi(strPayload));   // Convert the mqtt string value to int and do callback
	}
	if (subscription.subscribeCbType == CB_FLOAT) {
		SubscribeCbFuncFloat cbf = (SubscribeCbFuncFloat)subscription.subscribeCbFunc;
		cbf(subscription.topic, atof(strPayload));   // Convert the mqtt string value to float and do callback
	}
	if (subscription.subscribeCbType == CB_BOOL) {
		const String onNames[] = MQTT_BOOL_ON;
		const String offNames[] = MQTT_BOOL_OFF;
		bool value = false;
		bool valueOk = false;

		for (int i = 0; i < NUMITEMS(offNames); i++) {   // Go through the list of MQTT_BOOL_OFF names ("off", "false", "0" etc)
			if (offNames[i].equalsIgnoreCase(strPayload)) {
				valueOk = true;
				value = false;   // The mqtt payload message is false
			}
		}

		for (int i = 0; i < NUMITEMS(onNames); i++) {   // Go through the list of MQTT_BOOL_ON names ("on", "true", "1" etc)
			if (onNames[i].equalsIgnoreCase(strPayload)) {
				valueOk = true;
				value = true;   // The mqtt payload message is false
			}
		}

		if (valueOk) {
			SubscribeCbFuncBool cbf = (SubscribeCbFuncBool)subscription.subscribeCbFunc;
			cbf(subscription.topic, value);   // If we got a good value we do the callback
		} else {
			logger.send(WARN, "MQT", "Expected a bolean value in topic \"%s\", but got \"%s\"", subscription.topic, strPayload);
		}
	}
}



/*	Adds a mqtt topic/value to the mqttPublishQueue. Parameters:
	topic: mqtt topic
	value: a bool value
	onName: a string representation of the value if it's true
	offName: a string represenstation of the value if it's false 
*/
void EvtMqtt::publish(char* topic, bool value, const char* onName, const char* offName) {
	PublishItem publishItem;
	sprintf(publishItem.value, "%s", value ? onName : offName);
	strncpy(publishItem.topic, topic, sizeof(publishItem.topic));
	xQueueSend(mqttPublishQueue, &publishItem, 0); // Send the log message to the queue. If queue is full, just discard it.
}



/*	Adds a mqtt topic/value to the mqttPublishQueue. Parameters:
	topic: mqtt topic
	value: an integer value
*/
void EvtMqtt::publish(char* topic, int value) {
	PublishItem publishItem;
	itoa(value, publishItem.value , 10);
	strncpy(publishItem.topic, topic, sizeof(publishItem.topic));
	xQueueSend(mqttPublishQueue, &publishItem, 0); // Send the log message to the queue. If queue is full, just discard it.
}



/*	Adds a mqtt topic/value to the mqttPublishQueue. Parameters:
	topic: mqtt topic
	value: a float value
	decimals: the number of digits after the decimal point
*/
void EvtMqtt::publish(char* topic, float value, uint8_t decimals) {
	PublishItem publishItem;
	dtostrf(value, 4, decimals, publishItem.value);
	strncpy(publishItem.topic, topic, sizeof(publishItem.topic));
	xQueueSend(mqttPublishQueue, &publishItem, 0); // Send the log message to the queue. If queue is full, just discard it.
}
