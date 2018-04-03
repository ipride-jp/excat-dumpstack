#include "../common/HelperFunc.h"
#include "Global.h"
#include "MonitoringClass.h"
#include <string.h>

USE_NS(NS_COMMON)

const char *MonitoringClass::ATTR_CLASS = "Class";
const char *MonitoringClass::ATTR_CLASS_LOAD_STRING = "ClassLoaderString";

MonitoringClass::MonitoringClass(int type)
: className(NULL)
, classLoadString(NULL)
//modified by Qiu Song on 20090909 for インスタンス制限の削除
//, maxInstanceCount(MAX_INSTANCE_COUNT)
, maxInstanceCount(0)
//end of modified by Qiu Song on 20090909 for インスタンス制限の削除
, nMonitoringType(type)
{
}

MonitoringClass::~MonitoringClass()
{
	if (className != NULL)
	{
		delete[] className;
		className = NULL;
	}
	
	if (classLoadString != NULL)
	{
		delete[] classLoadString;
		classLoadString = NULL;
	}
}

void MonitoringClass::logConfig()
{
	string buf;
	buf = "DumpInstance Setting";
	LOG4CXX_INFO(Global::logger, buf.c_str());
	
	buf = "  ClassName:";
	buf += className;
	LOG4CXX_INFO(Global::logger, buf.c_str());
	
	//info
	if(classLoadString != NULL)
	{
		buf = "  ClassLoadString:";
		buf += classLoadString;
		LOG4CXX_INFO(Global::logger, buf.c_str());
	}
	
	buf = "  MaxInstanceCount:";
	char szTmp[20];
	sprintf(szTmp, "%d", maxInstanceCount);
	buf += szTmp;
	LOG4CXX_INFO(Global::logger, buf.c_str());
}

int MonitoringClass::operator<(const MonitoringClass &rhs) const
{
    int nRet = strcmp(this->className, rhs.className);
	if (nRet == 0)
	{
		nRet = strcmp(this->classLoadString, rhs.classLoadString);
	}
	return nRet < 0;
}

