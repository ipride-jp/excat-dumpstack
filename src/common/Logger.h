#pragma warning(disable : 4786) //åxçêC4786Çã÷é~
#if !defined(_LOGGER_H)
#define _LOGGER_H


#include <map>
#include <string>

#include "Define.h"

BEGIN_NS(NS_COMMON)

enum {
        FATAL_INT = 5,
        ERROR_INT = 4,
        WARN_INT = 3,
        INFO_INT = 2,
        DEBUG_INT = 1,
		TRACE_INT = 0
};

#define LOG4CXX_INFO(logger, message) {\
        if(logger->isInfoEnabled()) {\
             logger->info(message);}}

#define LOG4CXX_INFO_NATIVE(logger, message) {\
        if(logger->isInfoEnabled()) {\
             logger->infoNative(message);}}

#define LOG4CXX_WARN(logger, message) {\
        if(logger->isWarnEnabled()) {\
             logger->warn(message);}}

#define LOG4CXX_WARN_NATIVE(logger, message) {\
        if(logger->isWarnEnabled()) {\
             logger->warnNative(message);}}

#define LOG4CXX_ERROR(logger, message) {\
        if(logger->isErrorEnabled()) {\
             logger->error(message);}}

#define LOG4CXX_ERROR_NATIVE(logger, message) {\
        if(logger->isErrorEnabled()) {\
             logger->errorNative(message);}}

#define LOG4CXX_DEBUG(logger, message)   {\
        if(logger->isDebugEnabled()) {\
             logger->debug(message);}}

#define LOG4CXX_DEBUG_NATIVE(logger, message)   {\
        if(logger->isDebugEnabled()) {\
             logger->debugNative(message);}}

#define LOG4CXX_TRACE(logger, message)   {\
        if(logger->isTraceEnabled()) {\
             logger->trace(message);}}

#define LOG4CXX_TRACE_NATIVE(logger, message)   {\
        if(logger->isTraceEnabled()) {\
             logger->traceNative(message);}}

#define LOG4CXX_FATAL(logger, message)    logger->fatal(message)

#define LOG4CXX_FATAL_NATIVE(logger, message)    logger->fatalNative(message)

class Logger 
{
public:
    virtual ~Logger();

private:
    Logger();
	char* fileName;
	char* path;
	char* fullPath;
	int level;
	long maxFileSize;
    static Logger* instance;
	std::map<std::string,std::string> *msgIdMap;
	int vsnFormat(char *str, size_t str_m, const char *fmt, va_list ap);
    bool outputToConsole;

public:
	void setOutputToColsole(bool output){outputToConsole = output;};
	void setPath(char* logPath);
	void setLevel(char* logLevel);
	void setLevel(int logLevel);
	void setMaxFileSize(long size);
	void fatal(const char*msg);
	void error(const char*msg);
	void warn(const char*msg);
	void info(const char*msg);
	void debug(const char*msg);
	void trace(const char*msg);
	void fatalNative(const char*msg);
	void errorNative(const char*msg);
	void warnNative(const char*msg);
	void infoNative(const char*msg);
	void debugNative(const char*msg);
	void traceNative(const char*msg);
	void logFatal(const char*format, ...);
	void logError(const char*format, ...);
	void logWarn(const char*format, ...);
	void logInfo(const char*format, ...);
	void logDebug(const char*format, ...);
	void logTrace(const char*format, ...);
	void logMsgId(const char*msgId,...);
	void logFatalNative(const char*format, ...);
	void logErrorNative(const char*format, ...);
	void logWarnNative(const char*format, ...);
	void logInfoNative(const char*format, ...);
	void logDebugNative(const char*format, ...);
	void logTraceNative(const char*format, ...);
	void logMsgIdNative(const char*msgId,...);
	char* getLogLevelStr(int logLevel);
    inline bool isDebugEnabled(){return DEBUG_INT >= level;};
	inline bool isTraceEnabled(){return TRACE_INT >= level;};
	bool isInfoEnabled(){return INFO_INT >= level;};
	bool isWarnEnabled(){return WARN_INT >= level;};
	bool isErrorEnabled(){return ERROR_INT >= level;};
	
	static int  convertLevel(char* logLevel);
	static Logger* getLogger();
private:
	void genFullPath();
	void log(int logLevel,const char* msg,bool needConvertToNative);
	void backupLogFile();
	void initialMsg();
};

END_NS

#endif
