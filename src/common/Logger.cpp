#pragma warning( disable : 4786 )

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include <sys/stat.h>
#include <sys/types.h>

#ifdef _LINUX
#include <dirent.h>
#else
#include "windows.h"
#endif

#include "Logger.h"
#include "HelperFunc.h"
#include "MessageFile.h"

#define MAXVARMSGLEN 1024
#define MAXLOGFILESIZE 100*1024

#define LOG_FILE_NAME "DumpStack.log"

using namespace std;
USE_NS(NS_COMMON)

Logger* Logger::instance = NULL;

//constructor
Logger::Logger()
{
	fileName = NULL;
    path = NULL;
    fullPath = NULL;
    level = INFO_INT;
	maxFileSize = MAXLOGFILESIZE;//default 100k
	outputToConsole = true;
	msgIdMap = new map<string,string>();
	initialMsg();
}

Logger::~Logger()
{
	if(fileName != NULL)
	{
		delete fileName;
		fileName = NULL;
	}

	if(path != NULL)
	{
		delete path;
		path = NULL;
	}

	if(fullPath != NULL)
	{
		delete fullPath;
		fullPath = NULL;
	}

	if(msgIdMap != NULL)
	{
		msgIdMap->clear();
	    delete msgIdMap;
		msgIdMap = NULL;
	}
}

void Logger::initialMsg()
{
    int msgLen = sizeof(MsgArray) / sizeof(char*);
	char *spacePos;
	string key,msg;

	for(int i = 0;i < msgLen; i++)
	{
		spacePos = strchr(MsgArray[i],' ');
        if(spacePos != NULL)
        {
			msg = MsgArray[i];
			key = msg.substr(0,spacePos - MsgArray[i]);
            msgIdMap->insert(pair<string, string>(key,msg));
		}

	}
}

Logger* Logger::getLogger()
{
	 if(instance == NULL)
	 {
         instance = new Logger();
	 }

	 return instance;
}

void Logger::setLevel(char* logLevel)
{
	level = convertLevel(logLevel);
}

void Logger::setLevel(int logLevel)
{
	level = logLevel;
}

void Logger::setMaxFileSize(long size)
{
    maxFileSize = size;
}

void Logger::setPath(char* logPath)
{
    if(logPath == NULL)
	{
#ifdef _LINUX
        logPath = "/usr/share";
#else
        logPath = "c:";
#endif
	}
	if(path != NULL)
	{
		delete path;
		path = NULL;
	}
	int len = strlen(logPath);
	path = new char[len + 1];

	strcpy(path,logPath);
	genFullPath();

}

void Logger::genFullPath()
{
    if(fullPath != NULL)
	{
		delete fullPath;
		fullPath = NULL;
	}
	int len = strlen(path);
	int len2 =  strlen(LOG_FILE_NAME);
	fullPath = new char[len + len2 + 2];

#ifdef _LINUX
	sprintf(fullPath,"%s/%s",path,LOG_FILE_NAME);
#else
	sprintf(fullPath,"%s\\%s",path,LOG_FILE_NAME);
#endif
    
}

int   Logger::convertLevel(char* logLevel)
{
    if(strcmp(logLevel,"fatal") == 0)
	{
		return FATAL_INT;
	}

    if(strcmp(logLevel,"error") == 0)
	{
		return ERROR_INT;
	}

    if(strcmp(logLevel,"warn") == 0)
	{
		return WARN_INT;
	}

    if(strcmp(logLevel,"info") == 0)
	{
  	    return INFO_INT;
	}

    if(strcmp(logLevel,"debug") == 0)
	{
		return DEBUG_INT;
	}

    if(strcmp(logLevel,"trace") == 0)
	{
		return TRACE_INT;
	}

    //default 
	return INFO_INT;
}

void Logger::fatal(const char*msg)
{
   log(FATAL_INT,msg,true);
}
void Logger::error(const char*msg)
{
	log(ERROR_INT,msg,true);
}
void Logger::warn(const char*msg)
{
	log(WARN_INT,msg,true);
}
void Logger::info(const char*msg)
{
	log(INFO_INT,msg,true);
}
void Logger::debug(const char*msg)
{
    log(DEBUG_INT,msg,true);
}

void Logger::trace(const char*msg)
{
    log(TRACE_INT,msg,true);
}

void Logger::fatalNative(const char*msg)
{
   log(FATAL_INT,msg,false);
}
void Logger::errorNative(const char*msg)
{
	log(ERROR_INT,msg,false);
}
void Logger::warnNative(const char*msg)
{
	log(WARN_INT,msg,false);
}
void Logger::infoNative(const char*msg)
{
	log(INFO_INT,msg,false);
}
void Logger::debugNative(const char*msg)
{
    log(DEBUG_INT,msg,false);
}

void Logger::traceNative(const char*msg)
{
    log(TRACE_INT,msg,false);
}

//the max length of output should <= 1023
void Logger::logFatal(const char*format,...)
{
   char buffer[MAXVARMSGLEN];
   va_list argPtr ;
   va_start( argPtr, format );
   vsnFormat(buffer, MAXVARMSGLEN,format, argPtr);
   va_end( argPtr ) ;

   fatal(buffer);
}

//the max length of output should <= 1023
void Logger::logFatalNative(const char*format,...)
{
   char buffer[MAXVARMSGLEN];
   va_list argPtr ;
   va_start( argPtr, format );
   vsnFormat(buffer, MAXVARMSGLEN,format, argPtr);
   va_end( argPtr ) ;

   fatalNative(buffer);
}

//the max length of output should <= 1023
void Logger::logError(const char*format,...)
{
   char buffer[MAXVARMSGLEN];
   va_list argPtr ;
   va_start( argPtr, format );
   vsnFormat(buffer, MAXVARMSGLEN,format, argPtr);
   va_end( argPtr ) ;

   error(buffer);
}

//the max length of output should <= 1023
void Logger::logErrorNative(const char*format,...)
{
   char buffer[MAXVARMSGLEN];
   va_list argPtr ;
   va_start( argPtr, format );
   vsnFormat(buffer, MAXVARMSGLEN,format, argPtr);
   va_end( argPtr ) ;

   errorNative(buffer);
}

//the max length of output should <= 1023
void Logger::logWarn(const char*format,...)
{
   char buffer[MAXVARMSGLEN];
   va_list argPtr ;
   va_start( argPtr, format );
   vsnFormat(buffer, MAXVARMSGLEN,format, argPtr);
   va_end( argPtr ) ;

   warn(buffer);
}

//the max length of output should <= 1023
void Logger::logWarnNative(const char*format,...)
{
   char buffer[MAXVARMSGLEN];
   va_list argPtr ;
   va_start( argPtr, format );
   vsnFormat(buffer, MAXVARMSGLEN,format, argPtr);
   va_end( argPtr ) ;

   warnNative(buffer);
}

//the max length of output should <= 1023
void Logger::logInfo(const char*format,...)
{
   char buffer[MAXVARMSGLEN];
   va_list argPtr ;
   va_start( argPtr, format );
   vsnFormat(buffer, MAXVARMSGLEN,format, argPtr);
   va_end( argPtr ) ;

   info(buffer);
}

//the max length of output should <= 1023
void Logger::logInfoNative(const char*format,...)
{
   char buffer[MAXVARMSGLEN];
   va_list argPtr ;
   va_start( argPtr, format );
   vsnFormat(buffer, MAXVARMSGLEN,format, argPtr);
   va_end( argPtr ) ;

   infoNative(buffer);
}

//the max length of output should <= 1023
void Logger::logDebug(const char*format,...)
{
   char buffer[MAXVARMSGLEN];
   va_list argPtr ;
   va_start( argPtr, format );
   vsnFormat(buffer, MAXVARMSGLEN,format, argPtr);
   va_end( argPtr ) ;

   debug(buffer);
}

//the max length of output should <= 1023
void Logger::logDebugNative(const char*format,...)
{
   char buffer[MAXVARMSGLEN];
   va_list argPtr ;
   va_start( argPtr, format );
   vsnFormat(buffer, MAXVARMSGLEN,format, argPtr);
   va_end( argPtr ) ;

   debugNative(buffer);
}

//the max length of output should <= 1023
void Logger::logTrace(const char*format,...)
{
   char buffer[MAXVARMSGLEN];
   va_list argPtr ;
   va_start( argPtr, format );
   vsnFormat(buffer, MAXVARMSGLEN,format, argPtr);
   va_end( argPtr ) ;

   trace(buffer);
}

//the max length of output should <= 1023
void Logger::logTraceNative(const char*format,...)
{
   char buffer[MAXVARMSGLEN];
   va_list argPtr ;
   va_start( argPtr, format );
   vsnFormat(buffer, MAXVARMSGLEN,format, argPtr);
   va_end( argPtr ) ;

   traceNative(buffer);
}

void Logger::log(int logLevel,const char* utfMsg,bool needConvertToNative)
{
	if(logLevel < level)
	{
		return;
	}

	char* nativeMsg = NULL;
	if(needConvertToNative)
	    nativeMsg = HelperFunc::utf8_to_native(utfMsg);
	
	const char* msg = (nativeMsg == NULL) ? utfMsg : nativeMsg;

	time_t nowtime;
	time(&nowtime);
	struct tm *gt;
	gt = localtime(&nowtime);

	char*logLevelStr = getLogLevelStr(logLevel);
	if(outputToConsole){
		printf("Excat_Log <%04d-%02d-%02d %02d:%02d:%02d> %5s - %s\n",gt->tm_year+1900,
			gt->tm_mon+1,gt->tm_mday,gt->tm_hour,gt->tm_min,gt->tm_sec,
			logLevelStr,msg);
	}

	//log file path is not setup
	if(fullPath == NULL)
	{
		if (nativeMsg != NULL)
		{ 
			delete[] nativeMsg;
			nativeMsg = NULL;
		}
		return;
	}

	long fileSize = HelperFunc::getFileSize(fullPath);
	if(fileSize >= maxFileSize)
	{
        backupLogFile();
	}

	FILE *fp = fopen(fullPath,"a");
	if(fp == NULL)
	{
		printf("Fatal Error:can't open file %s to write log.\n",fullPath);
		if (nativeMsg != NULL)
		{
			delete[] nativeMsg;
			nativeMsg = NULL;
		}
		return;
	}

	fprintf(fp,"<%04d-%02d-%02d %02d:%02d:%02d> %5s - %s\n",gt->tm_year+1900,
		gt->tm_mon+1,gt->tm_mday,gt->tm_hour,gt->tm_min,gt->tm_sec,
		logLevelStr,msg);
	if (nativeMsg != NULL)
	{
		delete[] nativeMsg;
		nativeMsg = NULL;
	}
	fclose(fp);
}


//backup current file with the name of DumpStack.log.n which n is the largest
//number in the directory
void Logger::backupLogFile()
{
	 long max = 0;

#ifdef _LINUX
     DIR *pdir;
     struct dirent *pent;
 	  
	 pdir=opendir(path);
	 if(pdir == NULL)
	 {
		 return;
	 }
     while ((pent=readdir(pdir)))
	 {
          if(strstr(pent->d_name,LOG_FILE_NAME) != NULL &&
			  strlen(pent->d_name) > strlen(LOG_FILE_NAME) + 1)
		  {
			  char* surfix = pent->d_name + strlen(LOG_FILE_NAME) + 1;
			  long number = strtol(surfix, NULL, 10);
			  if(number > max)
				  max = number;
		  }
	 }

	 closedir(pdir);
#else

    WIN32_FIND_DATA FindFileData;
    HANDLE hFind = NULL;
    char* findFiles = new char[strlen(fullPath) + 3];
	strcpy(findFiles,fullPath);
	strcat(findFiles,".*");

	hFind = FindFirstFile(findFiles, &FindFileData);
	BOOL ret = true;
	while(hFind != NULL && ret)
	{
          if(strstr(FindFileData.cFileName,LOG_FILE_NAME) != NULL &&
			  strlen(FindFileData.cFileName) > strlen(LOG_FILE_NAME) + 1)
		  {
			  char* surfix = FindFileData.cFileName + strlen(LOG_FILE_NAME) + 1;
			  long number = strtol(surfix, NULL, 10);
			  if(number > max)
				  max = number;
		  }
          ret= FindNextFile(hFind, &FindFileData);
	}
    FindClose(hFind);
	delete findFiles;
#endif

	max++;
	int len = strlen(fullPath);
	char* newFullPath = new char[len + 64];

    //add .n to file name
	sprintf(newFullPath,"%s.%d",fullPath,max);
	HelperFunc::renameFile(fullPath,newFullPath);
	delete newFullPath;
}


char* Logger::getLogLevelStr(int logLevel)
{
	switch(logLevel)
	{
	case FATAL_INT:
		return "FATAL";
	case ERROR_INT:
		return "ERROR";
	case WARN_INT:
		return "WARN";
	case INFO_INT:
		return "INFO";
	case DEBUG_INT:
		return "DEBUG";
	case TRACE_INT:
		return "TRACE";
	}

	return "";
}

void Logger::logMsgId(const char*msgId,...)
{
    if(msgId == NULL)
		return;

	string key = msgId;
    map<string,string>::const_iterator it = msgIdMap->find(key);
	if(it != msgIdMap->end())
	{
		string msg = (string)it->second;
	    char buffer[MAXVARMSGLEN];
	    va_list argPtr ;
	    va_start( argPtr, msgId ) ;
	    vsnFormat( buffer,MAXVARMSGLEN, msg.c_str(), argPtr ) ;
	    va_end( argPtr ) ;
		char msgKind = *msgId;
		switch(msgKind){
		case 'F':
			fatal(buffer);
			break;
		case 'E':
			error(buffer);
			break;
        case 'W':
			warn(buffer);
			break;
        default:
			info(buffer);
			break;

		}
	}
}

void Logger::logMsgIdNative(const char*msgId,...)
{
    if(msgId == NULL)
		return;

	string key = msgId;
    map<string,string>::const_iterator it = msgIdMap->find(key);
	if(it != msgIdMap->end())
	{
		string msg = (string)it->second;
	    char buffer[MAXVARMSGLEN];
	    va_list argPtr ;
	    va_start( argPtr, msgId ) ;
	    vsnFormat( buffer,MAXVARMSGLEN, msg.c_str(), argPtr ) ;
	    va_end( argPtr ) ;
		char msgKind = *msgId;
		switch(msgKind){
		case 'F':
			fatalNative(buffer);
			break;
		case 'E':
			errorNative(buffer);
			break;
        case 'W':
			warnNative(buffer);
			break;
        default:
			infoNative(buffer);
			break;

		}
	}
}

int Logger::vsnFormat(char *str, size_t str_m, const char *fmt, va_list ap)
{
#ifdef _LINUX
	int ret = vsnprintf(str,str_m,fmt,ap);
#else
	int ret = _vsnprintf(str,str_m,fmt,ap);
#endif
    str[str_m - 1] = '\0';

	return ret;
}


