#include "EvtIO.h"

portMUX_TYPE EvtIO::mux = portMUX_INITIALIZER_UNLOCKED;
volatile Interrupt EvtIO::interruptPin[10];
uint8_t EvtIO::numOfTriggers = 0;



/* Constructor, starts a task that is responsible of handle the incomming interrupts */
EvtIO::EvtIO() {
	if (numOfTriggers == 0) { // We want one task to handle all interrupts
		logger.send(DEBUG, "IOP", "Starting IO handling taks");
		xTaskCreate(
			taskHandleInterrupts,	// Task function to call.
			"HandleInterrupts",		// Name of task.
			IO_STACK_SIZE,			// Stack size in words 
			NULL,					// We don't need to pass any parameters
			1,						// Priority of the task.
			NULL);
	}
}



/*	This task is responsible for going through all created interrupt triggers. If an interrupt counter has changed for
	a certain pin, the trigger is handled by calling the provided callback 
*/
void EvtIO::taskHandleInterrupts(void *pvParameters) {
	while (true) {
		for (uint8_t i = 0; i < numOfTriggers; i++) {   // Go through all the created triggers
			portENTER_CRITICAL(&mux);   // Make sure that our volatile variables are not touched by ISR
			if (interruptPin[i].interruptCount != interruptPin[i].lastInterruptCount) {   // Anything happened since last time on this pin
				// Save/copy all informations about the trigger
				uint8_t pinNumber = interruptPin[i].pinNumber;
				bool pinState = interruptPin[i].pinValue;
				uint32_t cbCalledTimes = ++interruptPin[i].triggerCount;
				InputCbFunc inputCbFunction = interruptPin[i].inputCbFunction;
				interruptPin[i].lastInterruptCount = interruptPin[i].interruptCount;
				portEXIT_CRITICAL(&mux);   // Not critical anymore bacause we have a copy
				logger.send(DEBUG, "IOP", "Received interrupt%d on pin %d. Doing Callback", i, pinNumber);
				inputCbFunction(pinNumber, pinState, cbCalledTimes);   // Do the callback
			}
			else { portEXIT_CRITICAL(&mux); }
		}
		vTaskDelay(10 / portTICK_PERIOD_MS); // TODO: Debounce or is 10ms resolution enough
	}
}



/* Public method to register a trigger on a digital input. Parameters:
	pinNumber: the physical pin number that we want to watch
	pinMode: 
	cbFunc: Callback function to call when triggered
	Returns true if the trigger is succesfully added 
*/
bool EvtIO::trigger(uint8_t pinNumber, uint8_t mode, InputCbFunc cbFunc) {
	if (numOfTriggers < 10) {   // If we havn't added too many triggers.
		logger.send(DEBUG, "IOP", "Setup interrupt trigger on pin %d", pinNumber);

		// Save pin and callback in our volatile shared array.
		interruptPin[numOfTriggers].pinNumber = pinNumber; 
		interruptPin[numOfTriggers].inputCbFunction = cbFunc;

		pinMode(pinNumber, mode);
		// We now attach an interrupt handler to our pin. Each pin needs it's own handler function (from 0-9)
		if (numOfTriggers == 0) attachInterrupt(digitalPinToInterrupt(pinNumber), handleHwInterrupt0, CHANGE);
		if (numOfTriggers == 1) attachInterrupt(digitalPinToInterrupt(pinNumber), handleHwInterrupt1, CHANGE);
		if (numOfTriggers == 2) attachInterrupt(digitalPinToInterrupt(pinNumber), handleHwInterrupt2, CHANGE);
		if (numOfTriggers == 3) attachInterrupt(digitalPinToInterrupt(pinNumber), handleHwInterrupt3, CHANGE);
		if (numOfTriggers == 4) attachInterrupt(digitalPinToInterrupt(pinNumber), handleHwInterrupt4, CHANGE);
		if (numOfTriggers == 5) attachInterrupt(digitalPinToInterrupt(pinNumber), handleHwInterrupt5, CHANGE);
		if (numOfTriggers == 6) attachInterrupt(digitalPinToInterrupt(pinNumber), handleHwInterrupt6, CHANGE);
		if (numOfTriggers == 7) attachInterrupt(digitalPinToInterrupt(pinNumber), handleHwInterrupt7, CHANGE);
		if (numOfTriggers == 8) attachInterrupt(digitalPinToInterrupt(pinNumber), handleHwInterrupt8, CHANGE);
		if (numOfTriggers == 9) attachInterrupt(digitalPinToInterrupt(pinNumber), handleHwInterrupt9, CHANGE);

		numOfTriggers++;
		return(true);
	} else {
		logger.send(ERR, "IOP", "No more than 10 input pins can be configured");
		return(false);
	}
}



/*	Public method to setup a pin as output and register a callback function for changes on that pin. Parameters:
	pinNumber: the physical pin
	reversedOutput: When a high is sent to outputSet it will take the pin low. 
	cbFunc: the call back function. Can be omitted if you don't want any callbacks happeing on output operations.
	Returns true if pin is successfully setup. If it's already setup we return false.
*/
bool EvtIO::outputSetup(uint8_t pinNumber, bool reversedOutput, OutputCbFunc cbFunc) {
	for (uint8_t t = 0; t < outputConfList.size(); t++) {   // Go through each pin in the output config-list
		if (outputConfList.get(t)->pinNumber == pinNumber) {
			logger.send(ERR, "IOP", "Setup pin %d has already been set up", pinNumber);
			return(false);   // If the pin already is configure, we return error.
		}
	}
	logger.send(DEBUG, "IOP", "Setup pin %d as output", pinNumber);
	pinMode(pinNumber, OUTPUT);
	OutputConf *output = new OutputConf({ pinNumber, reversedOutput, cbFunc, 0 });
	outputConfList.add(output);
	return(true);
}



/*	Public method to setup a pin as output without a callback function. Parameters:
	pinNumber: the physical pin
	reversedOutput: When a high is sent to outputSet it will take the pin low.
	Returns true if pin is successfully setup. If it's already setup we return false.
*/
bool EvtIO::outputSetup(uint8_t pinNumber, bool reversedOutput) {
	return (outputSetup(pinNumber, reversedOutput, nullptr));
}



/*	Public method to change the output of a physical pin. Parameters:
	pinNumber: The pin number
	pinState: The digital value the pin should be set to. 
	Returns true if the provided pin has already been configured. Otherwise false
*/
bool EvtIO::outputSet(uint8_t pinNumber, bool pinValue) {
	for (uint8_t t = 0; t < outputConfList.size(); t++) {   // Go through each pin in the output config-list
		OutputConf *outputConfig = outputConfList.get(t);
		if (outputConfig->pinNumber == pinNumber) {
			if (outputConfig->reversedOutput) pinValue = !pinValue;
			bool currentOutput = digitalRead(pinNumber);
			if (currentOutput != pinValue) {   // Only if we have a new value for our pin, we do something
				logger.send(DEBUG, "IOP", "Seting physical pin %d %s", pinNumber, pinValue ? "high" : "low");
				digitalWrite(pinNumber, pinValue);
				if (outputConfig->cbFunc != nullptr) {   // If we have a callback configured
					outputConfig->cbFunc(pinNumber, pinValue, ++outputConfig->triggerCount);   // Do the callback
				}
			}
			return(true);
		}
	}
	logger.send(ERR, "IOP", "Cant change output of %d because it's not setup yet", pinNumber);
	return(false);
}



/*	Public method to toggle the output of a pin. If it was high it becomes low and opposite. Parameters:
	pinNumber: The physical pin number
	Returns true if the provided pin has already been configured. Otherwise false
*/
bool EvtIO::outputToggle(uint8_t pinNumber) {
	for (uint8_t t = 0; t < outputConfList.size(); t++) {   // Go through each pin in the output config-list
		if (outputConfList.get(t)->reversedOutput) {
			return (outputSet(pinNumber, digitalRead(pinNumber)));
		} else {
			return (outputSet(pinNumber, !digitalRead(pinNumber)));
		}
	}
	return(false);
}



/*	These are the actual interrupts handlers. They contain as littel code as possible. They only counts the interrupts.
	The real handling is done in the taskHandleInterrupts. It's only possible to have a total of 10 pins that has an interrupt
	We need one function per interrupt because we cant otherwise destingish them from each other.
*/
void IRAM_ATTR EvtIO::handleHwInterrupt0() {
	portENTER_CRITICAL_ISR(&mux);   // We want to make sure nobody touches our variables while updating them
	interruptPin[0].interruptCount++;
	interruptPin[0].pinValue = digitalRead(interruptPin[0].pinNumber);
	portEXIT_CRITICAL_ISR(&mux);   // Now it's not critical anymore
}

void IRAM_ATTR EvtIO::handleHwInterrupt1() {
	portENTER_CRITICAL_ISR(&mux);
	interruptPin[1].interruptCount++;
	interruptPin[1].pinValue = digitalRead(interruptPin[1].pinNumber);
	portEXIT_CRITICAL_ISR(&mux);
}

void IRAM_ATTR EvtIO::handleHwInterrupt2() {
	portENTER_CRITICAL_ISR(&mux);
	interruptPin[2].interruptCount++;
	interruptPin[2].pinValue = digitalRead(interruptPin[2].pinNumber);
	portEXIT_CRITICAL_ISR(&mux);
}

void IRAM_ATTR EvtIO::handleHwInterrupt3() {
	portENTER_CRITICAL_ISR(&mux);
	interruptPin[3].interruptCount++;
	interruptPin[3].pinValue = digitalRead(interruptPin[3].pinNumber);
	portEXIT_CRITICAL_ISR(&mux);
}

void IRAM_ATTR EvtIO::handleHwInterrupt4() {
	portENTER_CRITICAL_ISR(&mux);
	interruptPin[4].interruptCount++;
	interruptPin[4].pinValue = digitalRead(interruptPin[4].pinNumber);
	portEXIT_CRITICAL_ISR(&mux);
}

void IRAM_ATTR EvtIO::handleHwInterrupt5() {
	portENTER_CRITICAL_ISR(&mux);
	interruptPin[5].interruptCount++;
	interruptPin[5].pinValue = digitalRead(interruptPin[5].pinNumber);
	portEXIT_CRITICAL_ISR(&mux);
}

void IRAM_ATTR EvtIO::handleHwInterrupt6() {
	portENTER_CRITICAL_ISR(&mux);
	interruptPin[6].interruptCount++;
	interruptPin[6].pinValue = digitalRead(interruptPin[6].pinNumber);
	portEXIT_CRITICAL_ISR(&mux);
}

void IRAM_ATTR EvtIO::handleHwInterrupt7() {
	portENTER_CRITICAL_ISR(&mux);
	interruptPin[7].interruptCount++;
	interruptPin[7].pinValue = digitalRead(interruptPin[7].pinNumber);
	portEXIT_CRITICAL_ISR(&mux);
}

void IRAM_ATTR EvtIO::handleHwInterrupt8() {
	portENTER_CRITICAL_ISR(&mux);
	interruptPin[8].interruptCount++;
	interruptPin[8].pinValue = digitalRead(interruptPin[8].pinNumber);
	portEXIT_CRITICAL_ISR(&mux);
}

void IRAM_ATTR EvtIO::handleHwInterrupt9() {
	portENTER_CRITICAL_ISR(&mux);
	interruptPin[9].interruptCount++;
	interruptPin[9].pinValue = digitalRead(interruptPin[9].pinNumber);
	portEXIT_CRITICAL_ISR(&mux);
}