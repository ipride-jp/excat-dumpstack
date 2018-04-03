// SystemInfo.cpp: SystemInfo クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////
#ifdef _LINUX
#include <unistd.h>
#else
#include <windows.h>
#endif

#include <string.h>
#include "HelperFunc.h"
#include "Global.h"
#include "SystemInfo.h"


USE_NS(NS_COMMON)
USE_NS(NS_OUTPUT)
//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////
SystemInfo* SystemInfo::systemInfo = NULL;

SystemInfo::SystemInfo()
{
   systemID = NULL;
   jvmID = NULL;
   jvmIDEscaped =  NULL;
   jvmVersion = NULL;
   jvmName = NULL;
   jvmVendor = NULL;
   initialized = false;
}

SystemInfo::~SystemInfo()
{
    if(systemID != NULL)
	{
		delete systemID;
		systemID = NULL;
	}
    if(jvmID != NULL)
	{
		delete jvmID;
		jvmID = NULL;
	}
    if(jvmIDEscaped != NULL)
	{
		delete jvmIDEscaped;
		jvmIDEscaped = NULL;
	}

    if(jvmVersion != NULL)
	{
		delete jvmVersion;
		jvmVersion = NULL;
	}
    if(jvmName != NULL)
	{
		delete jvmName;
		jvmName = NULL;
	}
    if(jvmVendor != NULL)
	{
		delete jvmVendor;
		jvmVendor = NULL;
	}
}

void  SystemInfo::setJvmID(char* param)
{
	jvmID = HelperFunc::strdup(param);
	if(param != NULL)
	{
	    int charNumber = calEscapeXMLChar(param);
		escapeXMLChar(param,&jvmIDEscaped,charNumber);
	}
}

SystemInfo* SystemInfo::getInstance()
{
	if(systemInfo == NULL)
	{
		systemInfo = new SystemInfo();
	}

	return systemInfo;

}

void SystemInfo::deleteInstance()
{
    if(systemInfo != NULL)
	{
		delete systemInfo;
		systemInfo = NULL;
	}
}

void SystemInfo::getJVMInfo(JNIEnv* jni)
{
	if(initialized)
	{
		return ;
	}

    initialized = true;

	//find the inserted class and method
	jclass classSystem = jni->FindClass("java/lang/System");
	if(classSystem == NULL)
	{
        LOG4CXX_ERROR(Global::logger, "Can't get class from jni:java.lang.System.");
		return ;
	}
	jmethodID methodGetProperty = jni->GetStaticMethodID(classSystem,
		"getProperty","(Ljava/lang/String;)Ljava/lang/String;");
	if(methodGetProperty == NULL)
	{
        LOG4CXX_ERROR(Global::logger, "Can't get method from jni:getProperty.");
		return ;
	}

	//get java.version
    bool ret = getValueOfKey(jni,classSystem,methodGetProperty,
		"java.version",&jvmVersion);
	if(!ret)
	{
        LOG4CXX_ERROR(Global::logger, "Can't get java version from jni.");
		return ;
	}

	//get jvm name
	ret = getValueOfKey(jni,classSystem,methodGetProperty,
		"java.vm.name",&jvmName);
	if(!ret)
	{
        LOG4CXX_ERROR(Global::logger, "Can't get jvm name from jni.");
		return ;
	}

	//get java.vendor
	ret = getValueOfKey(jni,classSystem,methodGetProperty,
		"java.vendor",&jvmVendor);
	if(!ret)
	{
        LOG4CXX_ERROR(Global::logger, "Can't get jvm vendor from jni.");
		return ;
	}

	ret = getSystemName();
	if(!ret)
	{
        LOG4CXX_ERROR(Global::logger, "Can't get host name.");
		strcpy(systemID,"unknown");
		return ;
	}
	
}

bool SystemInfo::getSystemName()
{

	char* tempSystemID = new char[MAX_BUF_LEN + 1];
    bool bRet;
#ifdef _LINUX
    int ret = gethostname(tempSystemID,MAX_BUF_LEN);
	bRet= (ret == 0);
#else
 	DWORD dw = MAX_BUF_LEN ;
	bRet = GetComputerName(tempSystemID,&dw) != 0;
#endif
  
	if(bRet)
	{
        int charNumber = calEscapeXMLChar(tempSystemID);
		escapeXMLChar(tempSystemID,&systemID,charNumber);
	}
	delete[] tempSystemID;
	return bRet;
}

bool SystemInfo::getValueOfKey(JNIEnv* jni,jclass classSystem,
							   jmethodID methodGetProperty,char* key,char** value)
{

	jstring keyUTF = jni->NewStringUTF(key); 
	jstring valueUTF = (jstring)jni->CallStaticObjectMethod(
		classSystem,methodGetProperty,keyUTF);
	if(valueUTF == NULL)
	{
       return false;
	}
    char *temp = new char[MAX_BUF_LEN];
	temp[0] = 0;
    HelperFunc::getJStringValue(jni,valueUTF,temp,MAX_BUF_LEN);

	if(strlen(temp) > 0)
	{
		int charNumber = calEscapeXMLChar(temp);
		escapeXMLChar(temp,value,charNumber);
	}

	delete[] temp;

	return true;
}
