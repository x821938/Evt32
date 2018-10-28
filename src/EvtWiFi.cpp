#include "EvtLogger.h"
#include "WiFi.h"
#include "EvtWiFi.h"



/*	Sets up a task that keeps connected to a WiFi AP. Parameters:
	ssid: The AP's ssid
	psk: Preshared key for the wifi aka password.
*/
void EvtWiFi::begin(char* ssid, char* psk)
{
	_ssid = ssid;
	_psk = psk;
	logger.send(DEBUG, "WFI", "Starting Wifi Task");

	xTaskCreate(
		TaskKeepConnected,		// Task function.
		"WifiKeepConnected",	// Name of task.
		WIFI_STACK_SIZE,	// Stack size in words 
		(void*)this,			// We need to give the static method a reference to the instance of this class
		1,						// Priority of the task.
		NULL);
}



/* Returns true if we are connected to wifi. Otherwise false */
bool EvtWiFi::isConnected() {
	return (WiFi.status() == WL_CONNECTED);
}



/* This task connects to wifi. If we loose the connection it reconnects */
void EvtWiFi::TaskKeepConnected(void *pvParameters) {
	EvtWiFi inst = *((EvtWiFi*)pvParameters); // We are inside static method. We need to be able to reference the instance.
	logger.send(INFO, "WFI", "Connecting to SSID %s", inst._ssid);
	WiFi.begin(inst._ssid, inst._psk);

	while (true) {   // We are inside a task, so we want to continue forever
		while (!inst.isConnected()) {   // We are not connected. Bad.
			inst.periodicReconnect();
			vTaskDelay(WIFI_CHECK_FOR_CONNECTION_EVERY / portTICK_PERIOD_MS); 
		}
		logger.send(INFO, "WFI", "Connected");

		while (inst.isConnected()) {   // We are already connected. Great. Do nothing.
			vTaskDelay(WIFI_CHECK_FOR_CONNECTION_EVERY / portTICK_PERIOD_MS);
		}
		logger.send( ERR, "WFI", "We got disconnected");
	}
}



/* After X seconds has passed it sends a log warning message and tells wifi module to reconnect */
void EvtWiFi::periodicReconnect() {
	static long lastWarning = millis();
	if ( millis() - lastWarning > 1000UL * WIFI_RECONNECT_INTERVAL) {
		logger.send(WARN, "WFI", "No Wifi. Reconnecting");
		WiFi.reconnect();
		lastWarning = millis();
	}
}
