#include "HelperFunc.h"
#include "MonitoringMethod.h"
#include "ReadCfgFileException.h"
#include "ExcludeClass.h"
#include "ObjectAutoRelease.h"
#include "Global.h"
#include <strstream>
#include "../antlrparse/CcatLexer.hpp"
#include "../antlrparse/CcatParser.hpp"

USE_NS(NS_COMMON)
USE_NS(NS_ANTLRPARSE)

const char *MonitoringMethod::TAG_METHOD = "Method";
const char *MonitoringMethod::ATTR_SUFFIX = "Suffix";
const char *MonitoringMethod::ATTR_NAME = "Name";
const char *MonitoringMethod::ATTR_SIGNATURE = "Signature";
const char *MonitoringMethod::ATTR_CONDITION = "Condition";
const char *MonitoringMethod::ATTR_MAX_DUMP_COUNT = "MaxDumpCount";
//add by Qiu Song on 20090821 for 監視中止タスク
const char *MonitoringMethod::ATTR_VALID = "Valid";
const char *MonitoringMethod::ATTR_POSITION = "Position";
//end of add by Qiu Song on 20090821 for 監視中止タスク

MonitoringMethod::MonitoringMethod()
: MonitoringClass(2)
, classSuffix(NULL)
, methodInfo(NULL)
, outputSetting(NULL)
{
}

MonitoringMethod::~MonitoringMethod()
{
	if (classSuffix != NULL)
	{
		delete[] classSuffix;
		classSuffix = NULL;
	}

	if (methodInfo != NULL)
	{
		vector<MethodDumpCondition*>::iterator it;
		for(it = methodInfo->begin(); it != methodInfo->end(); it++ )
		{
			delete *it;
        }  
		methodInfo->clear();
		delete methodInfo;
		methodInfo = NULL;
	}
}

int MonitoringMethod::init(DOMElement *node, string& errMsg)
{
 	//MonitoringMethod関連の属性を取得
	DOMNamedNodeMap *attributes = node->getAttributes();

	className = HelperFunc::getAttrValueUtf8(attributes, ATTR_CLASS);
    if(className != NULL)
	{
		if (strlen(className) == 0)
		{
			errMsg = "Monitoring method's class name should not be null.";
			return -1;
		}
		if (strstr(className, "[]") != NULL)
		{
			errMsg = "Monitoring method's class name should not be array. class: ";
            errMsg.append(className);
			return -1;
		}
	}
	
	classLoadString = HelperFunc::getAttrValueUtf8(attributes, ATTR_CLASS_LOAD_STRING);

	classSuffix = HelperFunc::getAttrValueUtf8(attributes, ATTR_SUFFIX);
	if (false == Config::isValidPrefix(classSuffix))
	{
		errMsg = "Class suffix [";
		errMsg.append(classSuffix);
		errMsg.append("] is invalid.The char you can use is: a-z A-Z 1-9 - _");
		return -1;
	}

	//get constructor name
	char* constructorName = strrchr(className, '.');
	if (constructorName == NULL)
		constructorName = (char*)className;
	else
		constructorName += 1;

	methodInfo = new vector<MethodDumpCondition*>;

    //get fields 
	XMLCh *xmlTemp;
	DOMNodeList *fieldsNodes = node->getElementsByTagName(
	    xmlTemp = XMLString::transcode(TAG_METHOD));
	XMLString::release(&xmlTemp);
	int fieldsNum = fieldsNodes->getLength();
	for(int i=0; i < fieldsNum;i++)
	{
       DOMNamedNodeMap *attributesField = fieldsNodes->item(i)->getAttributes();

	   //add by Qiu Song on 20090821 for 監視中止タスク
	   //Validを取得する
	   char* methodValid = HelperFunc::getAttrValueUtf8(attributesField, ATTR_VALID);
	   if(methodValid != NULL && strcmp(methodValid, "false") == 0)
	   {
		   continue;
	   }
	   //end of add by Qiu Song on 20090821 for 監視中止タスク

	   //get method name
	   char* valueField = HelperFunc::getAttrValueUtf8(attributesField, ATTR_NAME);
	   char* methodName = valueField;
       if(valueField != NULL)
	   {
		    if (strlen(valueField) == 0)
			{
			    errMsg = "Method name should not be null.class: ";
				errMsg.append(className);
                errMsg.append(" classLoadString: ");
				errMsg.append(classLoadString);
				delete[] valueField;
				return -1;
			}
			if (strcmp(valueField, "Object") == 0)
			{
				if (strcmp(className, "java.lang.Object") == 0)
				{
					errMsg = "Class java.lang.Object's constructor should not be monitored.";
					delete[] valueField;
					return -1;
				}
			}
			if (strcmp(constructorName, valueField) == 0)
			{
				methodName = new char[7];
				strcpy(methodName, "<init>");
				delete[] valueField;
			}
		}
	    AUTO_REL_OBJECT(methodName);
		   
		//get method signature
		char* methodSuffix = HelperFunc::getAttrValueUtf8(attributesField, ATTR_SUFFIX);
		AUTO_REL_OBJECT(methodSuffix);

		if (false == Config::isValidPrefix(methodSuffix))
		{
			errMsg = "Method suffix [";
			errMsg.append(methodSuffix);
			errMsg.append("] is invalid.The char you can use is: a-z A-Z 1-9 - _");
			return -1;
		}
			
		//get method signature
		char* methodSignature = HelperFunc::getAttrValueUtf8(attributesField, ATTR_SIGNATURE);
		AUTO_REL_OBJECT(methodSignature);
			
        //get dump condition
		char* dumpCondition = HelperFunc::getAttrValueUtf8(attributesField, ATTR_CONDITION);
			
		int dumpCount = 0;
		valueField = HelperFunc::getAttrValueUtf8(attributesField, ATTR_MAX_DUMP_COUNT);
		if (valueField != NULL)
		{
			dumpCount = strtol(valueField, NULL, 10);
			delete[] valueField;
			valueField = NULL;
		}

		//add by Qiu Song on 20090828 for メソッド監視
		char* dumpPosition = HelperFunc::getAttrValueUtf8(attributesField, ATTR_POSITION);
		int nPosition = 1;
		if(dumpPosition != NULL)
		{
			if(strcmp(dumpPosition, "start") == 0)
			{
				nPosition = 1;
			}
			else if(strcmp(dumpPosition, "finish") == 0)
			{
				nPosition = 2;
			}
			else if(strcmp(dumpPosition, "both") == 0)
			{
				nPosition = 0;
			}
			else
			{
				nPosition = 1;
			}
		}
		else
		{
			nPosition = 1;
		}
		//end of add by Qiu Song on 20090828 for メソッド監視

		MethodDumpCondition* target = new MethodDumpCondition(methodName,methodSignature,methodSuffix,
															  dumpCondition,dumpCount, nPosition);
		if (dumpCondition != NULL)
		{
			delete[] dumpCondition;
			dumpCondition = NULL;
		}

		//insert into the proper place
		vector<MethodDumpCondition*>::iterator p;
		for (p = methodInfo->begin(); p != methodInfo->end(); ++p) 
		{
			//confict definition check
			bool ret = target->isConflict(**p);
			if (ret == true)
			{
			    errMsg = "Duplicate method definition is exist.";
				errMsg.append("  class: ");
				errMsg.append(className);
				errMsg.append(" classLoadString: ");
				errMsg.append(classLoadString);
				errMsg.append(" methodName: ");
				errMsg.append(methodName);
				delete target;
				return -1;
			}

			if (*target < **p) 
				break;
		}

		//target->getDumpCondition()に対して、文法チェックをおこなう。
		//失敗したら、targetのメモリを解放して、エラーメッセージを出力して、
		//-1を返却する
		dumpCondition = target->getDumpCondition();
		if ((dumpCondition != NULL) && (strlen(dumpCondition) > 1))
		{
			istrstream str(dumpCondition, strlen(dumpCondition));
			CcatLexer lexer(str);
			CcatParser parser(lexer);
			char* pszMsg = new char[128 + 1];
			memset(pszMsg, 0, 128 + 1);
            bool bRet = parser.expr(pszMsg, true, NULL);
			if (!bRet)
			{
				string buf = "Condition \"";
				buf += dumpCondition;
				errMsg = buf.substr(0, buf.length() -1);
				errMsg +=  "\" is wrong. ";
				errMsg += pszMsg;
				errMsg += " (class: ";
				errMsg += className;
                errMsg += " classLoadString: ";
				errMsg += classLoadString;
                errMsg += " methodName: ";
				errMsg += methodName;
                errMsg += " methodSignature: ";
				errMsg += methodSignature;
                errMsg += ")";

				delete [] pszMsg;
				delete target;
				return -1;
			}
			delete [] pszMsg;
		}
		methodInfo->insert(p, target);
	}
	return 0;
}

//you should call this function after config file is read
void MonitoringMethod::logConfigForMethod()
{
	string buf = "Monitor the method in class: ";
	buf.append(className);
	buf.append(" classLoadString: ");
	buf.append(classLoadString);
	buf.append(" suffix: ");
	buf.append(classSuffix);
	LOG4CXX_INFO(Global::logger, buf.c_str());
	if (methodInfo != NULL)
	{
		vector<MethodDumpCondition*>::iterator it;
		for(it = methodInfo->begin(); it != methodInfo->end(); it++ )
		{
			(*it)->logConfig();
        }  
	}
	if (outputSetting != NULL)
		outputSetting->logConfig();
}

void MonitoringMethod::addMonitoringInstance(MonitoringInstance* p)
{
	this->nMonitoringType = 3;
	this->setMaxInstanceCount(p->maxInstanceCount);
}

bool MonitoringMethod::isConflict(const MonitoringClass &rhs, bool& isSame)
{
	isSame = false;
    if (strcmp(className, rhs.className) != 0)
		return false;

    if (strcmp(classLoadString, rhs.classLoadString) == 0)
	{
		isSame = true;
		return true;
	}

    if ((strlen(classLoadString) == 0) 
		|| (strlen(rhs.classLoadString) == 0))
		return true;

	if ((strstr(classLoadString, rhs.classLoadString) != NULL)
		|| (strstr(rhs.classLoadString, classLoadString) != NULL))
		return true;

	return false;
}

int MonitoringMethod::operator==(const MonitoringClass &rhs) const
{
	if (rhs.nMonitoringType != nMonitoringType)
		return false;

	const MonitoringMethod& tmp = (const MonitoringMethod&)rhs;

    if (strcmp(className, tmp.className) != 0)
		return false;

    if (strcmp(classLoadString, tmp.classLoadString) != 0)
		return false;

	if (maxInstanceCount != rhs.maxInstanceCount)
		return false;

    if (strcmp(classSuffix, tmp.classSuffix) != 0)
		return false;

    if (methodInfo->size() != tmp.methodInfo->size())
		return false;

	int methodCount = methodInfo->size();
	MethodDumpCondition* p1;
	MethodDumpCondition* p2;
	for (int i=0; i < methodCount; i++)
	{
		p1 = methodInfo->at(i);
		p2 = tmp.methodInfo->at(i);
		if (*p1 == *p2)
		{
		}
		else
		{
			return false;
		}
    }

    if (*(this->outputSetting) == *(tmp.outputSetting))
		return true;

    return false;
}

bool MonitoringMethod::needRedefine(MonitoringClass &rhs)
{
	if (rhs.getMonitoringType() != this->getMonitoringType())
		return true;

	MonitoringMethod& ref = (MonitoringMethod&)rhs;

    if (methodInfo->size() != ref.methodInfo->size())
		return true;

	int methodCount = methodInfo->size();
	MethodDumpCondition* p1;
	MethodDumpCondition* p2;
	for (int i=0; i < methodCount; i++)
	{
		p1 = methodInfo->at(i);
		p2 = ref.methodInfo->at(i);
		if ((strcmp(p1->getMethodName(), p2->getMethodName()) != 0)
			|| (strcmp(p1->getMethodSignature(), p2->getMethodSignature()) != 0))
		{
			return true;
		}
    }
    return false;
}


MethodDumpCondition::MethodDumpCondition() 
: methodName(NULL)
, methodSignature(NULL)
, methodSuffix(NULL)
, dumpCondition(NULL)
, maxDumpCount(1)
, dumpPosition(1)
{
}

MethodDumpCondition::MethodDumpCondition(char* name, char* signature, char* suffix, 
										 char* condition, int maxdumpCount, int position)
{
    if (name != NULL)
	{
		methodName = new char[strlen(name)+1];
		strcpy(methodName, name);
	}
    if (signature != NULL)
	{
		methodSignature = new char[strlen(signature)+1];
		strcpy(methodSignature, signature);
	}

    if (suffix != NULL)
	{
		methodSuffix = new char[strlen(suffix)+1];
		strcpy(methodSuffix, suffix);
	}

    if (condition != NULL)
	{
		dumpCondition = new char[strlen(condition)+2];
		strcpy(dumpCondition, condition);
		strcat(dumpCondition, ";");
	}
	this->maxDumpCount = maxdumpCount;
	if (maxdumpCount <= 0)
	{
		this->maxDumpCount = 0;  //use default value
	}

	//add by Qiu Song on 20090828 for メソッド監視
	dumpPosition = position;
	//end of add by Qiu Song on 20090828 for メソッド監視
}

MethodDumpCondition::~MethodDumpCondition()
{
	if (methodName != NULL)
	{
		delete[] methodName;
		methodName = NULL;
	}
	if (methodSignature != NULL)
	{
		delete[] methodSignature;
		methodSignature = NULL;
	}
	if (methodSuffix != NULL)
	{
		delete[] methodSuffix;
		methodSuffix = NULL;
	}
	if (dumpCondition != NULL)
	{
		delete[] dumpCondition;
		dumpCondition = NULL;
	}
}

int MethodDumpCondition::operator==(const MethodDumpCondition &rhs) const
{
    if (strcmp(methodName, rhs.methodName) != 0)
		return false;

	if (strcmp(methodSignature, rhs.methodSignature) != 0)
		return false;

	if (strcmp(methodSuffix, rhs.methodSuffix) != 0)
		return false;

	if (strcmp(dumpCondition, rhs.dumpCondition) != 0) 
		return false;

	return (maxDumpCount == rhs.maxDumpCount);
}

bool MethodDumpCondition::isConflict(const MethodDumpCondition &rhs)
{
    if (strcmp(methodName, rhs.methodName) != 0)
		return false;

    if (strcmp(methodSignature, rhs.methodSignature) == 0)
		return true;

    if ((strlen(methodSignature) == 0) 
		|| (strlen(rhs.methodSignature) == 0))
		return true;

	return false;
}

//you should call this function after config file is read
void MethodDumpCondition::logConfig()
{
	string buf = " Monitoring method info:";
    LOG4CXX_INFO(Global::logger, buf.c_str());

    buf = "    method name: ";
	buf.append(methodName);
    LOG4CXX_INFO(Global::logger, buf.c_str());

    buf = "    method signature: ";
	buf.append(methodSignature);
    LOG4CXX_INFO(Global::logger, buf.c_str());

    buf = "    dump condition: ";
	buf.append(dumpCondition);
    LOG4CXX_INFO(Global::logger, buf.substr(0, buf.length() -1).c_str());

    buf = "    max dump count: ";
	char szTmp[20];
	sprintf(szTmp, "%d", maxDumpCount);
	buf.append(szTmp);
    LOG4CXX_INFO(Global::logger, buf.c_str());

	buf = "    method suffix: ";
	buf.append(methodSuffix);
	LOG4CXX_INFO(Global::logger, buf.c_str());
}

MonitoringMethodInfo::MonitoringMethodInfo(MonitoringMethod* method)
{
    outputSetting = method->getOutputSetting();
	classLoadString = method->getClassLoadString();
	classSuffix = method->getClassSuffix();

	methodInfo = new map<string, MethodDumpInfo*>;

	vector<MethodDumpCondition*>* conditions = method->getMethodInfo();
    vector<MethodDumpCondition*>::iterator it;
	for (it = conditions->begin(); it != conditions->end(); it++)
	{
		char* methodName = (*it)->getMethodName();
		char* methodSig = (*it)->getMethodSignature();
        MethodDumpInfo* info = new MethodDumpInfo(*it);
		string key = methodName;
		key.append(methodSig);
		methodInfo->insert(pair<string,MethodDumpInfo*>(key, info));
	}
}

MonitoringMethodInfo::~MonitoringMethodInfo()
{
    if (methodInfo != NULL)
	{
		map<string, MethodDumpInfo*>::const_iterator it;
		for (it = methodInfo->begin(); it != methodInfo->end(); it++)
		{
			delete it->second;
		}
		methodInfo->clear();
		delete methodInfo;
		methodInfo = NULL;
	}
}

MethodDumpInfo* MonitoringMethodInfo::getMethodDumpInfo(char* methodName, char* methodSig)
{
    map<string, MethodDumpInfo*>::const_iterator it;
	
	//modified by Qiu Song on 20090915 for 監視関数曖昧指定
	string key = methodName;
	key.append(methodSig);

    it = methodInfo->find(key);
	if (it == methodInfo->end())
	{
	    it = methodInfo->begin();
		for (; it != methodInfo->end(); it++)
		{
			string key = it->first;
			if(HelperFunc::doesStringMatch(methodName, key.c_str()) == true)
			{
				return it->second;
			}
		}
		return NULL;
	}
	return it->second;

    return NULL;
    //end of modified by Qiu Song on 20090915 for 監視関数曖昧指定
}


MethodDumpInfo::MethodDumpInfo(MethodDumpCondition* conditionDef)
: bValid(true)
, dumpCount(0)
{
    maxDumpCount = conditionDef->getMaxDumpCount();
	dumpCondition = conditionDef->getDumpCondition();
	methodSuffix = conditionDef->getMethodSuffix();
}


MonitoringClassLoadInfo::MonitoringClassLoadInfo()
: classUrls(NULL)
, classLoaders(NULL)
{
}

MonitoringClassLoadInfo::~MonitoringClassLoadInfo()
{
	if (classUrls != NULL)
	{		
		classUrls->clear();
		delete classUrls;
        classUrls = NULL;
	}

	if (classLoaders != NULL)
	{		
		classLoaders->clear();
		delete classLoaders;
        classLoaders = NULL;
	}
}

void MonitoringClassLoadInfo::addClassUrl(string url)
{
	if (classUrls == NULL)
	{
		classUrls = new vector<string>;
		classUrls->push_back(url);
	}
	else
	{
		int nRet = -1;
		vector<string>::iterator it;
		for (it=classUrls->begin(); it != classUrls->end(); it++)
		{
			nRet = strcmp(url.c_str(), (*it).c_str());
			if (nRet < 0)
			{
				classUrls->insert(it, url);
				break;
			}
			else if (nRet == 0)
			{
				return;
			}
		}
	}
}

void MonitoringClassLoadInfo::addClassLoader(JNIEnv* jni, jobject loader)
{
	if (classLoaders == NULL)
	{
		classLoaders = new vector<jobject>;
		classLoaders->push_back(loader);
	}
	else
	{
        vector<jobject>::iterator it;
		for (it=classLoaders->begin(); it != classLoaders->end(); it++)
		{
			if (jni->IsSameObject(*it, loader))
			{
				break;
			}
		}
		if (it == classLoaders->end())
		{
			classLoaders->push_back(loader);
		}
	}
}

 void MonitoringClassLoadInfo::addClassUrls(vector<string>* urls)
{
	if (urls == NULL)
		return;

	if (classUrls == NULL)
	{
		classUrls = new vector<string>;
	}

	vector<string>::iterator it;
	for (it = classUrls->begin(); it != classUrls->end(); it++)
	{
		classUrls->push_back(*it);
	}
}
