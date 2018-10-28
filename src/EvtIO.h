#ifndef _EVTINPUTD_h
#define _EVTINPUTD_h

#include <Arduino.h>
#include "EvtLogger.h"
#include <LinkedList.h>

#define IO_STACK_SIZE 5000


typedef void(*InputCbFunc) (uint8_t pinNumber, bool pinState, unsigned long triggerCount); // Define callback function
typedef void(*OutputCbFunc) (uint8_t pinNumber, bool pinState, unsigned long triggerCount); // Define callback function


/* Information stored about each interrupt pin */
struct Interrupt {
	uint8_t pinNumber;
	bool pinValue;
	uint64_t interruptCount=0;
	uint64_t lastInterruptCount=0;
	InputCbFunc inputCbFunction;
	unsigned long triggerCount=0;
};


/* Information storead about each output pin */
struct OutputConf {
	uint8_t pinNumber;
	bool reversedOutput;
	OutputCbFunc cbFunc;
	unsigned long triggerCount;
};


class EvtIO {
private:
	static portMUX_TYPE mux;
	static volatile Interrupt interruptPin[10];
	static uint8_t numOfTriggers;

	static void IRAM_ATTR handleHwInterrupt0();
	static void IRAM_ATTR handleHwInterrupt1();
	static void IRAM_ATTR handleHwInterrupt2();
	static void IRAM_ATTR handleHwInterrupt3();
	static void IRAM_ATTR handleHwInterrupt4();
	static void IRAM_ATTR handleHwInterrupt5();
	static void IRAM_ATTR handleHwInterrupt6();
	static void IRAM_ATTR handleHwInterrupt7();
	static void IRAM_ATTR handleHwInterrupt8();
	static void IRAM_ATTR handleHwInterrupt9();
	static void taskHandleInterrupts(void *pvParameters);

	LinkedList<OutputConf*> outputConfList;
public:
	EvtIO();
	bool trigger(uint8_t pinNumber, uint8_t mode, InputCbFunc cbFunction);
	bool outputSetup(uint8_t pinNumber, bool reversedOutput, OutputCbFunc cbFunc);
	bool outputSetup(uint8_t pinNumber, bool reversedOutput);
	bool outputSet(uint8_t pinNumber, bool pinValue);
	bool outputToggle(uint8_t pinNumber);
};


#endif

