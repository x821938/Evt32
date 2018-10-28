#include "EvtTime.h"


LinkedList<TriggerIn*> EvtTime::triggerInList;
LinkedList<TriggerEvery*> EvtTime::triggerEveryList;



/* Constructor, starts a task that is responsible of launching timerelated triggers */
EvtTime::EvtTime() {
	logger.send(DEBUG, "TIM", "Starting time launcher task");

	xTaskCreate(
		taskTimerLauncher,		// Task function.
		"TimerLauncher",		// Name of task.
		TIME_LAUNCH_STACK_SIZE,			// Stack size in words
		NULL,			// We need to give the static method TaskShowLog a reference to the instance of this class
		1,						// Priority of the task.
		NULL);
}



/* This task is responsible for going through all the kinds of triggertypes (now TriggerIn and TriggerEvery) */
void EvtTime::taskTimerLauncher(void *pvParameters) {
	while (true) {
		handleTriggerIn();
		handleTriggerEvery();
		vTaskDelay(1 / portTICK_PERIOD_MS);
	}
}



/*	Goes through the list of registered In-triggers. If the time of the trigger has run out it calls the callback for it
	This should be called regularly
*/
void EvtTime::handleTriggerIn() {
	for (uint8_t t = 0; t < triggerInList.size(); t++) {   // Go through each In-trigger in the list
		TriggerIn *triggerIn = triggerInList.get(t);
		unsigned long timePassed = millis() - triggerIn->startedAtMs;
		if (timePassed >= triggerIn->ms) {   // Has the trigger been running more than the specified time?
			logger.send(DEBUG, "TIM", "In-Trigger index %d has passed %d ms. Doing Callback", t, triggerIn->ms);
			triggerIn->cbFunc(timePassed);   // Do the callback
			triggerInList.remove(t);   // Trigger is not needed anymore. Discard it
			delete(triggerIn);   // Also remove/free it from memory
		}
	}
}



/*	Goes through the list of registered Every-triggers. If the time of the trigger has run out it calls the callback for it
	This should be called regularly
*/
void EvtTime::handleTriggerEvery() {
	for (uint8_t t = 0; t < triggerEveryList.size(); t++) {   // Go through each Every-trigger in the list
		TriggerEvery *triggerEvery = triggerEveryList.get(t);
		unsigned long timePassed = millis() - triggerEvery->triggeredAtMs;
		if (timePassed >= triggerEvery->ms) {   // Are we passed the limit since last triggering?
			triggerEvery->triggeredAtMs = millis();
			triggerEvery->triggerCount++;

			logger.send(DEBUG, "TIM", "Every-trigger index %d has passed %d ms. Doing Callback", t, triggerEvery->ms);
			triggerEvery->cbFunc(timePassed, triggerEvery->triggerCount);   // Do the callback
		}
	}
}



/*	Public method to register an in-trigger. Parameters:
	ms: The time in ms, that the trigger should go off after
	cbFunc: The callback function that should be called when triggered
*/
void EvtTime::triggerIn(unsigned long ms, TimeInCbFunc cbFunc) {
	logger.send(DEBUG, "TIM", "Setup trigger to fire in %d ms", ms);
	TriggerIn *triggerIn = new TriggerIn;
	triggerIn->ms = ms;
	triggerIn->cbFunc = cbFunc;
	triggerIn->startedAtMs = millis();

	triggerInList.add(triggerIn);
}



/*	Public method to remove an in-trigger. Parameters:
	ms: reference to the timer that should be removed.
	returns true if it was found and removed. Otherwise false 
*/
bool EvtTime::triggerInRemove(unsigned long ms) {
	for (uint8_t t = 0; t < triggerInList.size(); t++) {   // Go through each In-trigger in the list
		TriggerIn *triggerIn = triggerInList.get(t);
		if (triggerIn->ms == ms) {
			triggerInList.remove(t);
			delete(triggerIn);
			logger.send(DEBUG, "TIM", "TriggerIn %d ms removed", ms);
			return(true);
		}
	}
	logger.send(ERR, "TIM", "Could not remove TriggerIn %d ms", ms);
	return (false);
}



/*	Public method to register an every-trigger. Parameters:
	ms: Time in ms between each triggering
	cbFunc: The callback function that should be called when triggered
*/
void EvtTime::triggerEvery(unsigned long ms, TimeEveryCbFunc cbFunc) {
	logger.send(DEBUG, "TIM", "Setup trigger to fire every %d ms", ms);
	TriggerEvery *triggerEvery = new TriggerEvery;
	triggerEvery->ms = ms;
	triggerEvery->cbFunc = cbFunc;
	triggerEvery->triggerCount = 0;
	triggerEvery->triggeredAtMs = millis();

	triggerEveryList.add(triggerEvery);
}



/*	Public method to remove an every-trigger. Parameters:
	ms: reference to the timer that should be removed.
	returns true if it was found and removed. Otherwise false
*/
bool EvtTime::triggerEveryRemove(unsigned long ms) {
	for (uint8_t t = 0; t < triggerEveryList.size(); t++) {   // Go through each In-trigger in the list
		TriggerEvery *triggerEvery = triggerEveryList.get(t);
		if (triggerEvery->ms == ms) {
			triggerEveryList.remove(t);
			delete(triggerEvery);
			logger.send(DEBUG, "TIM", "TriggerEvery %d ms removed", ms);
			return(true);
		}
	}
	logger.send(ERR, "TIM", "Could not remove TriggerEvery %d ms", ms);
	return (false);
}