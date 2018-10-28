#include "_EXAMPLE_SETUP.h"   // Login credentials for wifi, mqtt is here

#include "EvtLogger.h"
#include "EvtTimeNet.h"
#include "EvtWiFi.h"

EvtWiFi evtWiFi;   // Our event-wifi class instance
EvtTimeNet evtTime;    // Our event-time class instance


void setup(void)
{
	logger.setup(DEBUG, true);   // We want to timestamp logentries with our real time from RTC

	evtWiFi.begin(WIFI_SSID, WIFI_PASSWORD);	// From now on wifi-instance will keep the wifi up even on bad connections
	evtTime.begin(0, 0, NTP_SERVER);			// Offset in sec to GMT+daylightsaving, NTP server. Will keep synced
	
	evtTime.triggerAt("15:30", cbTimerAt);			// Set a trigger at 03:30PM
	evtTime.triggerAt({ 06, 0, 0 }, cbTimerAt);		// Set a trigger at 06:00:00AM
	//evtTime.triggerAtRemove({ 15, 30, 0 });		// Remove a trigger at 03:30:00PM

	evtTime.triggerAtMinute(55, cbTimerAtMinute);	// set a trigger for the minutes of time ending in 55.
	//evtTime.triggerAtMinuteRemove(55);			// Remove a trigger that happens at minute 55

	delay(10000);   // We wait until we are sure that RTC has synced time from NTP server
	if (evtTime.isRtcSynced()) {
		// a demonstration of some time comparison functions
		logger.send(NOTICE, "TST", "Are we between 19:00 and 21:00? %d", evtTime.isBetween({ 19, 0, 0 }, { 21, 0, 0 }));
		logger.send(NOTICE, "TST", "Are we between 05:00 and 06:00? %d", evtTime.isBetween("05:00", "06:00"));
		logger.send(NOTICE, "TST", "Are we after 22:00? %d", evtTime.isAfter({ 22, 0, 0 }));
		logger.send(NOTICE, "TST", "Are we before 13:00:10? %d", evtTime.isBefore("13:00:10"));
		logger.send(NOTICE, "TST", "Is today a saturday? %d", evtTime.getTime().tm_wday == SATURDAY);
		logger.send(NOTICE, "TST", "Is this month october? %d", evtTime.getTime().tm_mon == OCTOBER);
	}
}


void loop(void)
{
	delay(1000);   // Do nothing forever
}


/* This callback function is called when a one-time timer is triggered */
void cbTimerAt(TimeOnly time, unsigned long triggerCount) {
	logger.send(NOTICE, "TST", "Triggered at %02d:%02d:%02d. TriggerCount=%d", time.hour, time.minute, time.second, triggerCount);
}


/* This callback function is called when a repeating timer is triggered */
void cbTimerAtMinute(uint8_t minute, unsigned long triggerCount) {
	logger.send(NOTICE, "TST", "Triggered at minute %d. TiggerCount=%d", minute, triggerCount);
}
