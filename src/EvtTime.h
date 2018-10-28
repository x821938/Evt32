#ifndef _EVTTIME_h
#define _EVTTIME_h

#include <time.h>
#include "LinkedList.h"
#include "EvtLogger.h"


#define TIME_LAUNCH_STACK_SIZE 10000


typedef void(*TimeInCbFunc) (unsigned long msPassed); // Define callback function
typedef void(*TimeEveryCbFunc) (unsigned long msPassed, unsigned long triggerCount); // Define callback function


/* Each In-trigger is kept in this struct. A linked list of these are kept for storing many triggers */
struct TriggerIn {
	unsigned long ms;
	TimeInCbFunc cbFunc;
	unsigned long startedAtMs;
};


/* Each In-trigger is kept in this struct. A linked list of these are kept for storing many triggers */
struct TriggerEvery {
	unsigned long ms;
	TimeEveryCbFunc cbFunc;
	unsigned long triggerCount;
	unsigned long triggeredAtMs;
};



class EvtTime {
private:
	static LinkedList<TriggerIn*> triggerInList;
	static LinkedList<TriggerEvery*> triggerEveryList;

	static void taskTimerLauncher(void *pvParameters);
	static void handleTriggerIn();
	static void handleTriggerEvery();

public:
	EvtTime();
	void triggerIn(unsigned long ms, TimeInCbFunc cbFunc);
	bool triggerInRemove(unsigned long ms);
	void triggerEvery(unsigned long ms, TimeEveryCbFunc cbFunc);
	bool triggerEveryRemove(unsigned long ms);
};


#endif // ! _EVTTIMENET_h
