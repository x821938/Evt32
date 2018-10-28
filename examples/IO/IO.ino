#include "EvtLogger.h"
#include "EvtIO.h"

EvtIO evtIO;   // Our IO class instance

#define INPUT_PIN1 0
#define INPUT_PIN2 4
#define OUTPUT_PIN 21


void setup(void)
{
	logger.setup(INFO, false);   // We don't want to much logging

	// Set up a couple of inputs. Set up as internally pulled up.
	evtIO.trigger(INPUT_PIN1, INPUT_PULLUP, cbInput);
	evtIO.trigger(INPUT_PIN2, INPUT_PULLUP, cbInput);

	// And an output
	evtIO.outputSetup(OUTPUT_PIN, false, cbOutput);   // If second parameter is true, the physical state of the pin will be opposit of the value provided with outputSet.
	evtIO.outputSet(OUTPUT_PIN, false);   // Set the output pin low.

	while (true) {
		delay(1000);
		evtIO.outputToggle(OUTPUT_PIN);   // Toggle the output pin (if it was low it will become high)
	}
}


void loop(void)
{
	delay(1000);   // Do nothing forever
}


/* This gets called when an input pin changes */
void cbInput(uint8_t pinNumber, bool pinState, unsigned long triggerCount) {
	logger.send(NOTICE, "TST", "Input triggered on pin %d, TriggerCount=%d, Pin value=%d", pinNumber, triggerCount, pinState);
}


/* This gets called when an output pin changes */
void cbOutput(uint8_t pinNumber, bool pinState, unsigned long triggerCount) {
	logger.send(NOTICE, "TST", "Trigger because of output changed on pin %d, TiggerCount=%d, pin is %d, ", pinNumber, pinState, triggerCount);
}