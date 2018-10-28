#ifndef _EVTLOGGER_h
#define _EVTLOGGER_h

#include <Arduino.h>

#define LOGLEVEL 7

#define LOG_STACK_SIZE 2000
#define LOG_QUEUE_LENGTH 50   // Maximum number of logentries in the queue
#define LOG_LENGTH_SERVICE 5
#define LOG_LENGTH_MSG 100

#define MS_IN_DAY 86400000
#define MS_IN_HOUR 3600000
#define MS_IN_MINUTE 60000
#define MS_IN_SECOND  1000


enum LogLevels { EMERG, ALERT, CRIT, ERR, WARN, NOTICE, INFO, DEBUG };


struct LogMessage {
	long millis;
	LogLevels loglevel;
	char service[LOG_LENGTH_SERVICE];
	char msg[LOG_LENGTH_MSG];
};



class EvtLogger {
private:
	LogLevels _logLevel = DEBUG;    // If nobody does anything the default loglevel is the highest
	bool _showTrueTime;
	static QueueHandle_t logQueue;

	static void TaskShowLog(void *pvParameters);
public:
	EvtLogger();
	void send(LogLevels logLevel, char* service, char* format, ...);
	void setup(LogLevels logLevel, bool showTrueTime);
};


#endif

extern EvtLogger logger;
