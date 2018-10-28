#ifndef _EVTTIMENET_h
#define _EVTTIMENET_h

#include "EvtTime.h"
#include "EvtLogger.h"
#include <Arduino.h>
#include "LinkedList.h"
#include "WiFi.h"

#define TIME_SYNC_STACK_SIZE 2000
#define TIME_NETLAUNCH_STACK_SIZE 5000
#define TIME_TRIGGER_RESOLUTION 100   // how often the current time is updated in ms
#define TIME_RETRY_INTERVAL 10   // How often we retry getting time from NTP in seconds
#define TIME_RESYNC_INTERVAL 10   // How often the RTC is resynced from NTP server in minutes


/* Definition of weekdays that can be used for comparisons */
enum Weekday {
	SUNDAY,
	MONDAY,
	TUESDAY,
	WEDNESDAY,
	THURSDAY,
	FRIDAY,
	SATURDAY
};


/* Definition of months that can be used for comparisons */
enum Month {
	JANUARY,
	FEBRUARY,
	MARCH,
	APRIL,
	MAY,
	JUNE,
	JULY,
	AUGUST,
	SEPTEMBER,
	OCTOBER,
	NOVEMBER,
	DECEMBER
};


/* Time without data information */
struct TimeOnly {
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
};


typedef void(*TimerAtCbFunc) (TimeOnly time, unsigned long triggerCount); // Define callback function
typedef void(*TimerAtMinuteCbFunc) (uint8_t minute, unsigned long triggerCount); // Define callback function


/* Each At-trigger is kept in this struct. A linked list of these are kept for storing many triggers */
struct TriggerAt {
	TimeOnly time;
	TimerAtCbFunc cbFunc;
	bool justAdded;
	uint8_t triggeredDay;
	uint32_t secAfterMidnight;
	unsigned long triggerCount;
};


/* Each AtMinute-trigger is kept in this struct. A linked list of these are kept for storing many triggers */
struct TriggerAtMinute {
	uint8_t minute;
	TimerAtMinuteCbFunc cbFunc;
	bool justAdded;
	uint8_t triggeredHour;
	unsigned long triggerCount;
};


class EvtTimeNet : public EvtTime {

private:
	static LinkedList<TriggerAt*> triggerAtList;
	static LinkedList<TriggerAtMinute*> triggerAtMinuteList;
	static tm curTime;
	static uint32_t curSecSinceMidnight;
	static bool rtcSynced;

	static void taskTimeSync(void *pvParameters);
	static void taskNetTimerLauncher(void *pvParameters);

	void handleTriggerAt();
	void handleTriggerAtMinute();
	void updateTimeFromRtc();
	TimeOnly stringToTimeOnly(char* time);
	uint32_t getSecAfterMidnight(TimeOnly time);
	void prtDebugTime();

	long _gmtOffsetSec;
	int _daylightOffsetSec;
	char* _ntpServer;

public:
	EvtTimeNet();
	void begin(long gmtOffsetSec, int daylightOffsetSec, char* ntpServer);
	
	bool isBefore(TimeOnly time);
	bool isBefore(char* time);

	bool isAfter(TimeOnly time);
	bool isAfter(char* time);

	bool isBetween(TimeOnly start, TimeOnly end);
	bool isBetween(char* startTime, char* endTime);

	bool isRtcSynced();

	void triggerAt(TimeOnly time, TimerAtCbFunc cbFunc);
	void triggerAt(char* time, TimerAtCbFunc cbfunc);
	bool triggerAtRemove(TimeOnly time);
	bool triggerAtRemove(char* time);
	void triggerAtMinute(uint8_t minute, TimerAtMinuteCbFunc cbfunc);
	bool triggerAtMinuteRemove(uint8_t minute);

	tm getTime();
};

#endif
