#include "../common/HelperFunc.h"
#include "Global.h"
#include "MonitoringInstance.h"

USE_NS(NS_COMMON)

const char *MonitoringInstance::TAG_INSTANCE = "Instance";
const char *MonitoringInstance::ATTR_MAX_INSTANCE_COUNT = "MaxInstanceCount";
const char *MonitoringInstance::ATTR_VALID = "Valid";

MonitoringInstance::MonitoringInstance()
:MonitoringClass(1)
,valid(true)
{
}

int MonitoringInstance::init(DOMElement *node, string& errMsg)
{
	//MonitoringInstanceの関連属性を取得
	DOMNamedNodeMap *attributes = node->getAttributes();

	//Class属性
	className = HelperFunc::getAttrValueUtf8(attributes, ATTR_CLASS);
    if(className != NULL)
	{
		if (strlen(className) == 0)
		{
			errMsg = "DumpInstance's class name should not be null.";
			return -1;
		}
		if (strstr(className, "[]") != NULL)
		{
			errMsg = "DumpInstance's class name should not be array. class: ";
            errMsg.append(className);
			return -1;
		}
		else if (strcmp(className, "java.lang.Object") == 0)
		{
			errMsg = "DumpInstance's class name should not be java.lang.Object.";
			return -1;
		}
	}
	
	//ClassLoadString属性
	classLoadString = HelperFunc::getAttrValueUtf8(attributes, ATTR_CLASS_LOAD_STRING);
	
	//MaxInstanceCount属性
	char* valueField = HelperFunc::getAttrValueUtf8(attributes, ATTR_MAX_INSTANCE_COUNT);
	maxInstanceCount = 0;
	if (valueField != NULL)
	{
		maxInstanceCount = strtol(valueField, NULL, 10);
		//modified by Qiu Song on 20090810 for インスタンスダンプ制限の削除
		/*if ((maxInstanceCount > MAX_INSTANCE_COUNT) || (maxInstanceCount < 0))
		{
			maxInstanceCount = MAX_INSTANCE_COUNT;
		}*/

        if (maxInstanceCount < 0)
		{
			maxInstanceCount = 0;
		}
		//end of modified by Qiu Song on 20090810 for インスタンスダンプ制限の削除
		delete[] valueField;
		valueField = NULL;
	}
	
	//Valid属性
	char* pTmp = HelperFunc::getAttrValueUtf8(attributes, ATTR_VALID);
	if (NULL != pTmp)
	{
		if(strcmp(pTmp,"false") == 0)
		{
			valid = false;
		}
		delete[] pTmp;
		pTmp = NULL;
	}

	return 0;
}

int MonitoringInstance::operator==(const MonitoringClass &rhs) const
{
	if (rhs.nMonitoringType != nMonitoringType)
		return false;

    int nRet = strcmp(className, rhs.className);
	if (nRet == 0)
	{
		nRet = strcmp(classLoadString, rhs.classLoadString);
	}

	if (nRet == 0)
	{
		return (maxInstanceCount == rhs.maxInstanceCount);
	}
	else
	{
		return false;
	}
}

bool MonitoringInstance::isConflict(const MonitoringClass &rhs, bool& isSame)
{
	isSame = false;
    int nRet = strcmp(className, rhs.className);
	if (nRet != 0)
	{
		return false;
	}
	if (rhs.nMonitoringType == 2)
	{
		if (strcmp(classLoadString, rhs.classLoadString) == 0)
		{
			isSame = true;
			return false;
		}
		else if ((strstr(this->classLoadString, rhs.classLoadString) != NULL)
				|| (strstr(rhs.classLoadString, this->classLoadString) != NULL))
		{
			return true;
		}
	}
	else
	{
		if (strcmp(this->classLoadString, rhs.classLoadString) == 0)
		{
			isSame = true;
			return true;
		}
		else if ((strstr(this->classLoadString, rhs.classLoadString) != NULL)
				|| (strstr(rhs.classLoadString, this->classLoadString) != NULL))
		{
			return true;
		}
	}
	return false;
}

bool MonitoringInstance::needRedefine(MonitoringClass &rhs)
{
	if (rhs.getMonitoringType() != this->getMonitoringType())
	{
		return true;
	}
    return false;
}
