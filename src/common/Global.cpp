#include <jvmti.h>
#include "Global.h"
#include "HelperFunc.h"
#include "Define.h"

USE_NS(NS_COMMON)

Logger* Global::logger = Logger::getLogger();

jvmtiEnv* Global::jvmti = NULL;
JavaVM* Global::savedvm = NULL;
Config* Global::config = NULL;
int Global::curAttrDepth = 0;
bool Global::errorStatus = false;
bool Global::needEncode = true;
bool Global::isIBMJvm = false;
bool Global::isJrockitJvm = false;
//add by Qiu Song on 20090910 for openJDK対応
bool Global::isOpenJDK = false;
//end of add by Qiu Song on 20090910 for openJDK対応
unsigned char Global::encodeKey[ENCODE_KEY_NUMBER];
//Monitor Global::monitor;
jrawMonitorID  Global::configMonitor = NULL;

//added by gancl 
int Global::isJVMToUnload = 0;
int Global::monitorThreadStatus = -1;
int Global::dumpFileDelThreadstatus = -1;
//end of added by gancl 
bool Global::isTrial = false;

//add by Qiu Song on 20090825 for メソッド監視
//map<string , map<int,int>*>* Global::classFieldTable = NULL;
//map<string, map<int, string>*>* Global::classConstantPool = NULL;
//end of add by Qiu Song on 20090825 for メソッド監視

deque<SimpleStackTrace*>* Global::stackTraceCache = NULL;

Config* Global::getConfig()
{
	return Global::config;
}

void Global::setConfig(Config* newConfig)
{
	Global::config = newConfig;
}


void Global::setIsJrockitJvm(bool param)
{
	isJrockitJvm = param;
}

bool Global::getIsJrockitJvm()
{
    return isJrockitJvm;
}

void Global::setIsIBMJvm(bool param)
{
	isIBMJvm = param;
}

bool Global::getIsIBMJvm()
{
    return isIBMJvm;
}

void Global::setEncodeKey(unsigned char* key,int keyBytes)
{
	int i = 0;
	for(i=0;i<ENCODE_KEY_NUMBER && i < keyBytes;i++)
	{
         encodeKey[i] = key[i];
	}

	if(i < ENCODE_KEY_NUMBER)
	{
        for(;i<ENCODE_KEY_NUMBER ;i++)
		{
			encodeKey[i] = 0xFF;
		}
	}
}

