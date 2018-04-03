#pragma warning( disable : 4786 )
#include <jni.h>
#include <string.h>
#include <assert.h>
#include "Logger.h"
#include "HelperFunc.h"
#include "Config.h"
#include "Global.h"
#include "ReadCfgFileException.h"
#include "ObjectAutoRelease.h"
#include <locale.h>
#include <iostream>

#ifdef XERCESC20
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/parsers/AbstractDOMParser.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>
#include <xercesc/dom/DOMImplementationRegistry.hpp>
#include <xercesc/dom/DOMBuilder.hpp>
#include <xercesc/dom/DOMException.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMError.hpp>
#include <xercesc/dom/DOMLocator.hpp>
#else
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>
#include <xercesc/dom/DOMWriter.hpp>
#include <xercesc/framework/StdOutFormatTarget.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/XMLUni.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#endif

USE_NS(NS_COMMON)
#ifndef XERCESC20
XERCES_CPP_NAMESPACE_USE
#endif

string Config::errorMsg = "";
const char *Config::TAG_TASK = "Task";
const char *Config::TAG_MONITORING_TARGET = "MonitoringTarget";
const char *Config::TAG_MONITORING_METHOD = "MonitoringMethod";
const char *Config::TAG_OBJECTELEMENT = "ObjectElement";
const char *Config::TAG_OBJECT = "Object"; 
const char *Config::TAG_OUTPUT_SETTING = "DumpData";
const char *Config::TAG_SLEEP_SETTING = "CheckConfig";
const char *Config::TAG_LOG = "Log";
const char *Config::TAG_OTHERS= "Others";
const char *Config::TAG_DUMPFILE = "DumpFile";
const char *Config::TAG_MONITORSIGNAL = "MonitoringSignal";
const char *Config::ATTR_PATH = "Path";
const char *Config::ATTR_PREFIX = "Prefix";
const char *Config::ATTR_MIN_DISK_SPACE = "MinDiskSpace";
const char *Config::ATTR_SLEEP = "Sleep";
const char *Config::TAG_MAXDUMPDATA_SETTING = "MaxDumpData";
const char *Config::ATTR_LIMIT = "Limit";
const char *Config::ATTR_LOGPATH = "Path";
const char *Config::ATTR_LOGLEVEL = "Level";
const char *Config::TAG_MAIL = "Mail";
const char *Config::TAG_CHECK_DUPLICATION = "CheckDuplication";
const char *Config::ATTR_DUMP_DUPLICATION_WHEN_THREAD_DIFF = "DumpDuplicationWhenThreadDiff";
const char *Config::ATTR_TIME_LIMIT = "TimeLimit";
const char *Config::TAG_DUMP_INSTANCE = "DumpInstance";
const char *Config::TAG_INSTANCE = "Instance";
const char *Config::ATTR_DUMP_KIND = "DumpKind";
const char *Config::ATTR_THREAD_STATUS = "Status";//add by Qiu Song on 20090821 for 指定状態のスレッドダウンロード
const char *Config::ATTR_THREAD_PRIORITY = "ThreadPriority";//add by Qiu Song on 20090821 for 指定状態のスレッドダウンロード
//add by Qiu Song on 20090821 for 監視中止タスク
const char *Config::ATTR_VALID = "Valid";
const char *Config::ATTR_SUFFIX = "Suffix";
//end of add by Qiu Song on 20090821 for 監視中止タスク

Config::Config(): monitoringTargets(NULL), monitoringClasses(NULL), outputSetting(NULL),
   sleepTime(0),configFileFullPath(NULL),dumpObjectTemplates(NULL),dumpObjectMap(NULL),
   dumpFilePath(NULL),dumpFilePathUtf8(NULL),dumpFilePrefix(NULL),outputSettingMap(NULL),
   outputSettingList(NULL),	maxDumpData(0),logPath(NULL),logLevel(INFO_INT),
   monitorSignal(false),signalDumpKind(1),threadStatus(NULL),signalOutputSetting(NULL),logPathUtf8(NULL),
   mailSettingMap(NULL), checkDuplication(false), dumpDuplicationWhenThreadDiff(true),
   timeLimit(0),minDiskSpace(MIN_DISK_SIZE),monitorAllException(true), threadPriority(0)
{
}

Config::~Config()
{
	if (monitoringTargets != NULL)
	{
		vector<MonitoringTarget*>::iterator it;
		for(it = monitoringTargets->begin(); it != monitoringTargets->end(); it++ )
		{
			delete *it;
        }  

		monitoringTargets->clear();
		delete monitoringTargets;
		monitoringTargets = NULL;
	}

	if (monitoringClasses != NULL)
	{
		vector<MonitoringClass*>::iterator it;
		for(it = monitoringClasses->begin(); it != monitoringClasses->end(); it++ )
		{
			delete *it;
        }
		monitoringClasses->clear();
		delete monitoringClasses;
		monitoringClasses = NULL;
	}

	if(outputSettingList != NULL)
	{
		vector<OutputSetting*>::iterator it;
		for(it = outputSettingList->begin(); it != outputSettingList->end(); it++ )
		{
			delete *it;
        }  

		outputSettingList->clear();
		delete outputSettingList;
		outputSettingList = NULL;
	}

	if(dumpObjectTemplates != NULL)
	{
		vector<DumpObject*>::iterator it;
		for(it = dumpObjectTemplates->begin(); it != dumpObjectTemplates->end(); it++ )
		{
			delete *it;
        }  

		dumpObjectTemplates->clear();
		delete dumpObjectTemplates;
		dumpObjectTemplates = NULL;
	}

	if(dumpObjectMap != NULL)
	{
        dumpObjectMap->clear();
		delete dumpObjectMap;
		dumpObjectMap = NULL;
	}
	
    if(outputSettingMap != NULL)
	{
        outputSettingMap->clear();
		delete outputSettingMap;
		outputSettingMap = NULL;
	}


	if(configFileFullPath != NULL)
	{
		delete configFileFullPath;
		configFileFullPath = NULL;
	}
	if ((dumpFilePath != NULL)
		&& (dumpFilePath != dumpFilePathUtf8))
	{
			delete dumpFilePath;
			dumpFilePath = NULL;
	}
	if(dumpFilePathUtf8 != NULL)
	{
		delete dumpFilePathUtf8;
		dumpFilePathUtf8 = NULL;
	}
	if(dumpFilePrefix != NULL)
	{
		delete dumpFilePrefix;
		dumpFilePrefix = NULL;
	}

	if ((logPath != NULL)
		&& (logPath != logPathUtf8))
	{
		delete logPath;
		logPath = NULL;
	}

	if(logPathUtf8 != NULL)
	{
		delete logPathUtf8;
		logPathUtf8 = NULL;
	}

	if (mailSettingMap != NULL) {
		mailSettingMap->clear();
		delete mailSettingMap;
		mailSettingMap = NULL;
	}

	//add by Qiu Song on 20090819 for 指定タイプのスレッドダンプ
	if(threadStatus != NULL)
	{
		delete[] threadStatus;
		threadStatus = NULL;
	}
	//end of add by Qiu Song on 20090819 for 指定タイプのスレッドダンプ
}

bool Config::hasMonitoringClassesChanged(const Config &rhs)
{
	if(this->monitoringClasses->size() != rhs.monitoringClasses->size())
	{
		return true;
	}

	for(int i = 0; i < this->monitoringClasses->size();i++)
	{
		MonitoringClass* first = (MonitoringClass* )this->monitoringClasses->at(i);
        MonitoringClass* second = (MonitoringClass* )rhs.monitoringClasses->at(i);

		if(*first == *second)
		{}
		else
		{
			return true;
		}
	}
	return false;
}

int Config::operator==(const Config &rhs) const
{
    if(this->monitorAllException != rhs.monitorAllException)
		return false;

	if(this->monitoringTargets->size() != rhs.monitoringTargets->size())
		return false;

	for(int i = 0; i < this->monitoringTargets->size();i++ )
	{
		MonitoringTarget* first = (MonitoringTarget* )this->monitoringTargets->at(i);
        MonitoringTarget* second = (MonitoringTarget* )rhs.monitoringTargets->at(i);
		if(*first == *second)
		{ 
			//compare outputsetting
			map<string,OutputSetting*>::const_iterator iter1 = outputSettingMap->find(first->getThrowableClass() );
            map<string,OutputSetting*>::const_iterator iter2 = rhs.outputSettingMap->find(second->getThrowableClass() );
			if((iter1 == outputSettingMap->end() && iter2 != outputSettingMap->end()) ||
				(iter1 != outputSettingMap->end() && iter2 == rhs.outputSettingMap->end()))
			{
				return false;
            } 

			if(iter1 != outputSettingMap->end() && iter2 != rhs.outputSettingMap->end())
			{
				//compare outputsetting
				if(*(iter1->second) == *(iter2->second))
				{
	                continue;
				}else
				{
					return false;
				}
			}
		}
		else
		{
			return false;
		}
    } 

	if(this->dumpObjectTemplates->size() != rhs.dumpObjectTemplates->size())
	{
		return false;
	}

	for(int j = 0; j < this->dumpObjectTemplates->size();j++ )
	{
		DumpObject* first = (DumpObject* )this->dumpObjectTemplates->at(j);
        DumpObject* second = (DumpObject* )rhs.dumpObjectTemplates->at(j);
		if(*first == *second)
		{ continue;
		}
		else
		{
			return false;
		}
    } 

	if ((mailSettingMap != NULL && rhs.mailSettingMap == NULL) ||
		(mailSettingMap == NULL && rhs.mailSettingMap != NULL))
	{
		return false;
	}
	if (mailSettingMap != NULL && rhs.mailSettingMap != NULL)
	{
		if (mailSettingMap->size() != rhs.mailSettingMap->size()) 
		{
			return false;
		}

		map<string, string>::iterator iter = mailSettingMap->begin();
		while (iter != mailSettingMap->end()) 
		{
			if ((*rhs.mailSettingMap)[iter->first] != iter->second)
			{
				return false;
			}
			++iter;
		}
	}

	if (this->monitorSignal != rhs.monitorSignal)
	{
		return false;
	}

	if (monitorSignal == true)
	{
		if (this->signalDumpKind != rhs.signalDumpKind)
		{
			return false;
		}
	}
	
	if (this->signalOutputSetting != NULL)
	{
		if (rhs.signalOutputSetting == NULL)
			return false;
		else
		{
			if (*(this->signalOutputSetting) == *(rhs.signalOutputSetting))
			{
			}
			else
			{
				return false;
			}
		}
	}
	else if (rhs.signalOutputSetting != NULL)
	{
		return false;
	}

    //we don't compare log path and level
	return this->sleepTime == rhs.sleepTime &&
		   strcmp(this->dumpFilePath,rhs.dumpFilePath ) == 0 &&
		   strcmp(this->dumpFilePrefix,rhs.dumpFilePrefix) == 0 && 
		   this->minDiskSpace == rhs.minDiskSpace &&
		   this->maxDumpData == rhs.maxDumpData &&
		   this->checkDuplication == rhs.checkDuplication &&
		   this->dumpDuplicationWhenThreadDiff == rhs.dumpDuplicationWhenThreadDiff &&
		   this->timeLimit == rhs.timeLimit;
}

void Config::init(char *configFileName)
{
	//remeber the config file name
	configFileFullPath = HelperFunc::strdup(configFileName);

	//initial xerces-c
	DOMBuilder* parser = initXerces_c();

	try{
		//parse xml file
		DOMDocument *doc = parseXML(parser);

		//get others element info
		readOthersElement(doc);

		//get monitor class info
		readMonitorTargets(doc);

		//get dump instance info
		readDumpInstance(doc);

		//get object templates info
		readObjectTemplates(doc);

		//Monitorモードの設定
		chkIfToMonitorAllException();
	}catch(ReadCfgFileException& e )
	{
        parser->release();
		XMLPlatformUtils::Terminate();
	    
		throw ReadCfgFileException(e.getErrorMsg());
	}

	parser->release();
	XMLPlatformUtils::Terminate();
}

//initial xerces-c 
DOMBuilder*  Config::initXerces_c()
{
    try {
        XMLPlatformUtils::Initialize();
		setlocale(LC_CTYPE,"");//transcode will fail without this line for kanji
    }catch (const XMLException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
		string buf = "Error during initialization: ";
		buf += message;
		XMLString::release(&message);

		throw  ReadCfgFileException(buf.c_str());
    }

	//create parser
    XMLCh tempStr[MAX_BUF_LEN];
    XMLString::transcode("LS", tempStr, MAX_BUF_LEN - 1);
    DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(tempStr);
    DOMBuilder* parser = ((DOMImplementationLS*)impl)->createDOMBuilder(
		DOMImplementationLS::MODE_SYNCHRONOUS, 0);
	
	if(parser == NULL)
	{
        throw  ReadCfgFileException("Can't create parser");
	}
    // optionally you can set some features on this builder
    if (parser->canSetFeature(XMLUni::fgDOMValidation, true))
    {
		parser->setFeature(XMLUni::fgDOMValidation, true);
	}
    if (parser->canSetFeature(XMLUni::fgDOMNamespaces, true))
	{
		parser->setFeature(XMLUni::fgDOMNamespaces, true);
    }
    if (parser->canSetFeature(XMLUni::fgDOMDatatypeNormalization, true))
    {    parser->setFeature(XMLUni::fgDOMDatatypeNormalization, true);
	}
	if (parser->canSetFeature(XMLUni::fgXercesSchema, true))
	{
		parser->setFeature(XMLUni::fgXercesSchema, true);
	}

	return parser;
}

//parse xml file
DOMDocument* Config::parseXML(DOMBuilder* parser)
{
    DOMDocument *doc = NULL;
    // implement  DOMErrorHandler and set it to the builder
    DOMConfigErrorHandler errHandler;
    parser->setErrorHandler(&errHandler);

	//we try to read the file ten times when there is an error
	int readNumber = 0;
	while(readNumber < 10)
    {
		readNumber++;
		doc = parser->parseURI(configFileFullPath);

		//解析中にエラーが発生する場合
		if (errHandler.getSawErrors())
		{
			if(readNumber >= 10)
			{
				throw  ReadCfgFileException(errorMsg.c_str());
			}
		}else
		{
			break;
        }
    }

	return doc;
}


//get monitor class info
void Config::readMonitorTargets(DOMDocument *doc)
{
	if (monitoringTargets == NULL)
	{
		monitoringTargets = new vector<MonitoringTarget*>;
	}
	if (monitoringClasses == NULL)
	{
		monitoringClasses = new vector<MonitoringClass*>;
	}

	DOMElement *root = doc->getDocumentElement();
	
	//get task nodes
	XMLCh *xmlTemp;
	DOMNodeList* nodeTaskList = root->getElementsByTagName(
		xmlTemp = XMLString::transcode(TAG_TASK));
	XMLString::release(&xmlTemp);
	XMLSize_t size = nodeTaskList->getLength();
	XMLSize_t index=0;

	int nMonitorMode = MONITOR_MODE_NULL;//add by Qiu Song on 20091007 for 自動監視機能と指定タイプ監視機能の排他処理
    for (index = 0; index < size; index++)
    {

        DOMElement *nodeTask = (DOMElement*)nodeTaskList->item(index);
		//add by Qiu Song on 20090824 for 監視中止タスク
		DOMNamedNodeMap *attributesTask = nodeTask->getAttributes();
		
		char *taskSuffix = HelperFunc::getAttrValueUtf8(attributesTask, ATTR_SUFFIX);

		char *taskValid = HelperFunc::getAttrValueUtf8(attributesTask, ATTR_VALID);
		if (taskValid != NULL && strcmp(taskValid, "false") == 0)
		{
			continue;
		}
		//end of add by Qiu Song on 20090824 for 監視中止タスク

		//OutputSettingインスタンスを作成
		DOMNodeList* nodeOutputList = nodeTask->getElementsByTagName(
			xmlTemp = XMLString::transcode(TAG_OUTPUT_SETTING));
		XMLString::release(&xmlTemp);
		OutputSetting *taskSetting = NULL;
		if (nodeOutputList->getLength() == 1)
		{
			taskSetting = new OutputSetting((DOMElement *)nodeOutputList->item(0));

			if(outputSettingList == NULL)
			{
				outputSettingList = new vector<OutputSetting*>;
			}
			if(taskSetting != NULL)
			{
				outputSettingList->push_back(taskSetting);
			}

 		}
        if(outputSettingMap == NULL)
		{
			outputSettingMap = new map<string,OutputSetting*>;
		}

		//get MonitoringSigal node
		DOMNodeList* nodeSignalList = nodeTask->getElementsByTagName(
			xmlTemp = XMLString::transcode(TAG_MONITORSIGNAL));
		XMLString::release(&xmlTemp);
		XMLSize_t sizeSignal  = nodeSignalList->getLength();
        if(sizeSignal > 0)
		{
			if(sizeSignal > 1)
			{
				throw  ReadCfgFileException("Only one MonitoringSignal element should be defined.");
			}else
			{
				if(monitorSignal)
				{
					throw  ReadCfgFileException("Only one MonitoringSignal element should be defined.");
				}else
				{
					monitorSignal = true;
					signalOutputSetting = taskSetting;
					
					//get DumpKind Attribute
					DOMNamedNodeMap *attributesSignal = nodeSignalList->item(0)->getAttributes();

					char *value = HelperFunc::getAttrValueUtf8(attributesSignal,ATTR_DUMP_KIND);
					if (value != NULL)
					{
						signalDumpKind = strtol(value, NULL, 10);
						delete[] value;
						value = NULL;
					}
					if ((signalDumpKind != 1) && (signalDumpKind != 2))
					{
						throw  ReadCfgFileException("MonitoringSignal's DumpKind should be 1 or 2.");
					}

					//add by Qiu Song on 20090819 for 指定状態のスレッドダンプ
					char *threadStatusConfig = HelperFunc::getAttrValueUtf8(attributesSignal, ATTR_THREAD_STATUS);
					if (threadStatusConfig != NULL)
					{
						threadStatus = new char[strlen(threadStatusConfig) + 1];
						strcpy(threadStatus, threadStatusConfig);
						delete[] threadStatusConfig;
						threadStatusConfig = NULL;
					}
					else
					{
						threadStatus = new char[4];
						strcpy(threadStatus, "all");
						threadStatus[3] = '\0';
					}
					//end of add by Qiu Song on 20090819 for 指定状態のスレッドダンプ

					//add by Qiu Song on 20091022 for ダンプスレッドの優先度の指定
					char* threadPriorityConfig = HelperFunc::getAttrValueUtf8(attributesSignal, ATTR_THREAD_PRIORITY);
					if (threadStatusConfig != NULL)
					{
						threadPriority = atoi(threadPriorityConfig);
						delete[] threadPriorityConfig;
						threadPriorityConfig = NULL;
					}
					else
					{
						threadPriority = 0;
					}
					//end of add by Qiu Song on 20091022 for ダンプスレッドの優先度の指定
				}
			}
 		}

		//add by Qiu Song on 20091105 for タスクprefix出力
		if(taskSuffix != NULL && taskSetting != NULL)
		{
			taskSetting->setTaskPrefix(taskSuffix);
		}
		//end of add by Qiu Song on 20091105 for タスクprefix出力

		//MonitoringTargetsベクタを作成
		DOMNodeList* nodeTargetsList = nodeTask->getElementsByTagName(
			xmlTemp = XMLString::transcode(TAG_MONITORING_TARGET));
		XMLString::release(&xmlTemp);
		XMLSize_t sizeTargets  = nodeTargetsList->getLength();

		XMLSize_t indexTarget=0;
		int ret;
		for (indexTarget = 0; indexTarget < sizeTargets; indexTarget++)
		{
			DOMNode *node = nodeTargetsList->item(indexTarget);
			MonitoringTarget *target = new MonitoringTarget();
			
			//設定ファイルから監視クラス情報を取得する
			ret = target->init((DOMElement *)node);
			if(ret != 0)
			{
				delete target;
				throw   ReadCfgFileException("Failed to get monitor class info.");
			}

			//add by Qiu Song on 20090821 for 監視中止タスク
			//監視中止フラグによって、有効無効の設定を行う
			if(target->getThrowableClassValid() == false)
			{
				delete target;
				continue;
			}
			//end of add by Qiu Song on 20090821 for 監視中止タスク

			string monitorClassName = target->getThrowableClass();

            if(outputSettingMap->find(monitorClassName) != outputSettingMap->end() &&
				strcmp(monitorClassName.c_str(), "java.lang.Throwable") != 0 )
			{
				string buf = "Duplicated monitor class name: ";
				buf += target->getThrowableClass();
				delete target;
				throw  ReadCfgFileException(buf.c_str());
			}

			//add by Qiu Song on 20091007 for 指定タイプと自動監視の排他処理
			int nCompareRet = strcmp(monitorClassName.c_str(), "java.lang.Throwable");
			switch(nMonitorMode)
			{
			case MONITOR_MODE_NULL:
				nMonitorMode = nCompareRet == 0? MONITOR_MODE_AUTO : MONITOR_MODE_EXCEPTION;
				break;
			case MONITOR_MODE_AUTO:
				if( nCompareRet != 0 )
				{
					throw  ReadCfgFileException("「例外を自動監視するタスク」及び「指定タイプの例外自動監視タスク」は共存されません");
				}
				break;
			case MONITOR_MODE_EXCEPTION:
				if( nCompareRet == 0 )
				{
					throw  ReadCfgFileException("「例外を自動監視するタスク」及び「指定タイプの例外自動監視タスク」は共存されません");
				}
				break;
			}
 			//end of add by Qiu Song 20091007 for 指定タイプと自動監視の排他処理
			
			//insert into the proper place
			vector<MonitoringTarget*>::iterator p;
			for (p = monitoringTargets->begin(); p != monitoringTargets->end(); ++p) 
			{
				if (*target < **p) 
					break;
			}
			monitoringTargets->insert(p, target);
			outputSettingMap->insert(
				pair<string,OutputSetting*>(monitorClassName, taskSetting));
		}

		//MonitoringMethodベクタを作成
		DOMNodeList* nodeMethodsList = nodeTask->getElementsByTagName(
			xmlTemp = XMLString::transcode(TAG_MONITORING_METHOD));
		XMLString::release(&xmlTemp);
		XMLSize_t sizeMethods  = nodeMethodsList->getLength();

		XMLSize_t indexMethod=0;
		for (indexMethod = 0; indexMethod < sizeMethods; indexMethod++)
		{
			DOMNode *node = nodeMethodsList->item(indexMethod);
			MonitoringMethod *target = new MonitoringMethod();
			string buf = "";
			ret = target->init((DOMElement *)node, buf);
			if (ret != 0)
			{
				delete target;
				throw  ReadCfgFileException(buf.c_str());
			}

			//insert into the proper place
			vector<MonitoringClass*>::iterator p;
			bool isSame = false;
			for (p = monitoringClasses->begin(); p != monitoringClasses->end(); ++p) 
			{
				bool ret = target->isConflict(**p, isSame);
				if (ret == true)
				{
					string  buf = "Duplicated definition of MonitoringMethod exist. class: ";
					buf.append(target->getClassName());
                    buf.append(" classLoadString: ");
					buf.append(target->getClassLoadString()); 
					delete target;
					throw  ReadCfgFileException(buf.c_str());
				}

				if (*target < **p) 
					break;
			}
			target->setOutputSetting(taskSetting);
			monitoringClasses->insert(p, target);
		}
		if (sizeTargets + sizeMethods + sizeSignal == 0)
		{
			throw  ReadCfgFileException("Task element should have one of the following elements:MonitoringMethod,MonitoringTarget,MonitoringSignal.");
		}
	}//of for (index = 0; index < size; index++)
}

//get dumpInstance info
void Config::readDumpInstance(DOMDocument *doc)
{
	DOMElement *root = doc->getDocumentElement();
	
	//get dumpInstance node list
	XMLCh *xmlTemp;
	DOMNodeList*  nodeObjectList = root->getElementsByTagName(
		xmlTemp = XMLString::transcode(TAG_DUMP_INSTANCE));
	XMLString::release(&xmlTemp);
	
	if(nodeObjectList == NULL)
		return;
		
	XMLSize_t size = nodeObjectList->getLength();
	if (size == 0)
		return;

	assert(size == 1);

	DOMElement *nodeDumpInstance = (DOMElement*)nodeObjectList->item(0);
	
	//get instance node list
	nodeObjectList = nodeDumpInstance->getElementsByTagName(
			xmlTemp = XMLString::transcode(TAG_INSTANCE));
	XMLString::release(&xmlTemp);

	assert(nodeObjectList != NULL);
	
	XMLSize_t sizeInstance = nodeObjectList->getLength();
	assert(sizeInstance > 0);
	
	string errMsg = "";
	//read instance node 
    XMLSize_t index=0;
	for (index = 0; index < sizeInstance; index++)
	{
		MonitoringInstance *target = new MonitoringInstance();
		errMsg = "";
        int ret = target->init((DOMElement *)(nodeObjectList->item(index)), errMsg);
		if (ret < 0)
		{   //there is an error
			delete target;
			throw  ReadCfgFileException(errMsg.c_str());
		}

		if (false == target->isValid())
		{
			delete target;
			continue;
		}

		//insert into the proper place
		vector<MonitoringClass*>::iterator p;
		bool isSame = false;
		int  nType = 0;
		for (p = monitoringClasses->begin(); p != monitoringClasses->end(); ++p) 
		{
			nType = (*p)->nMonitoringType;
			isSame = false;
			bool ret = target->isConflict(**p, isSame);
			if (ret == true)
			{
				string  buf = "";
				if (nType != 2)
				{
					buf = "Duplicated definition of DumpInstance exist. class: ";
				}
				else
				{
					buf = "The definition of DumpInstance is conflict with MonitoringMethod. class: ";
				}
				buf.append(target->getClassName());
				buf.append(" classLoadString: ");
				buf.append(target->getClassLoadString()); 
				delete target;
				throw  ReadCfgFileException(buf.c_str());
			}

			if ((nType == 2) && isSame)
			{
				((MonitoringMethod*)(*p))->addMonitoringInstance(target);
				delete target;
				target = NULL;
				break;
			}
			if (*target < **p) 
				break;
		}
		if (target != NULL)
			monitoringClasses->insert(p, target);
	}
}

//get object templates info
void Config::readObjectTemplates(DOMDocument *doc)
{
	if(dumpObjectTemplates == NULL)
	{
       dumpObjectTemplates = new vector<DumpObject*>;
	   dumpObjectMap = new map<string, DumpObject*>;
	}

	DOMElement *root = doc->getDocumentElement();
	
	//get Object node list
	XMLCh *xmlTemp;
	DOMNodeList*  nodeObjectList = root->getElementsByTagName(
		xmlTemp = XMLString::transcode(TAG_OBJECT));
	XMLString::release(&xmlTemp);
    XMLSize_t index=0;

	if(nodeObjectList != NULL)
	{
	    XMLSize_t size = nodeObjectList->getLength();
	    for (index = 0; index < size; index++)
		{

		    DOMNode *nodeObject = nodeObjectList->item(index);
		    DumpObject *targetObject = new DumpObject();
            int ret = targetObject->init((DOMElement *)nodeObject);
			if(ret < 0)
			{   //there is an error
				delete targetObject;
				throw  ReadCfgFileException("Failed to get object template info.");
			}
			if(!targetObject->isValid() )
			{//Valid flag is not true
                 delete targetObject;
				 continue;
			}

            string objectClassName = targetObject->getObjectClassName();
            if(dumpObjectMap->find(objectClassName) != dumpObjectMap->end())
			{
				string buf = "Duplicated Object class name: ";
				buf += targetObject->getObjectClassName();
				delete targetObject;
				throw  ReadCfgFileException(buf.c_str());
			}
		    //insert into the proper place
		    vector<DumpObject*>::iterator pObject;
		    for (pObject = dumpObjectTemplates->begin(); 
			     pObject != dumpObjectTemplates->end(); ++pObject) 
			{
			    if (*targetObject < **pObject) 
			     	break;
			}
			dumpObjectTemplates->insert(pObject, targetObject);
			dumpObjectMap->insert(
				pair<string,DumpObject*>(objectClassName, targetObject));
		}
	}
}

//get others element info
void Config::readOthersElement(DOMDocument *doc)
{
	DOMElement *root = doc->getDocumentElement();
	
	//get Object node list
	XMLCh *xmlTemp;

	//get others attribute
	DOMNodeList* nodeOthersList = root->getElementsByTagName(
		xmlTemp = XMLString::transcode(TAG_OTHERS));
	XMLString::release(&xmlTemp);
	if (nodeOthersList->getLength() == 1)
	{
		DOMElement *node = (DOMElement *)nodeOthersList->item(0);

		//get log attribute
	    DOMNodeList *logNodes = node->getElementsByTagName(
	  	    xmlTemp = XMLString::transcode(TAG_LOG));
     	XMLString::release(&xmlTemp);
		if (logNodes->getLength() == 1)
		{
			DOMNamedNodeMap *attributesLog = logNodes->item(0)->getAttributes();
			char *value;

			//get log level
			value = HelperFunc::getAttrValueUtf8(attributesLog,ATTR_LOGLEVEL); 
			if (value != NULL)
			{
			    logLevel = Logger::convertLevel(value);
				delete[] value;
				value = NULL;
			}
			Global::logger->setLevel(logLevel);

			//get log path
			value = HelperFunc::getAttrValueUtf8(attributesLog,ATTR_LOGPATH);
			if (value != NULL)
			{
				logPathUtf8 = new char[strlen(value) + 1];
				strcpy(logPathUtf8, value);
				delete[] value;
				value = NULL;
			}

			logPath = HelperFunc::utf8_to_native(logPathUtf8);
			if (logPath != NULL)
			{
				bool ret = HelperFunc::isValidDir(logPath);
				if(!ret)
				{
					string buf = "Log file path is invalid:";
					buf += logPathUtf8; //write to log in utf-8 format
					throw ReadCfgFileException(buf.c_str());
				}
			}
			Global::logger->setPath(logPath);
		}
		//get dump file attribute
	    DOMNodeList *dumpFileNodes = node->getElementsByTagName(
	  	    xmlTemp = XMLString::transcode(TAG_DUMPFILE));
     	XMLString::release(&xmlTemp);
		if (dumpFileNodes->getLength() == 1)
		{
			DOMNamedNodeMap *attributesDumpFile = dumpFileNodes->item(0)->getAttributes();
			//DumpFile path in utf8
			char* value = HelperFunc::getAttrValueUtf8(attributesDumpFile,ATTR_PATH);
			if (value != NULL)
			{
				dumpFilePathUtf8 = new char[strlen(value) + 1];
				strcpy(dumpFilePathUtf8, value);
				delete[] value;
				value = NULL;
			}

		    //DumpFile path
			dumpFilePath = HelperFunc::utf8_to_native(dumpFilePathUtf8);
			if (dumpFilePath == NULL)
			{
				dumpFilePath = dumpFilePathUtf8;
			}
			if (dumpFilePath != NULL)
			{
				bool ret = HelperFunc::isValidDir(dumpFilePath);
				if(!ret)
				{
					string buf = "Dump file path is invalid:";
					buf += dumpFilePathUtf8;  //write to log in utf-8 format
					throw  ReadCfgFileException(buf.c_str());
				}
			}

			//get dump file prefix
			value = HelperFunc::getAttrValueUtf8(attributesDumpFile,ATTR_PREFIX);
			if (value != NULL)
			{
				//we use the value to dumpFilePrefix,so you shouldn't delete value[]
				dumpFilePrefix = value;
				if(!isValidPrefix(dumpFilePrefix))
				{
					string buf = "Dump file prefix " ;
					buf += dumpFilePrefix;
					buf += "is invalid.The char you can use is: a-z A-Z 1-9 - _";
				    throw ReadCfgFileException(buf.c_str());
				}

			}

			//get MinDiskSpace
			value = HelperFunc::getAttrValueUtf8(attributesDumpFile,ATTR_MIN_DISK_SPACE);
			if (value != NULL)
			{
				int nTmp = strtol(value, NULL, 10);
				if (nTmp > 0)
				{
					minDiskSpace = (unsigned __int64)nTmp*1024*1024;
				}

				delete[] value;
				value = NULL;
			}			
		}

		//get sleep time
	    DOMNodeList *sleepNodes = node->getElementsByTagName(
	  	    xmlTemp = XMLString::transcode(TAG_SLEEP_SETTING));
     	XMLString::release(&xmlTemp);
		if (sleepNodes->getLength() == 1)
		{
			//sleep time
			DOMNamedNodeMap *attributesSleep = sleepNodes->item(0)->getAttributes();
			char *value = HelperFunc::getAttrValueUtf8(attributesSleep,ATTR_SLEEP);
			if (value != NULL)
			{
			   sleepTime = strtol(value, NULL, 10);
			   delete[] value;
			   value = NULL;
			}
		}
		//get max dump data 
		DOMNodeList *maxDumpDataNodes = node->getElementsByTagName(
	  		xmlTemp = XMLString::transcode(TAG_MAXDUMPDATA_SETTING));
		XMLString::release(&xmlTemp);
		if (maxDumpDataNodes->getLength() == 1)
		{
			//dump data limit
			DOMNamedNodeMap *attributesLimit = maxDumpDataNodes->item(0)->getAttributes();
			char *value = HelperFunc::getAttrValueUtf8(attributesLimit,ATTR_LIMIT);
			if (value != NULL)
			{
				long lLimit = 0;
				if(value != NULL)
				{
				   lLimit = strtol(value, NULL, 10);
				   if(lLimit < 0)
				   {
					   lLimit = 0;
				   }
				   //max 1G
				   if(lLimit > 1024*1024)
				   {
					   lLimit = 1024*1024;
				   }
				   maxDumpData = lLimit * 1024;

				}
				delete[] value;
				value = NULL;
			}
		}

		//get mail setting
		DOMNodeList *mailNodes = node->getElementsByTagName(
	  		xmlTemp = XMLString::transcode(TAG_MAIL));
		XMLString::release(&xmlTemp);
		if (mailNodes->getLength() == 1)
		{
			DOMNamedNodeMap *attributesMail = mailNodes->item(0)->getAttributes();
			mailSettingMap = new map<string, string>(); 
			
			for (int i = 0; i < attributesMail->getLength(); i++) 
			{
				DOMNode *attributeNode = attributesMail->item(i);
				char *name = XMLString::transcode(attributeNode->getNodeName());
				char *value = HelperFunc::getAttrValueUtf8(attributesMail, name);
				//AUTO_REL_OBJECT_ARR(name);
				AUTO_REL_OBJECT_ARR(value);
				
				(*mailSettingMap)[name] = string(value);

				if ((value != NULL) && (strcmp(value,"") == 0))
				{
					if ((strcmp(name, "From") == 0) 
						|| (strcmp(name, "To") == 0)
						|| (strcmp(name, "SmtpServer") == 0)
						|| (strcmp(name, "Subject") == 0)
						)
					{
						string buf = "Mail's ";
						buf += name;
						buf += " attribute should not be null.";
						throw ReadCfgFileException(buf.c_str());
					}
				}
				if (strcmp(name, "BodyTemplateFolderPath") == 0)
				{
					if ((value != NULL) && (strcmp(value,"") != 0))
					{
						char *value1 = HelperFunc::utf8_to_native(value);
						if (value1 != NULL)
						{
							bool ret = HelperFunc::isValidDir(value1);
							delete [] value1;
							value1 = NULL;
							if(!ret)
							{
								string buf = "Mail template folder path is invalid:";
								buf += value; //write to log in utf-8 format
								LOG4CXX_WARN(Global::logger, buf.c_str());
								(*mailSettingMap)[name] = string("");
							}
						}
					}
				}//of if (strcmp(name, "BodyTemplateFolderPath") == 0)
				XMLString::release(&name);
			}//of for (int i = 0; i < attributesMail->getLength(); i++)
		}//of if (mailNodes->getLength() == 1)

		//get check duplication setting
		DOMNodeList *checkDuplicationNodes = node->getElementsByTagName(
			xmlTemp = XMLString::transcode(TAG_CHECK_DUPLICATION));
		XMLString::release(&xmlTemp);
		if (checkDuplicationNodes->getLength() == 1)
		{
			this->checkDuplication = true;
			
			DOMNamedNodeMap *attributesCheckDuplication = checkDuplicationNodes->item(0)->getAttributes();
			char *value = HelperFunc::getAttrValueUtf8(attributesCheckDuplication, ATTR_DUMP_DUPLICATION_WHEN_THREAD_DIFF);
			this->dumpDuplicationWhenThreadDiff = (strcmp(value, "true") == 0);
			delete value;

			value = HelperFunc::getAttrValueUtf8(attributesCheckDuplication, ATTR_TIME_LIMIT);
			if(value != NULL)
			{
				this->timeLimit = strtol(value, NULL, 10);
				if(this->timeLimit < 0)
				{
					this->timeLimit = 0;
				}
				delete[] value;
				value = NULL;
			}
		}
	}
}

//you should call this function after config file is readed
void Config::logConfig()
{

	if (monitoringTargets != NULL)
	{
		vector<MonitoringTarget*>::iterator it;
		for(it = monitoringTargets->begin(); it != monitoringTargets->end(); it++ )
		{
           ((MonitoringTarget*)(*it))->logConfig();
           string monitorClass = (*it)->getThrowableClass();
           map<string,OutputSetting*>::const_iterator iter = outputSettingMap->find(monitorClass);
	       OutputSetting* setting = iter->second;
		   setting->logConfig();
        }  
	}

	if (monitoringClasses != NULL)
	{
		vector<MonitoringClass*>::iterator it;
		int monitoringType = 0;
		for(it = monitoringClasses->begin(); it != monitoringClasses->end(); it++ )
		{
			monitoringType = ((MonitoringClass*)(*it))->getMonitoringType();
			if ((monitoringType == 2) || (monitoringType == 3))
				((MonitoringMethod*)(*it))->logConfigForMethod();
        }  
	}

	if (monitoringClasses != NULL)
	{
		vector<MonitoringClass*>::iterator it;
		int monitoringType = 0;
		for(it = monitoringClasses->begin(); it != monitoringClasses->end(); it++ )
		{
			monitoringType = ((MonitoringClass*)(*it))->getMonitoringType();
			if ((monitoringType == 1) || (monitoringType == 3))
			{
				(*it)->logConfig();
			}
        }  
	}

	if (dumpObjectTemplates != NULL)
	{
		vector<DumpObject*>::iterator it;
		for(it = dumpObjectTemplates->begin(); it != dumpObjectTemplates->end(); it++ )
		{
           ((DumpObject*)(*it))->logConfig();
        }  
	}
   
	if(monitorSignal)
	{
        LOG4CXX_INFO(Global::logger, "Monitor signal");
		if (signalDumpKind == 1)
		{
			LOG4CXX_INFO(Global::logger, "  Dump stackTrace");
		}
		else
		{
			LOG4CXX_INFO(Global::logger, "  Dump class instance");
		}
	}else
	{
        LOG4CXX_INFO(Global::logger, "Not to monitor signal");
	}

	string buf;
    char digitBuf[MAX_BUF_LEN];

	buf = "Dump file path: ";
    buf += dumpFilePathUtf8;
	LOG4CXX_INFO(Global::logger, buf.c_str());

	buf = "Dump file prefix: ";
	buf += dumpFilePrefix;
	LOG4CXX_INFO(Global::logger, buf.c_str());

	buf = "Check config file interval time: ";
	sprintf(digitBuf,"%ds",sleepTime);
    buf += digitBuf;
	LOG4CXX_INFO(Global::logger, buf.c_str());

	buf = "Max dump data size limit: ";
	if (maxDumpData > 0)
	{
		sprintf(digitBuf,"%dKB",maxDumpData /1024);
		buf += digitBuf;
	}
	else
	{
		buf += "unlimited";
	}
    LOG4CXX_INFO(Global::logger, buf.c_str());

	buf = "Minimum disk space needed to dump: ";
	sprintf(digitBuf,"%dMB",(int)(minDiskSpace /1024/1024));
	buf += digitBuf;

	LOG4CXX_INFO(Global::logger, buf.c_str());

	buf = "Log file path: ";
	buf += logPathUtf8;
	LOG4CXX_INFO(Global::logger, buf.c_str());

	buf = "Log level: ";
	buf += Global::logger->getLogLevelStr(logLevel);
	LOG4CXX_INFO(Global::logger, buf.c_str());

	if (mailSettingMap != NULL) {
		map<string, string>::iterator iter = mailSettingMap->begin();
		while (iter != mailSettingMap->end()) {
			buf = "Mail[";
			buf += iter->first;
			buf += "]: ";
			buf += iter->second;
			LOG4CXX_INFO(Global::logger, buf.c_str());
			
			++iter;
		}			
	}

	buf = "Check duplication of dump: ";
	buf += this->checkDuplication ? "true" : "false";
	LOG4CXX_INFO(Global::logger, buf.c_str());
	
	buf = "Check duplication of dump for different thread: ";
	buf += this->dumpDuplicationWhenThreadDiff ? "true" : "false";
	LOG4CXX_INFO(Global::logger, buf.c_str());

	sprintf(digitBuf, "Not to check duplication after %ds", this->timeLimit);
	LOG4CXX_INFO(Global::logger, digitBuf);
}

DumpObject* Config::getDumpObject(char* objectClassName)
{ 
	if(dumpObjectMap == NULL)
	{
		return NULL;
	}
	string key = objectClassName;
    map<string, DumpObject*>::iterator it = dumpObjectMap->find(key);
	if(it == dumpObjectMap->end())
	{
		return NULL;
	}else
	{
		return it->second;
	}
}

MonitoringTarget* Config::getMonitorTargetByName(const char *throwableClassName)
{
	for (int index = 0; index < monitoringTargets->size(); index++) 
	{
		MonitoringTarget *target = (MonitoringTarget *)monitoringTargets->at(index);

		if(strcmp(target->getThrowableClass(),throwableClassName) == 0)
		{
		    return target;
		}
	}

	return NULL;
}

bool Config::isMonitoringTarget(const char *throwableClassName, const char *classSig, 
						   const char *methodName, const char *methodSig)
{
	bool match = false;
	for (int index = 0; index < monitoringTargets->size(); index++) 
	{
		MonitoringTarget *target = (MonitoringTarget *)monitoringTargets->at(index);

		if(target->shouldDump(throwableClassName,classSig,methodName,methodSig))
		{
			match = true;
		    break;
		}
	}
	
	return match;
}

/**
 * ダンプファイルのPrefixに使える文字列のチェック
 */
bool Config::isValidPrefix(const char* prefix)
{
	if(prefix == NULL)
	{
		return false;
	}

	int len = strlen(prefix);
	for(int i = 0;i < len;i++)
	{
		char ch = *(prefix + i);
		if( ('0' <= ch && ch <='9') ||
			('a' <= ch && ch <='z') ||
			('A' <= ch && ch <='Z') ||
			ch == '-'               ||
			ch == '_'
          )
		{
			;//do nothing
		}else
		{
			return false;
		}
	}

	return true;
}

void Config::setOutputSetting(char* monitorClassName)
{
    string monitorClass = monitorClassName;
    map<string,OutputSetting*>::const_iterator iter = outputSettingMap->find(monitorClass);
	outputSetting = iter->second;
}

bool Config::setStackOverflowErrorOutputSetting()
{
    map<string,OutputSetting*>::const_iterator iter;
	string monitorClass[4] = {"java.lang.StackOverflowError",
		                      "java.lang.VirtualMachineError",
		                      "java.lang.Error",
		                      "java.lang.Throwable"};

	for (int i = 0; i < 4; i++)
	{
		iter = outputSettingMap->find(monitorClass[i]);
		if (iter != outputSettingMap->end())
		{
			outputSetting = iter->second;
			return true;
		}
	}
	return false;
}

bool Config::execeedDumpDataLimit(long lsize)
{
	if(maxDumpData <=0)
	{
		return false;
	}
	return lsize >= maxDumpData;
}


Config::DOMConfigErrorHandler::DOMConfigErrorHandler() :
    fSawErrors(false)
{
}

Config::DOMConfigErrorHandler::~DOMConfigErrorHandler()
{
}


/**
 * Implementation of the DOMErrorHandler interface
 */
bool Config::DOMConfigErrorHandler::handleError(const DOMError& domError)
{
    fSawErrors = true;

	errorMsg = StrX(domError.getLocation()->getURI()).localForm();

	char digitBuf[MAX_BUF_LEN];
	sprintf(digitBuf,", line %d, char %d",domError.getLocation()->getLineNumber(),
		domError.getLocation()->getColumnNumber());
	errorMsg += digitBuf;
	errorMsg += ", Message: ";
    errorMsg += StrX(domError.getMessage()).localForm();

	return true;
}

void Config::DOMConfigErrorHandler::resetErrors()
{
    fSawErrors = false;
}

//監視クラスかどうか判断する
bool Config::isRedefineClassName(const char* className)
{
	bool match = false;
    if (monitoringClasses == NULL)
		return false;

	char* tmp = HelperFunc::replaceChar((char*)className, '/', '.');
	vector<MonitoringClass*>::iterator it;
	for (it=monitoringClasses->begin();it!=monitoringClasses->end();it++)
	{
		if (strcmp(tmp, (*it)->getClassName()) == 0)
		{
			match = true;
		    break;
		}
	}
	delete [] tmp;
	return match;    
}

vector<MonitoringClass*>* Config::getMonitorClassInfo(const char* className, const char* classUrl)
{
    vector<MonitoringClass*>* pRet = new vector<MonitoringClass*>;

    if (monitoringClasses == NULL)
		return pRet;

	char* tmp = HelperFunc::replaceChar((char*)className, '/', '.');
	vector<MonitoringClass*>::iterator it;
	for (it=monitoringClasses->begin();it!=monitoringClasses->end();it++)
	{
		if (strcmp(tmp, (*it)->getClassName()) != 0)
		{
			continue;
		}
		if (strstr(classUrl, (*it)->getClassLoadString()) != NULL)
		{
			pRet->push_back(*it);
		}
	}
	delete[] tmp;
	return pRet;
}

int  Config::getMaxInstanceCount(string& className, string& classUrl)
{
	vector<MonitoringClass*>::iterator it;
	for (it=monitoringClasses->begin();it!=monitoringClasses->end();it++)
	{
		if ((*it)->nMonitoringType == 2)
		{
			continue;	
		}
		int nRet = strcmp(className.c_str(), (*it)->getClassName());
		if (nRet > 0)
		{
			continue;
		} else if (nRet < 0)
		{
			break;
		}
		if (strstr(classUrl.c_str(), (*it)->getClassLoadString()) != NULL)
		{
			return (*it)->maxInstanceCount;
		}
	}	
	return 0;
}

//すべてのException(Throwable)を監視するかどうか
void Config::chkIfToMonitorAllException()
{
	string monitorClassName = "java.lang.Throwable";

    if(outputSettingMap != NULL && (outputSettingMap->find(monitorClassName) != outputSettingMap->end())){
		monitorAllException = true;
	}else{
		monitorAllException = false;
	}
}

//Throwableを監視する時に、発生したExceptionをcatchしたクラス名が、
//監視するパッケージ名に属するかどうかをチェックする
//@param classSig　発生したExceptionをcatchしたクラス
bool Config::isMonitorPackageForAllException(const char *classSig){

	bool match = false;
	for (int index = 0; index < monitoringTargets->size(); index++) 
	{
		MonitoringTarget *target = (MonitoringTarget *)monitoringTargets->at(index);

		char* throwableClass = target->getThrowableClass();
		if(strcmp(throwableClass,"java.lang.Throwable") == 0)
		{
			if(target->isMonitorPackage(classSig))
			{
				match = true;
				break;
			}
		}
	}
	
	return match;
}

//Throwableを監視する時に、発生したExceptionをExcludeすべきかどうかをチェックする
//@param exceptionClassSig　発生したExceptionのクラス名
bool Config::isExcludeClassForAllException(const char *exceptionClassSig)
{
	bool match = false;
	for (int index = 0; index < monitoringTargets->size(); index++) 
	{
		MonitoringTarget *target = (MonitoringTarget *)monitoringTargets->at(index);

		char* throwableClass = target->getThrowableClass();
		if(strcmp(throwableClass,"java.lang.Throwable") == 0)
		{
			if(target->excludeClassExist(exceptionClassSig))
			{
				match = true;
				break;
			}
		}
	}
	
	return match;
}
