#if !defined(_MONITORING_CLASS_H)
#define _MONITORING_CLASS_H

#include <vector>
#include <xercesc/dom/DOM.hpp>
#include "Define.h"

using namespace std;
#ifndef XERCESC20
XERCES_CPP_NAMESPACE_USE
#endif

BEGIN_NS(NS_COMMON)

class MonitoringClass
{
public:
	MonitoringClass(int type);
    virtual ~MonitoringClass();

	virtual int operator<(const MonitoringClass &rhs) const;
	virtual void logConfig();
	
	virtual int init(DOMElement *node, string& errMsg) = 0;
	virtual int operator==(const MonitoringClass &rhs) const  = 0;
	virtual bool isConflict(const MonitoringClass &rhs, bool& isSame)  = 0;
	virtual bool needRedefine(MonitoringClass &rhs)  = 0;
	
	char* getClassName(){return className;};
	char* getClassLoadString(){return classLoadString;};
	int   getMaxInstanceCount(){return maxInstanceCount;};
	void  setMaxInstanceCount(int count){maxInstanceCount = count;};
	int   getMonitoringType(){return  nMonitoringType;};
	
public:
	char* className;
	char* classLoadString;
	int   nMonitoringType;  //1:MonitoringInstance 2:MonitoringMethod 3:MonitoringMethod+MonitoringInstance
	int   maxInstanceCount;

	static const char* ATTR_CLASS;
	static const char* ATTR_CLASS_LOAD_STRING;
};

END_NS
#endif

