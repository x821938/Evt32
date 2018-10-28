#include "EvtTimeNet.h"

LinkedList<TriggerAt*> EvtTimeNet::triggerAtList;
LinkedList<TriggerAtMinute*> EvtTimeNet::triggerAtMinuteList;
tm EvtTimeNet::curTime;
uint32_t EvtTimeNet::curSecSinceMidnight;
bool EvtTimeNet::rtcSynced;



/* Constructor, starts a task that is responsible of launching timerelated triggers */
EvtTimeNet::EvtTimeNet() {
	logger.send(DEBUG, "TIM", "Starting net time launcher task");

	xTaskCreate(
		taskNetTimerLauncher,		// Task function.
		"NetTimerLauncher",		// Name of task.
		TIME_NETLAUNCH_STACK_SIZE,			// Stack size in words
		(void*)this,			// We need to give the static method TaskShowLog a reference to the instance of this class
		1,						// Priority of the task.
		NULL);
}



/* This task is responsible for going through all the kinds of triggertypes (now TriggerAt) */
void EvtTimeNet::taskNetTimerLauncher(void *pvParameters) {
	EvtTimeNet inst = *((EvtTimeNet*)pvParameters);   // We are inside static method. We need to be able to reference the instance.
	while (true) {
		inst.handleTriggerAt();
		inst.handleTriggerAtMinute();
		vTaskDelay(TIME_TRIGGER_RESOLUTION / portTICK_PERIOD_MS);
	}
}



/*  This task is responsible for getting NTP time and sync the RTC to it. At certain intervals it will resync
	because the local RTC is not so precise */
void EvtTimeNet::taskTimeSync(void *pvParameters) {
	EvtTimeNet inst = *((EvtTimeNet*)pvParameters);   // We are inside static method. We need to be able to reference the instance.

	unsigned long lastSynced = millis();
	bool justBooted = true;

	while (true) {
		inst.updateTimeFromRtc();   // Keep the current time updated about once every 100ms. We only need 1sec accuracy for triggers so this should be sufficient.

		if (WiFi.status() == WL_CONNECTED) {   // Only if we are connected on wifi it makes sense to sync the clock
			if (!rtcSynced) {   // We don't have a valid time in the RTC
				if (millis() - lastSynced > 1000UL * TIME_RETRY_INTERVAL || justBooted) {   // Every X sec we try syncing time until we succeed
					logger.send(WARN, "TIM", "RTC time not valid. trying sync from NTP server %s", inst._ntpServer);
					configTime(inst._gmtOffsetSec, inst._daylightOffsetSec, inst._ntpServer);
					inst.updateTimeFromRtc();
					inst.prtDebugTime();
					lastSynced = millis();
					justBooted = false;
				}
			}
			else {   // We already have a valid time in RTC
				if (millis() - lastSynced > TIME_RESYNC_INTERVAL * 60000UL) {   // Is it time to resync it, because local RTC is not so good.
					logger.send(DEBUG, "TIM", "it's %d minutes since last resync. Resyncing time from NTP server %s", TIME_RESYNC_INTERVAL, inst._ntpServer);
					configTime(inst._gmtOffsetSec, inst._daylightOffsetSec, inst._ntpServer);
					inst.updateTimeFromRtc();
					inst.prtDebugTime();
					lastSynced = millis();
				}
			}
		}
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}



/*	Gets the time from the RTC clock and updates these properties in this class:
	rtcSynced: If this is true we have a valid time in our RTC clock. Otherwise it's false
	curTime: the current time in RTC
	curSecSinceMidnight: The calculated number of seconds since midnight the current day.
*/
void EvtTimeNet::updateTimeFromRtc() {
	rtcSynced = getLocalTime(&curTime, 1000);   // Time should come back in less than a second
	if (rtcSynced) {
		curSecSinceMidnight = getSecAfterMidnight({ curTime.tm_hour, curTime.tm_min, curTime.tm_sec } );
	}
}



/*	Goes through the list of registered At-triggers. If a trigger is due it calls the callback for it 
	This should be called regularly more often than every second, because we have a 1 second resolution on our At-trigger */
void EvtTimeNet::handleTriggerAt() {
	if (rtcSynced) {   // Only if we have a valid RTC time it makes sense to do triggers
		for (uint8_t t = 0; t < triggerAtList.size(); t++) {   // Go through each At-trigger in the list
			TriggerAt *ta = triggerAtList.get(t);
			if (ta->justAdded) {   // If it's the first time we handle the trigger, we need to see if current time is before or after the trigger time
				if (curSecSinceMidnight >= ta->secAfterMidnight) {
					ta->triggeredDay = curTime.tm_mday;
				} else {
					ta->triggeredDay = curTime.tm_mday - 1;
				}

				ta->justAdded = false;
			}
			if (ta->triggeredDay != curTime.tm_mday && curSecSinceMidnight >= ta->secAfterMidnight ) {   // If we havn't triggered today and trigger time is reached
				logger.send(DEBUG, "TIM", "At-Trigger index %d has passed %02d:%02d:%02d. Doing Callback", t, ta->time.hour, ta->time.minute, ta->time.second);
				ta->cbFunc(ta->time, ++ta->triggerCount);   // Do the callback
				ta->triggeredDay = curTime.tm_mday;   // We did a trigger today. No more today...
			}
		}
	}
}



/*	Goes through the list of registered AtMinute-triggers. If a trigger is due it calls the callback for it
	This should be called regularly, because we have a 1 minute resolution on our At-trigger */
void EvtTimeNet::handleTriggerAtMinute() {
	if (rtcSynced) {   // Only if we have a valid RTC time it makes sense to do triggers
		for (uint8_t t = 0; t < triggerAtMinuteList.size(); t++) {   // Go through each AtMinute-trigger in the list
			TriggerAtMinute *tam = triggerAtMinuteList.get(t);
			if (tam->justAdded) {   // If it's the first time we handle the trigger, we need to see if current time is before or after the trigger time
				if (curTime.tm_min == tam->minute) {
					tam->triggeredHour = curTime.tm_hour;
				} else {
					tam->triggeredHour = curTime.tm_hour - 1;
				}

				tam->justAdded = false;
			}
			if (tam->triggeredHour != curTime.tm_hour && curTime.tm_min == tam->minute) {   // If we havn't triggered this hour and trigger minute is reached
				logger.send(DEBUG, "TIM", "AtMinute-Trigger index %d has reached minute %d. Doing Callback", t, tam->minute);
				tam->cbFunc(tam->minute, ++tam->triggerCount);   // Do the callback
				tam->triggeredHour = curTime.tm_hour;   // We did a trigger this day. No more this hour...
			}
		}
	}
}



/*	Returns true if current time is before the provided time. Parameters:
	time: the time to check agains 
*/
bool EvtTimeNet::isBefore(TimeOnly time) {
	return (getSecAfterMidnight(time) > curSecSinceMidnight);
}



/*	Returns true if current time is before the provided time. Parameters:
	time: the time to check agains
*/
bool EvtTimeNet::isBefore(char* time) {
	return (isBefore(stringToTimeOnly(time)));
}



/*	Returns true if current time is after the provided time. Parameters:
	time: the time to check agains 
*/
bool EvtTimeNet::isAfter(TimeOnly time) {
	return (getSecAfterMidnight(time) < curSecSinceMidnight);
}



/*	Returns true if current time is after the provided time. Parameters:
	time: the time to check agains
*/
bool EvtTimeNet::isAfter(char* time) {
	return (isAfter(stringToTimeOnly(time)));
}



/*	Returns true if current time is between the two provided times. Parameters:
	start: The beginning of the time period
	end: The end of the time period
*/
bool EvtTimeNet::isBetween(TimeOnly start, TimeOnly end) {
	return (isAfter(start) && isBefore(end));
}



/*	Returns true if current time is between the two provided times. Parameters:
	startTime: The beginning of the time period
	endTime: The end of the time period
*/
bool EvtTimeNet::isBetween(char* startTime, char* endTime) {
	return (isBetween(stringToTimeOnly(startTime), stringToTimeOnly(endTime)));
}



/* Returns true if we have a valid time in the RTC clock */
bool EvtTimeNet::isRtcSynced() {
	return(rtcSynced);
}



/* Returns the current time (which is updated regularly by the taskTimeSync task */
tm EvtTimeNet::getTime() {
	return (curTime);
}



/* Given a time it returns the seconds that has passed since midnight */
uint32_t EvtTimeNet::getSecAfterMidnight(TimeOnly time) {
	return (time.second + time.minute * 60 + time.hour * 3600);
}



/*	Publich method that creates a task for syncing RTC clock with an ntp source. Parameters:
	gmtOffsetSec: positive or negative offset in seconds to GMT time
	daylightOffsetSec: offset in seconds for daylight savings
	ntpServer: IP or FQDN of NTP server
*/
void EvtTimeNet::begin(long gmtOffsetSec, int daylightOffsetSec, char* ntpServer) {
	_gmtOffsetSec = gmtOffsetSec;
	_daylightOffsetSec = daylightOffsetSec;
	_ntpServer = ntpServer;

	logger.send(DEBUG, "TIM", "Starting time sync task");

	xTaskCreate(
		taskTimeSync,		// Task function.
		"TimerSync",		// Name of task.
		TIME_SYNC_STACK_SIZE,			// Stack size in words
		(void*)this,			// We need to give the static method TaskShowLog a reference to the instance of this class
		1,						// Priority of the task.
		NULL);
}



/*	Public method to register an At-trigger. Parameters:
	time: The time the trigger should go off
	cbFunc: The callback function that should be called when triggered
*/
void EvtTimeNet::triggerAt(TimeOnly time, TimerAtCbFunc cbFunc) {
	logger.send(DEBUG, "TIM", "Setup trigger to fire when RTC time is %02d:%02d:%02d every day", time.hour, time.minute, time.second);
	TriggerAt *ta = new TriggerAt{ time, cbFunc };
	ta->triggerCount = 0;
	ta->secAfterMidnight = getSecAfterMidnight({ time.hour, time.minute, time.second } );   // This is stored so we don't have to calculate everytime checked.
	ta->justAdded = true;

	triggerAtList.add(ta);   // Add it to the list
}



/*	Public method to register an At-trigger. Parameters:
	time: The time the trigger should go off
	cbFunc: The callback function that should be called when triggered
*/
void EvtTimeNet::triggerAt(char* time, TimerAtCbFunc cbfunc) {
	triggerAt(stringToTimeOnly(time), cbfunc);
}



/*	Public method to remove a previously created At-trigger. Parameters:
	time: The time of the preciously created trigger
	Returns true if a trigger was found and deleted. Otherwise false
*/
bool EvtTimeNet::triggerAtRemove(TimeOnly time) {
	for (uint8_t t = 0; t < triggerAtList.size(); t++) {   // Go through each At-trigger in the list
		TriggerAt *triggerAt = triggerAtList.get(t);
		if (triggerAt->secAfterMidnight == getSecAfterMidnight(time)) {
			triggerAtList.remove(t);
			delete(triggerAt);
			logger.send(DEBUG, "TIM", "TriggerAt %02d:%02d:%02d removed", time.hour, time.minute, time.second);
			return(true);
		}
	}
	logger.send(ERR, "TIM", "Could not remove TriggerAt %02d:%02d:%02d", time.hour, time.minute, time.second);
	return (false);
}

bool EvtTimeNet::triggerAtRemove(char* time) {
	triggerAtRemove(stringToTimeOnly(time));
}



/*	Public method to register an AtMinute-trigger. Parameters:
	minute: The minute value on the RTC current time the trigger should go off
	cbFunc: The callback function that should be called when triggered
*/
void EvtTimeNet::triggerAtMinute(uint8_t minute, TimerAtMinuteCbFunc cbFunc) {
	logger.send(DEBUG, "TIM", "Setup trigger to fire when RTC minute is %02d every hour", minute);
	TriggerAtMinute *tam = new TriggerAtMinute{ minute, cbFunc };
	tam->triggerCount = 0;
	tam->justAdded = true;

	triggerAtMinuteList.add(tam);
}



/*	Public method to remove a previously created AtMinute-trigger. Parameters:
	minute: The minute of the created trigger
	Returns true if a trigger was found and deleted. Otherwise false 
*/
bool EvtTimeNet::triggerAtMinuteRemove(uint8_t minute) {
	for (uint8_t t = 0; t < triggerAtMinuteList.size(); t++) {   // Go through each AtMinute-trigger in the list
		TriggerAtMinute *triggerAtMinute = triggerAtMinuteList.get(t);
		if (triggerAtMinute->minute == minute) {
			triggerAtMinuteList.remove(t);
			delete(triggerAtMinute);
			logger.send(DEBUG, "TIM", "TriggerAtMinute at %d removed", minute);
			return(true);
		}
	}
	logger.send(ERR, "TIM", "Could not remove TriggerAtMinute %d", minute);
	return (false);
}



/*	Converts a string to a TimeOnly struct (hour, min, sec). Parameters
	time: a string of the format "21:19" or "21:19:05" 
*/
TimeOnly EvtTimeNet::stringToTimeOnly(char* time) {
	char timeCopy[9];
	strncpy(timeCopy, time, 9);
	timeCopy[2] = 0;   // Zero terminate positions with the colon
	timeCopy[5] = 0;   // Zero terminate positions with the colon

	if (strlen(time) == 5) strcpy(timeCopy + 6, "00");   // If we have "21:19" format, we add the seconds as zero

	TimeOnly to{ atoi(timeCopy), atoi(timeCopy + 3), atoi(timeCopy + 6) };
	return (to);
}



/* When a NTP sync is made, this is called to print the fetched time nicely */
void EvtTimeNet::prtDebugTime() {
	if (rtcSynced) {
		logger.send(INFO, "TIM", "Our fresh NTP date is now %04d-%02d-%02d and time is %02d:%02d:%02d", \
			curTime.tm_year + 1900, curTime.tm_mon + 1, curTime.tm_mday, curTime.tm_hour, curTime.tm_min, curTime.tm_sec);
	}
}
