#include "EvtLogger.h"
#include "EvtTime.h"

EvtTime evtTime;   // Our time class instance

void setup(void)
{
	logger.setup(INFO, false);   // We don't want to much logging

	// Set up a couple of one-time triggers
	evtTime.triggerIn(3000, cbTimerIn);   // After 3 seconds cbTimerIn will be called
	evtTime.triggerIn(7000, cbTimerIn);

	// And a couple of repeating triggers
	evtTime.triggerEvery(5000, cbTimerEvery);   // Every 5 seconds cbTimerEvery is called
	evtTime.triggerEvery(10000, cbTimerEvery);

	delay(20000);

	evtTime.triggerEveryRemove(5000);   // Remove one of the repeating triggers
}

void loop(void)
{
	delay(1000);   // Do nothing forever
}


// This is called when a one-time trigger is triggered
void cbTimerIn(unsigned long msPassed) {
	logger.send(NOTICE, "TST", "Onetime Timer triggered after %d ms", msPassed);
}


// This is called when a repeating trigger is triggered
void cbTimerEvery(unsigned long msPassed, unsigned long triggerCount) {   
	logger.send(NOTICE, "TST", "Repeating Timer triggered time number %d after %d ms", triggerCount, msPassed);
}
