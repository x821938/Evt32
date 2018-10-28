#ifndef _EXAMPLE_h
#define _EXAMPLE_h

// General setup for all examples
#define WIFI_SSID		"my_ap"
#define WIFI_PASSWORD	"my_secret_password"

#define NTP_SERVER		"dk.pool.ntp.org"

#define MQTT_SERVER		"my_mqttserver.com"
#define MQTT_CLIENT_ID	"myesp32"
#define MQTT_PORT		8883
#define MQTT_USER		"my_mqtt_user"
#define MQTT_PASS		"my_mqtt_password"

// Setup for the Mqtt example:
#define MQTT_TOPIC_INT			"my_topic_root/int"
#define MQTT_TOPIC_INT_REPLY	"my_topic_root/reply/int"
#define MQTT_TOPIC_FLOAT		"my_topic_root/float"
#define MQTT_TOPIC_FLOAT_REPLY	"my_topic_root/reply/float"
#define MQTT_TOPIC_BOOL			"my_topic_root/bool"
#define MQTT_TOPIC_BOOL_REPLY	"my_topic_root/reply/bool"

// Setup for the FullExample:
#define MQTT_TOPIC_RELAY				"my_topic_root/relay"
#define MQTT_TOPIC_TEMPERATURE			"my_topic_root/temperature"
#define MQTT_TOPIC_SETPOINT_TEMPERATURE	"my_topic_root/setpoint"
#define RELAY_PIN						21 // IO pin for the temperature controlled relay
#define TEMPERATURE_SENSOR_PIN			23 // The pin where the DS18B20 temperature sensor is connected

#endif