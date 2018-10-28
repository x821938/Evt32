#include "EvtLogger.h"
#include "time.h"


// All logging goes through this queue
QueueHandle_t EvtLogger::logQueue = xQueueCreate(LOG_QUEUE_LENGTH, sizeof(LogMessage));



/* Creates a task the will handle the log queue */
EvtLogger::EvtLogger() {
	// Serial has to be up before spitting out messages
	Serial.begin(115200);
	delay(100);

	xTaskCreate(
		TaskShowLog,			// Task function.
		"showLog",				// Name of task.
		LOG_STACK_SIZE,			// Stack size in words
		(void*)this,			// We need to give the static method TaskShowLog a reference to the instance of this class
		1,						// Priority of the task.
		NULL);
	send(DEBUG, "LOG", "Logger task started");
}



/*	Sets the level of detail you want the logger to output. Parameters:
	loglevel: can be EMERG, ALERT, CRIT, ERR, WARN, NOTICE, INFO, DEBUG
	showTrueTime: if true the real human time (from NTP) is showed for each logentry. Otherwise just time since boot.
*/
void EvtLogger::setup(LogLevels logLevel, bool showTrueTime) {
	_logLevel = logLevel;
	_showTrueTime = showTrueTime;
}



// Task that keeps emptying and printing the log queue forever
void EvtLogger::TaskShowLog(void *pvParameters) {
	tm time;
	EvtLogger inst = *((EvtLogger*)pvParameters);   // We are inside static method. We need to be able to reference the instance.
	const char* logTexts[] = { "EMERG", "ALERT", "CRIT", "ERR", "WARN", "NOTICE", "INFO", "DEBUG" };
	char formattedLog[20+LOG_LENGTH_SERVICE+LOG_LENGTH_MSG]; // This will hold the complete formattet logline destined for serial.

	LogMessage logMessage;
	while (true) {
		xQueueReceive(logQueue, &logMessage, portMAX_DELAY); // A blocking read from queue. If its empty, we will just sit waiting here

		EvtLogger inst = *((EvtLogger*)pvParameters);   // The instance of the class
		if (logMessage.loglevel <= inst._logLevel) {   // Only if we have a logentry lower or equal to the loglevel, we handle it
			// Make the log message look nice
			if (inst._showTrueTime && getLocalTime(&time, 0)) {   // If we can read RTC clock we format the log with real human time.
				sprintf(formattedLog, "%04d-%02d-%02d %02d:%02d:%02d %-6s (%-3s) %s", \
					time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec, \
					logTexts[logMessage.loglevel], logMessage.service, logMessage.msg);
			}
			else {   // Otherwise we just format the time since boot.
				unsigned long logTime = logMessage.millis;
				unsigned short days = logTime / MS_IN_DAY;
				unsigned char hours = (logTime % MS_IN_DAY) / MS_IN_HOUR;
				unsigned char minutes = ((logTime % MS_IN_DAY) % MS_IN_HOUR) / MS_IN_MINUTE;
				unsigned char seconds = (((logTime % MS_IN_DAY) % MS_IN_HOUR) % MS_IN_MINUTE) / MS_IN_SECOND;
				unsigned short ms = ((((logTime % MS_IN_DAY) % MS_IN_HOUR) % MS_IN_MINUTE) % MS_IN_SECOND);
				sprintf(formattedLog, "%03u:%02u:%02u:%02u:%03u %-6s (%-3s) %s", days, hours, minutes, seconds, ms, logTexts[logMessage.loglevel], logMessage.service, logMessage.msg);
			}
			Serial.println(formattedLog);
		}
	}
}



/*	Adds a log entry to the log queue. Parameters:
	logLevel: can be EMERG, ALERT, CRIT, ERR, WARN, NOTICE, INFO, DEBUG 
	service: a string representing the category the log entry belongs to 
	format: a formatting string (printf compatible) 
	...: all the values that should be formatted
*/
void EvtLogger::send(LogLevels logLevel, char* service, char* format, ...) {
	if (logLevel <= DEBUG && strlen(service) < LOG_LENGTH_SERVICE) {   //  Only valid log entries are handled
		// Make a formattet print of the incoming parameters
		va_list arg;
		va_start(arg, format); 
		char temp[LOG_LENGTH_MSG];
		char* buffer = temp;
		size_t len = vsnprintf(temp, sizeof(temp), format, arg);  // Format the log entry. If it's too long it will be truncated
		va_end(arg);

		// Make the log entry ready for the queue (in a LogMessage struct)
		LogMessage logMessage;
		logMessage.millis = millis();
		logMessage.loglevel = logLevel;
		strncpy(logMessage.service, service, LOG_LENGTH_SERVICE);
		strncpy(logMessage.msg, temp, LOG_LENGTH_MSG);

		xQueueSend(logQueue, &logMessage, 0); // Add it to queue. If it's full just discard the logentry.
	}
}


EvtLogger logger;   // We always want an instance of the logger