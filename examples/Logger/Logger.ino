/*	A demonstation example of log level filtering. 
	If you want true time for each logentry you need wifi connection. See the example in "TimeNet" 
*/

#include "EvtLogger.h"   // This automatically gives an instance of the class called "logger"


void setup(void)
{
	logger.setup(DEBUG, false);		// All log entries will be shown because DEBUG is the lowest possible loglevel.
	showSomeLogs();

	Serial.println();

	logger.setup(ERR, false);		// After this point the logger will only show messages with higher level than ERR.
	showSomeLogs();
}


void loop(void)
{
	delay(1000);   // Do nothing forever
}


void showSomeLogs() {
	uint8_t i;
	logger.send(DEBUG, "SHO", "Here is a debug message (number %d)", i++);
	delay(random(0, 500));
	logger.send(INFO, "SHO", "Here is a info message (number %d)", i++);
	delay(random(0, 500));
	logger.send(NOTICE, "SHO", "Here is a notice message (number %d)", i++);
	delay(random(0, 500));
	logger.send(WARN, "SHO", "Here is a warning message (number %d)", i++);
	delay(random(0, 500));
	logger.send(ERR, "SHO", "Here is a error message (number %d)", i++);
	delay(random(0, 500));
	logger.send(CRIT, "SHO", "Here is a critical message (number %d)", i++);
	delay(random(0, 500));
	logger.send(ALERT, "SHO", "Here is a alert message (number %d)", i++);
	delay(random(0, 500));
	logger.send(EMERG, "SHO", "Here is an emergency message (number %d)", i++);
	delay(random(0, 500));
}