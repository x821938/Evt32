#ifndef _EVTWIFI_h
#define _EVTWIFI_h


#define WIFI_STACK_SIZE 10000
#define WIFI_CHECK_FOR_CONNECTION_EVERY 10   // ms between each check to see if we are reconnected
#define WIFI_RECONNECT_INTERVAL 10   // Seconds between each wifi reconnect attempt



class EvtWiFi
{
private:
	char* _ssid;
	char* _psk;
	static void TaskKeepConnected(void *pvParameters);
	void periodicReconnect();
public:
	void begin(char* ssid, char* psk);
	bool isConnected();
};

#endif