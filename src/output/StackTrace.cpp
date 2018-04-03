#pragma warning( disable : 4786 )

#include "StackTrace.h"
#include "../common/Config.h"
#include "../common/Global.h"

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <strstream>
#include <sys/timeb.h>
#include "Method.h"
#include "Instance.h"
#include "../common/HelperFunc.h"
#include "../common/JvmtiAutoRelease.h"
#include "../common/ObjectAutoRelease.h"

#include "../common/JniLocalRefAutoRelease.h"

#include "../common/ExcludeClass.h"
#include "../common/SimpleStackTrace.h"
#include "../common/SystemInfo.h"
#include "../jniutility/AgentCallbackHandler.h"

#include "../antlrparse/CcatLexer.hpp"
#include "../antlrparse/CcatParser.hpp"


using namespace std;
USE_NS(NS_OUTPUT)
USE_NS(NS_COMMON)
USE_NS(NS_JNI_UTILITY)
USE_NS(NS_ANTLRPARSE)

//add by Qiu Song on 20091006 for モニター所有スレッド名の取得
StackTraceParamTag::~StackTraceParamTag()
{
	if(useMonitorThread != NULL)
	{
		delete[] useMonitorThread;
		useMonitorThread = NULL;
	}
}
//end of add by Qiu Song on 20091006 for モニター所有スレッド名の取得

StackTrace::StackTrace(StackTraceParam *param)
{
	this->param = param;
	objectPool = NULL;

	objectDataes = new vector<ObjectData*>();
    methodDataes = new vector<MethodData*>();
	instanceDataes = new vector<InstanceData*>();
	lDumpDataSize = 0;
	dumpCompleted = true;
    threadName = NULL;

	//add by Qiu Song on 20090818 for Excat Version 3.0
	//例外オブジェクトダンプ
	exceptionObjRefId = -1;
	//モニターオブジェクトダンプ
	monitorObjRefId = -1;
    monitorClassName = NULL;
	//メソッド監視
	strDumpPosition = "Start";
	//スレッド状態/CPU時間/待機理由を取得
	getThreadStatusAndCPUTime();
	//ダンプ開始時間
	curDumpTime = NULL;
	//end of add by Qiu Song on 20090818 for Excat Version 3.0
}

StackTrace::~StackTrace()
{
   
	if(objectDataes != NULL)
	{
		vector<ObjectData*>::iterator iter;
		for (iter = objectDataes->begin(); iter != objectDataes->end(); ++iter) 
		{
			ObjectData * objData = (ObjectData *)(*iter);
			if(objData != NULL)
			{
				delete objData;
				objData = NULL;
			}
		}
		objectDataes->clear();
		delete objectDataes;
		objectDataes = NULL;
	}
	
	if(methodDataes != NULL)
	{
		vector<MethodData*>::iterator iter;
		for (iter = methodDataes->begin(); iter != methodDataes->end(); ++iter) 
		{
			MethodData * methodData = (MethodData *)(*iter);
			delete methodData;
		}
		methodDataes->clear();
        delete methodDataes;
		methodDataes = NULL;
	}

	if(instanceDataes != NULL)
	{
		vector<InstanceData*>::iterator iter;
		for (iter = instanceDataes->begin(); iter != instanceDataes->end(); ++iter) 
		{
			InstanceData * instanceData = (InstanceData *)(*iter);
			delete instanceData;
		}
		instanceDataes->clear();
        delete instanceDataes;
		instanceDataes = NULL;
	}

	if(threadName != NULL)
	{
		delete[] threadName;
		threadName = NULL;
	}
	
	//add by Qiu Song on 20090908 for バグ：ファイル名と内容に表示される時間は一致しない
	if(curDumpTime != NULL)
	{
		delete curDumpTime;
		curDumpTime = NULL;
	}
	//end of add by Qiu Song on 20090908 for バグ：ファイル名と内容に表示される時間は一致しない
}

void StackTrace::addDumpDataSize(long dataSize)
{
    lDumpDataSize += dataSize;

	Config *config = Global::getConfig();
	if(config->execeedDumpDataLimit(lDumpDataSize))
	{
        throw new ExceedMemLimitException("dump data size limit is execeeded.");
	}
}

void StackTrace::addObjectData(ObjectData *objectData)
{
    objectDataes->push_back(objectData);
	//addDumpDataSize(objectData->getDumpSize());
}

void StackTrace::addMethodData(MethodData *methodData)
{
    methodDataes->push_back(methodData);
}

void StackTrace::addInstanceData(InstanceData *instanceData)
{
    instanceDataes->push_back(instanceData);
}

/**
 * 発生した例外をダンプするかどうかを判断する。
 * (1)StackOverFlowの場合、このメソッドが呼ばれない
 * (2)除外例外である場合、ダンプしない
 * (3)例外発生箇所が条件を満たさない場合、ダンプしない
 */
bool StackTrace::shouldDumpException(){

	//Throwableを監視する場合、判断ずみ
	if (param->dumpType == DUMP_KIND_THROWABLE )  
		return true;

	OutputSetting *outputSetting = Global::getConfig()->getOutputSetting();
    jvmtiError err;

	jint frameCount = 0;

	//get frame count
	err = param->jvmti->GetFrameCount(param->thread,&frameCount);
	if(err != JVMTI_ERROR_NONE){
		Global::logger->logError("failed to call GetFrameCount in StackTrace::shouldDumpException,error cd =%d.",err);
		return false;
	}

	jvmtiFrameInfo *frameInfos = new jvmtiFrameInfo[frameCount];
    AUTO_REL_OBJECT_ARR(frameInfos);

	//get all stack frames
	jint count;
 	err = param->jvmti->GetStackTrace(param->thread, 0, frameCount, frameInfos, &count);
    if(err != JVMTI_ERROR_NONE)
	{
		Global::logger->logError("failed to call GetStackTrace in StackTrace::shouldDumpException,error cd =%d.",err);
		return false;
	}

	//除外判断
	int pos = 2; //exclude callbacks and dumpstack
	//check if the exception is excluded
	bool ret = shouldExclude(pos, frameCount,frameInfos);
	if(ret)
	{
		return false;
	}else
	{
		//メール途中の例外をExclude
        if(outputSetting->getMail())
		{
            if(exceptionInMail(frameCount,frameInfos))
            {
				Global::logger->logDebug("exception %s happened in mail process.",
					param->exceptionName);
				return false;
			}
			
		}
    }

	//check the place where the exception happens
	bool happend = false;
	for(int frameIndex = frameCount - 1; frameIndex >= pos; frameIndex--)
	{
		if (validateExceptionPlace(frameIndex,frameInfos,pos))
		{
			happend = true;
			break;
		}
	}
	if(!happend)
	{
		return false;
	}
	return true;
}

/**
 * メソッドトリガで、ダンプすべきかどうかを判断
 */
bool StackTrace::shouldDumpMethod(){
	//get stack frames
	OutputSetting *outputSetting = Global::getConfig()->getOutputSetting();
	jint count;
    jvmtiError err;

	//get frame count
	jint frameCount = 0;
	err = param->jvmti->GetFrameCount(param->thread,&frameCount);
    if(err != JVMTI_ERROR_NONE)
	{
		Global::logger->logError("failed to call GetFrameCount in StackTrace::shouldDumpMethod,error cd =%d.",err);
		return false;
	}

	jvmtiFrameInfo *frameInfos = new jvmtiFrameInfo[frameCount];
	AUTO_REL_OBJECT_ARR(frameInfos);
     
	//get all stack frames
 	err = param->jvmti->GetStackTrace(param->thread, 0, frameCount, frameInfos, &count);
    if(err != JVMTI_ERROR_NONE)
	{
		Global::logger->logError("failed to call GetStackTrace in StackTrace::shouldDumpMethod,error cd =%d.",err);
		return false;
	}	

	//check if the dump condition is satisfied 
	bool ret = shouldDump(2, frameInfos);
	if(!ret)
	{
		return false;
	}
	return true;
}

void StackTrace::init()
{
	//get stack frames
	OutputSetting *outputSetting = Global::getConfig()->getOutputSetting();
	jint count;
    jvmtiError err;
	int pos = 0;

	//get frame count
	jint frameCount = 0;
	err = param->jvmti->GetFrameCount(param->thread,&frameCount);
    if(err != JVMTI_ERROR_NONE)
	{
		Global::logger->logError("failed to call GetFrameCount in StackTrace::init,error cd =%d.",err);
		return;
	}
	string logBuf;

	jvmtiFrameInfo *frameInfos = new jvmtiFrameInfo[frameCount];
	AUTO_REL_OBJECT_ARR(frameInfos);
     
	//get all stack frames
 	err = param->jvmti->GetStackTrace(param->thread, 0, frameCount, frameInfos, &count);
    if(err != JVMTI_ERROR_NONE)
	{
		Global::logger->logError("failed to call GetStackTrace in StackTrace::init,error cd =%d.",err);
		return;
	}

	if (param->dumpType == DUMP_KIND_EXCEPTION || param->dumpType == DUMP_KIND_THROWABLE )  //for exception
	{
		if (strcmp(param->exceptionName, "java.lang.StackOverflowError") != 0)
		{
			//frameIfos:callback(Object thread, Object obj)
			//frameIfos+1:callback(Object obj) 
			jint depth = outputSetting->getStackTraceDepth();
			if(param->dumpType == DUMP_KIND_EXCEPTION || param->dumpType == DUMP_KIND_THROWABLE)
			{
				//get the position of the exception
				//int pos = getExpectedMethodPos(frameInfos,frameCount);
				pos = 2; //exclude callbacks and dumpstack

				//add by Qiu Song on 20091007 for パッケージダウンロードの除外判断
				if( param->dumpType == DUMP_KIND_THROWABLE )
				{
					pos = 0;
				}
                //end of add by Qiu Song on 20091007 for パッケージダウンロードの除外判断

				//check if the exception is excluded
				bool ret = shouldExclude(pos, frameCount,frameInfos);
				if(ret)
				{
					return;
				}else
				{
					//メール途中の例外をExclude
					if(outputSetting->getMail())
					{
						if(exceptionInMail(frameCount,frameInfos))
						{
							Global::logger->logDebug("exception %s happened in mail process.",
								param->exceptionName);
							return;
						}
						
					}
				}

				if(param->dumpType == DUMP_KIND_EXCEPTION){
					//check the place where the exception happens
					bool happend = false;
					for(int frameIndex = frameCount - 1; frameIndex >= pos; frameIndex--)
					{
						if (validateExceptionPlace(frameIndex,frameInfos,pos))
						{
							happend = true;
							break;
						}
					}
					if(!happend)
					{
						return;
					}
				}

			}//of if(param->dumpType == DUMP_KIND_EXCEPTION || param->dumpType == DUMP_KIND_THROWABLE)

			//we only dump to the depth that ther user configed
			if(count > pos + depth )
			{
				count = pos + depth;
			}
		}

		if (Global::getConfig()->isCheckDuplication() && isDuplicate(frameInfos, pos, count)) {
			return;
		}

		logBuf = "Dump Begined for exception: ";
		logBuf += param->exceptionName;
		LOG4CXX_INFO(Global::logger,logBuf.c_str());
	}
	else   //dump for method monitor
	{
		//frameIfos:callback(Object thread, Object obj)
		//frameIfos+1:callback(Object obj) 
		jint depth = outputSetting->getStackTraceDepth();

		//get the position of the monitoring method
		pos = 2; //exclude callbacks and dumpstack

		//check if satisfied the dump condition
		bool ret = shouldDump(pos, frameInfos);
		if(!ret)
		{
			return;
		}

		//we only dump to the depth that ther user configed
		if(count > pos + depth )
		{
			count = pos + depth;
		}
		logBuf = "Dump Begined for class: ";
        logBuf += param->className;
        logBuf += "  method: ";
        logBuf += param->methodName;
		LOG4CXX_INFO(Global::logger,logBuf.c_str());
	}
	dumpStack(frameInfos,pos,count,true);
}

//called by dumpAllTread in AgentRoutine.cpp
void StackTrace::dumpThread(bool currentThreadFlag)
{
    string logbuf = "Dump Begined for thread ";
    logbuf += param->exceptionName;
	LOG4CXX_INFO(Global::logger, logbuf.c_str());
	char logBuf[MAX_BUF_LEN];
    jvmtiError err;
    //get frame count
    jint frameCount = 0;
    err = param->jvmti->GetFrameCount(param->thread,&frameCount);
    if(Global::logger->isDebugEnabled())
   {
        sprintf(logBuf,"frame count:%d",frameCount);
        LOG4CXX_DEBUG(Global::logger, logBuf);
    }
    if(err != JVMTI_ERROR_NONE)
    {
       Global::logger->logError("failed to call GetFrameCount in StackTrace::dumpThread,error cd =%d.",err);
        return;
    }

    jvmtiFrameInfo *frameInfos = new jvmtiFrameInfo[frameCount];
    AUTO_REL_OBJECT_ARR(frameInfos);
    //get all stack frames
    jint count =0;
    err = param->jvmti->GetStackTrace(param->thread, 0, frameCount, frameInfos, &count);
    if(err != JVMTI_ERROR_NONE)
    {
          Global::logger->logError("failed to call GetStackTrace in StackTrace::dumpThread,error cd =%d.",err);
          return;
    }

	OutputSetting *outputSetting = Global::getConfig()->getOutputSetting();
	jint depth = outputSetting->getStackTraceDepth();
	if(count > depth)
	{
		count = depth;
	}
    if(count > 0)
    {
		int pos = 0;
		if(currentThreadFlag){
			pos = 2; //skip callback
		}
        dumpStack(frameInfos,pos,count,false);
    }
}

void StackTrace::dumpInstanceForSignal()
{
	string logbuf = "Dump Begined for Instances";
	LOG4CXX_INFO(Global::logger, logbuf.c_str());
	dumpStack(NULL,0,0,false);
}

void StackTrace::dumpStack(jvmtiFrameInfo *frameInfos,int pos ,
						   jint count,bool suspendThread)
{
	LOG4CXX_DEBUG(Global::logger, "Enter StackTrace::dumpStack");

	//check disk space
	if (false == HelperFunc::hasEnoughFreeDiskSpace())
	{
		Global::logger->logError("There is not enough space to make DumpFile.");
		return;
	}

	//ダンプ日時
	//modified by Qiu Song on 20090902 for 時刻精度アップ
	/*time_t ltime;
	time( &ltime );
	memset(strDumptime,0,sizeof(strDumptime));
    //strcpy(strDumptime,asctime(localtime(&ltime)));
    strftime(strDumptime,63, "%Y/%m/%d %H:%M:%S", localtime(&ltime));*/
	
	setDumpTime();
	memset(strDumptime,0,sizeof(strDumptime));
	sprintf(strDumptime,"%04d/%02d/%02d %02d:%02d:%02d:%03d", curDumpTime->m_year, curDumpTime->m_month,
		    curDumpTime->m_day , curDumpTime->m_hour, curDumpTime->m_min, curDumpTime->m_sec, curDumpTime->m_millianSec);

	//end of modified by Qiu Song on 20090902 for 時刻精度アップ

    //get all threads
    jint threadsCount = 0;
    jthread* threads = NULL;
    jvmtiError err;
    jvmtiError* errResults = NULL;
	jthread* objectThreads = NULL;
	int objThreadsCount = 0;
	if(suspendThread)
	{
	    err = param->jvmti->GetAllThreads(&threadsCount,&threads);
		if(err != JVMTI_ERROR_NONE)
		{
			 Global::logger->logError("failed to get all threads,err cd =%d",err);
			 return;
		}		
		errResults = new jvmtiError[threadsCount];
		objectThreads = new jthread[threadsCount];
		
		for(int i=0; i<threadsCount;i++)
		{
			 if(param->jni->IsSameObject(param->thread,threads[i]))
			 {
				 ;//find same thread
			 }else
			 {
				 objectThreads[objThreadsCount++] = threads[i];
			 }
		}
		
		err =  param->jvmti->SuspendThreadList(objThreadsCount,objectThreads,errResults);
		if(err != JVMTI_ERROR_NONE)
		{
			 Global::logger->logError("failed to suspend all threads,err cd =%d",err);
		} 
	}
	AUTO_REL_JVMTI_OBJECT(threads);
	
    //get thread name
	jvmtiThreadInfo threadInfo;
	err = param->jvmti->GetThreadInfo(param->thread,&threadInfo);

	if(err != JVMTI_ERROR_NONE)
	{
		Global::logger->logError("failed to get thread info ,err cd =%d",err);
	}else
	{
		threadName = HelperFunc::strdup(threadInfo.name);
		param->jvmti->Deallocate((unsigned char*)(threadInfo.name));

		if(threadInfo.thread_group != NULL)
		{
			 param->jni->DeleteLocalRef(threadInfo.thread_group);
		}
		if(threadInfo.context_class_loader != NULL)
		{
			 param->jni->DeleteLocalRef(threadInfo.context_class_loader);
		}
	}

	//create object pool
	ObjectPoolParam poolParam;
	poolParam.jni = param->jni;
	poolParam.jvmti = param->jvmti;
	poolParam.thread = param->thread;
	poolParam.parent = this;
	objectPool = new ObjectPool(&poolParam);
	
	try{
		//method情報のダンプ
		if ((param->dumpType == DUMP_KIND_THREAD) && param->dumpInstance)
		{
			//do nothing
		}
		else
		{
			for (jint index = count - 1; index >= pos; index--)
			{
				jvmtiFrameInfo *frameInfo = frameInfos + index;
				MethodParam methodParam;
				MethodData* methodData = new MethodData();
				addMethodData(methodData);
        
				methodParam.jni = param->jni;
				methodParam.jvmti = param->jvmti;
				methodParam.depth = index;
				methodParam.parent = this;
				methodParam.frameInfo = frameInfo;
				methodParam.pool = objectPool;
				methodParam.thread = param->thread;
				methodParam.methodData = methodData;
				//modified by Qiu Song on 20091126 for bug:518
				bool isCurDumpMethod = false;
				if(index == pos)
				{
					isCurDumpMethod = true;
				}
				Method method(&methodParam,isCurDumpMethod);
				//end of modified by Qiu Song on 20091126 for bug:518
				//Method method(&methodParam);
		/*		if(param->jni->ExceptionOccurred() != NULL)
				{
					LOG4CXX_ERROR(Global::logger,"exception happend");
					param->jni->ExceptionDescribe();
					break;
				}*/
			}
		}

		//インスタンス情報のダンプ
		map<string, vector<jweak>*>* instanceMap = AgentCallbackHandler::instanceMap;
		if (param->dumpInstance && (instanceMap != NULL))
		{
			map<string, vector<jweak>*>::const_iterator it = instanceMap->begin();
			for (;it != instanceMap->end();it++)
			{
				string url = strchr(it->first.c_str(), '#') + 1;
				string name = it->first.substr(0, it->first.length() - url.length() - 1);

				
				//ダンプ数制限を取得する
				int nCount = Global::getConfig()->getMaxInstanceCount(name, url);
				//modified by Qiu Song on 2009.08.10 for インスタンスダンプ制限の削除
				if (nCount >= 0)
				//end of modified by Qiu Song on 2009.08.10 for インスタンスダンプ制限の削除
				{
					InstanceParam instanceParam;
					InstanceData* instanceData = new InstanceData(name, url);
					
					instanceParam.jni = param->jni;
					instanceParam.jvmti = param->jvmti;
					instanceParam.thread = param->thread;
					instanceParam.parent = this;
					instanceParam.className = name;
					instanceParam.classUrl = url;
					instanceParam.pool = objectPool;
					instanceParam.objRefVec = it->second;
					instanceParam.maxInstanceCount = nCount;
					instanceParam.instanceData = instanceData;
					
					Global::curAttrDepth = 0;
					Instance instance(&instanceParam);
					if (instanceData->getObjectRef() != NULL)
					{
						addInstanceData(instanceData);
					}
					else
					{
						delete instanceData;
					}
				}
			}
		}

		//add by Qiu Song on 20090819 for Excat Version 3.0
		//例外オブジェクトのrefIDを取得する
		exceptionObjRefId = getObjRefIdFromObjectPool(param->exceptionObj, objectPool);

		//モニターオブジェクトのrefIDを取得する
		monitorObjRefId = getObjRefIdFromObjectPool(param->monitorObj, objectPool);
		if(monitorObjRefId >= 0)
		{
			setMonitorClassName();
		}
		//end of add by Qiu Song on 20090819 for Excat Version 3.0
	}catch(ExceedMemLimitException* e )
	{
        LOG4CXX_INFO(Global::logger, e->what());
		dumpCompleted = false;
		delete e;
    }
    //add by Qiu Song on 20100224 for メモリリックの件
	catch(exception* e)
	{

		LOG4CXX_INFO(Global::logger, "メモリ不足");
		dumpCompleted = false;
	}
	//end of add by Qiu Song on 20100224 for メモリリックの件

	if (objectPool != NULL)
	{
		delete objectPool;
		objectPool = NULL;
	}
	
	if(suspendThread)
	{
		param->jvmti->ResumeThreadList(objThreadsCount,objectThreads,errResults);
		delete[] errResults;
		delete[] objectThreads;
		for(int i = 0; i < threadsCount;i++)
		{
			param->jni->DeleteLocalRef(threads[i]);
		}
	}

}

/**
int frameIndex, 判断するフレームID
jvmtiFrameInfo *frameInfos,　//フレームデータの配列
int posException 監視する例外のフレームID：必要じゃない
*/
bool StackTrace::validateExceptionPlace(int frameIndex, 
										jvmtiFrameInfo *frameInfos,
										int posException)
{
	//frameIfos:callback(Object thread, Object obj)
	//frameIfos+1:callback(Object obj) 
	jmethodID methodId = (frameInfos + frameIndex)->method;

	char *methodName = NULL, *methodSig = NULL;
	if (!HelperFunc::validateJvmtiError(param->jvmti, 
		param->jvmti->GetMethodName(methodId, &methodName, &methodSig, NULL), ""))
		return false;
	AUTO_REL_JVMTI_OBJECT(methodName);
	AUTO_REL_JVMTI_OBJECT(methodSig);

	jclass declaringClass = NULL;
	if (!HelperFunc::validateJvmtiError(param->jvmti,
		param->jvmti->GetMethodDeclaringClass(methodId, &declaringClass), ""))
		return false;
	AUTO_REL_JNI_LOCAL_REF(param->jni, declaringClass);
	
	char *classSig = NULL;
	if (!HelperFunc::validateJvmtiError(param->jvmti,
		param->jvmti->GetClassSignature(declaringClass, &classSig, NULL), ""))
		return false;
	AUTO_REL_JVMTI_OBJECT(classSig);


	char *newClassSig = HelperFunc::convertClassSig(classSig);  //発生するクラス名
	char *newMethodSig = HelperFunc::convertMethodSig(methodSig);

	bool ret = Global::getConfig()->isMonitoringTarget(param->monitorClassName, newClassSig, 
		methodName, newMethodSig);

	AUTO_REL_OBJECT(newClassSig);
	AUTO_REL_OBJECT(newMethodSig);

	return ret;
}

//to check if objectClass is sub class of parentClass
bool StackTrace::isSubClass(char* objectClass,char * parentClass)
{
	if(objectClass == NULL || parentClass == NULL)
	{
		return false;
	}
    if(strcmp(objectClass,parentClass) == 0)
	{
		return true;
	}
	if(strlen(parentClass) ==0 || strlen(objectClass) == 0)
	{
		return false;
	}
	//find the inserted class and method
	jclass classCallbacks = param->jni->FindClass("Callbacks");
	AUTO_REL_JNI_LOCAL_REF(param->jni, classCallbacks);
	if(classCallbacks == NULL)
	{
		LOG4CXX_ERROR(Global::logger, "can't get class Callbacks");
		return false;
	}

	jmethodID isSubClassMethod = param->jni->GetStaticMethodID(classCallbacks,"isSuperClass",
		"(Ljava/lang/Class;Ljava/lang/String;)Z");
	if(isSubClassMethod == NULL)
	{
		LOG4CXX_ERROR(Global::logger, "can't get method isSuperClass");
		return false;
	}

	//jstring objectClassString = param->jni->NewStringUTF(objectClass); 
	jstring parentClassString = param->jni->NewStringUTF(parentClass); 

	jboolean ret = (jboolean)param->jni->CallStaticBooleanMethod(
		classCallbacks,isSubClassMethod,param->exceptionCls,parentClassString);

	return ret == JNI_TRUE;
}

//to check if the exception happend in the process of mail
bool StackTrace::exceptionInMail(int frameCount, jvmtiFrameInfo *frameInfos)
{
	jmethodID methodId = (frameInfos + frameCount - 1)->method;

	char *methodName = NULL, *methodSig = NULL;
	if (!HelperFunc::validateJvmtiError(param->jvmti, 
		param->jvmti->GetMethodName(methodId, &methodName, &methodSig, NULL), ""))
		return false;
	AUTO_REL_JVMTI_OBJECT(methodName);
	AUTO_REL_JVMTI_OBJECT(methodSig);

	jclass declaringClass = NULL;
	if (!HelperFunc::validateJvmtiError(param->jvmti,
		param->jvmti->GetMethodDeclaringClass(methodId, &declaringClass), ""))
		return false;
	AUTO_REL_JNI_LOCAL_REF(param->jni, declaringClass);
	
	char *classSig = NULL;
	if (!HelperFunc::validateJvmtiError(param->jvmti,
		param->jvmti->GetClassSignature(declaringClass, &classSig, NULL), ""))
		return false;
	AUTO_REL_JVMTI_OBJECT(classSig);

	char *newClassSig = HelperFunc::convertClassSig(classSig);  //発生するクラス名
    
	AUTO_REL_OBJECT(newClassSig);

	if(strcmp(newClassSig,"Callbacks$SendMailThread") == 0 &&
		strcmp(methodName,"run")==0)
	{
		return true;
	}

	return false;
}

//to check if satisfied the dump condition
bool StackTrace::shouldDump(int checkPos, jvmtiFrameInfo *frameInfos)
{
	jmethodID methodId = (frameInfos + checkPos)->method;

	char *methodName = NULL, *methodSig = NULL;
	if (!HelperFunc::validateJvmtiError(param->jvmti, 
		param->jvmti->GetMethodName(methodId, &methodName, &methodSig, NULL), ""))
		return false;
	AUTO_REL_JVMTI_OBJECT(methodName);
	AUTO_REL_JVMTI_OBJECT(methodSig);

	jclass declaringClass = NULL;
	if (!HelperFunc::validateJvmtiError(param->jvmti,
		param->jvmti->GetMethodDeclaringClass(methodId, &declaringClass), ""))
		return false;
	AUTO_REL_JNI_LOCAL_REF(param->jni, declaringClass);
	
	char *classSig = NULL;
	if (!HelperFunc::validateJvmtiError(param->jvmti,
		param->jvmti->GetClassSignature(declaringClass, &classSig, NULL), ""))
		return false;
	AUTO_REL_JVMTI_OBJECT(classSig);

	char *newClassSig = HelperFunc::convertClassSig(classSig);  //発生するクラス名
	char *newMethodSig = HelperFunc::convertMethodSig(methodSig);
    
	AUTO_REL_OBJECT(newClassSig);
	AUTO_REL_OBJECT(newMethodSig);

	MethodDumpInfo* pDumpInfo = 
		AgentCallbackHandler::getMethodDumpInfo(param->classUrl, methodName, newMethodSig);
    if (NULL == pDumpInfo)
	{
		return false;
	}
	if (false == pDumpInfo->bValid)
	{
		return false;
	}
	//modofied by Qiu Song on 20090924 for メソッド監視の最高制限を削除する
	if (pDumpInfo->maxDumpCount != 0 && pDumpInfo->dumpCount >= pDumpInfo->maxDumpCount)
	{
		return false;
	}
    //end of modofied by Qiu Song on 20090924 for メソッド監視の最高制限を削除する

	if (pDumpInfo->dumpCondition.length() <= 1)
	{
		pDumpInfo->dumpCount++;
		param->className = newClassSig;
		param->methodName = methodName;
		param->methodSignature = newMethodSig;
		param->methodSuffix = pDumpInfo->methodSuffix;
		return true;
	}

	//use antlr to check condition
	string condition = pDumpInfo->dumpCondition;
	istrstream str(condition.c_str(), condition.length());

	if (Logger::getLogger()->isDebugEnabled())
	{
		string buf = "Enter getting condition value of \"";
		buf += condition;
		string msg = buf.substr(0, buf.length() -1);
		msg +=  "\".";
		msg += " (class: ";
		msg += newClassSig;
		msg += " methodname: ";
		msg += methodName;
		msg += " methodSignature: ";
		msg += newMethodSig;
		msg += ")";
		LOG4CXX_DEBUG(Logger::getLogger(), msg.c_str());
	}

	CcatLexer lexer(str);
	CcatParser parser(lexer);
	char* pszTmp = new char[128 + 1];
	memset(pszTmp, 0, 128+1);
	//make ParserParam parameter
	ParserParam* antlrParam = new ParserParam;
	antlrParam->jni = param->jni;
	antlrParam->jvmti = param->jvmti;
	antlrParam->thread = param->thread;
	antlrParam->depth = checkPos;
	antlrParam->frameInfo = frameInfos;

	bool bRet = parser.expr(pszTmp, false, antlrParam);
	if (!bRet)
	{
		if (strlen(pszTmp) != 0)
		{
			//TODO: error level より conditons status を設定する
			//pDumpInfo->bValid = false;
			string buf = "Cann't get bool value of condition \"";
			buf += condition;
			string errMsg = buf.substr(0, buf.length() -1);
			errMsg +=  "\". ";
			errMsg += pszTmp;
			errMsg += " (class: ";
			errMsg += newClassSig;
			errMsg += " methodname: ";
			errMsg += methodName;
			errMsg += " methodSignature: ";
			errMsg += newMethodSig;
			errMsg += ")";
			LOG4CXX_ERROR(Global::logger, errMsg.c_str());
		}
		else if (Logger::getLogger()->isDebugEnabled())
		{
			string buf = "Condition value of \"";
			buf += condition;
			string msg = buf.substr(0, buf.length() -1);
			msg +=  "\" is false.";
			msg += " (class: ";
			msg += newClassSig;
			msg += " methodname: ";
			msg += methodName;
			msg += " methodSignature: ";
			msg += newMethodSig;
			msg += ")";
			LOG4CXX_DEBUG(Global::logger, msg.c_str());
		}
		delete [] pszTmp;
		delete antlrParam;
		return false;
	}
	LOG4CXX_DEBUG(Logger::getLogger(), "condition value is true.");
	delete [] pszTmp;
	delete antlrParam;

	pDumpInfo->dumpCount++;
	param->className = newClassSig;
	param->methodName = methodName;
	param->methodSignature = newMethodSig;
	param->methodSuffix = pDumpInfo->methodSuffix;
	return true;
}

//to check if the exception happened should be excluded
bool StackTrace::shouldExclude(int posException,int frameCount,
							   jvmtiFrameInfo *frameInfos)
{
    MonitoringTarget* target = Global::getConfig(
		)->getMonitorTargetByName(param->monitorClassName);
	if(target == NULL)
	{
        Global::logger->logError("Can't get monitor target:%s",param->monitorClassName);
		return false;
	}
  
	vector<ExcludeClass*>* excludeVector = target->getExcludeClassVector();
	//if no exclude class,return false
	if(excludeVector == NULL || excludeVector->size() == 0)
	{
		return false;
	}

	vector<ExcludeClass*>::iterator it;
	for(it = excludeVector->begin(); it != excludeVector->end(); it++ )
	{
		ExcludeClass* excludeClass = *it;
		char* excludeClassName = excludeClass->getExcludeClassName();
		//if the exception happened is not the sub class of the excludeClass,not to eclude
		if(!isSubClass(param->exceptionName,excludeClassName))
		{
			continue;
		}
        //does this eclude class has place info?
		if(!excludeClass->hasPlaceInfo())
		{
			return true;
		}
		//use space in Config,because if you define a method using ExcludeClass
		//in StackTrace.h,you get compile error unkown
		Global::getConfig()->setCurrentExcludeClass(excludeClass);
        //does any frame matched the palce
		bool happened = false;
		for(int frameIndex =  posException ; frameIndex < frameCount; frameIndex++)
		{
			if (validateExcludePlace(frameIndex,frameInfos))
			{
				happened = true;
				break;
			}
		}

		if(happened)
		{
			return true;
		}
    }  
	return false;
}

/**
int frameIndex, 判断するフレームID
jvmtiFrameInfo *frameInfos,　フレームデータの配列
*/
bool StackTrace::validateExcludePlace(int frameIndex,jvmtiFrameInfo *frameInfos)
{
	//frameIfos:callback(Object thread, Object obj)
	//frameIfos+1:callback(Object obj) 
	jmethodID methodId = (frameInfos + frameIndex)->method;

	char *methodName = NULL, *methodSig = NULL;
	if (!HelperFunc::validateJvmtiError(param->jvmti, 
		param->jvmti->GetMethodName(methodId, &methodName, &methodSig, NULL), ""))
		return false;
	AUTO_REL_JVMTI_OBJECT(methodName);
	AUTO_REL_JVMTI_OBJECT(methodSig);

	jclass declaringClass = NULL;
	if (!HelperFunc::validateJvmtiError(param->jvmti,
		param->jvmti->GetMethodDeclaringClass(methodId, &declaringClass), ""))
		return false;
	AUTO_REL_JNI_LOCAL_REF(param->jni, declaringClass);
	
	char *classSig = NULL;
	if (!HelperFunc::validateJvmtiError(param->jvmti,
		param->jvmti->GetClassSignature(declaringClass, &classSig, NULL), ""))
		return false;
	AUTO_REL_JVMTI_OBJECT(classSig);


	char *newClassSig = HelperFunc::convertClassSig(classSig);  //発生するクラス名
	char *newMethodSig = HelperFunc::convertMethodSig(methodSig);
    
	ExcludeClass* excludeClass = Global::getConfig()->getCurrentExcludeClass();
	bool ret = excludeClass->shouldExclude(newClassSig,methodName, newMethodSig);

	AUTO_REL_OBJECT(newClassSig);
	AUTO_REL_OBJECT(newMethodSig);
	return ret;
}

bool StackTrace::writeToFile(const char* fileName,JNIEnv *jni)
{
	OutputToFile* opf = new OutputToFile();

	opf->setDumpType(param->dumpType);
	
	//add by Qiu Song on 20091016 for RefIDはHashに変更する
	opf->setObjectDatas(objectDataes);
	//end of add by Qiu Song on 20091016 for RefIdはHashに変更する
	bool ret = opf->writeToFile(fileName,jni,(char*)(param->utf8ExceptionName),threadName,
		strDumptime,objectDataes,methodDataes,instanceDataes, exceptionObjRefId, 
		param->threadStutas, param->cpuTime, param->threadPriority, monitorObjRefId, monitorClassName, 
		param->className, param->methodName, param->methodSignature, strDumpPosition, param->useMonitorThread,
		dumpCompleted);
	if(!ret)
	{
		string logBuf = "Failed to write dump file:";
		logBuf += fileName;
        LOG4CXX_ERROR_NATIVE(Global::logger, logBuf.c_str());
	}
	else
	{
		appendAutoDelFileName(fileName);
	}
	delete opf;

	return ret;
}

void StackTrace::appendAutoDelFileName(const char* fileName)
{
	int saveDays = Global::getConfig()->getOutputSetting()->getSaveDays();
	if (saveDays == 0)
		return;

	//make directory
    const char *filePath = Global::getConfig()->getDumpFilePath();
	char* dirNameBuf = new char[strlen(filePath) + 80];
	AUTO_REL_OBJECT_ARR(dirNameBuf);

	strcpy(dirNameBuf, filePath);
	strcat(dirNameBuf, FILE_SPERATOR);
	SystemInfo* systemInfo = SystemInfo::getInstance();
	strcat(dirNameBuf, systemInfo->getJvmID());
	strcat(dirNameBuf, "_DumpFileDelTask");
	
    if(!HelperFunc::makeDirectory(dirNameBuf))
	{
		string errmsgbuf = "Failed to make directory: ";
        errmsgbuf += dirNameBuf;
		LOG4CXX_ERROR_NATIVE(Global::logger, errmsgbuf.c_str());
		return;
    }
	
	//make autoDelListFile
	char* delFileName = getAutoDelListFileName(saveDays);

	AUTO_REL_OBJECT_ARR(delFileName);
	strcat(dirNameBuf, FILE_SPERATOR);
	strcat(dirNameBuf, delFileName);
	
	FILE* f = fopen(dirNameBuf, "a+");
	if (f == NULL)
	{
		string errmsgbuf = "Failed to write DumpFileDelTask file: ";
		errmsgbuf += dirNameBuf;
		LOG4CXX_ERROR_NATIVE(Global::logger, errmsgbuf.c_str());
		return;
	}
	setvbuf(f, NULL, _IONBF, 0);
	string fileNameStr = fileName;
	fileNameStr += "\n";
	fwrite(fileNameStr.c_str(), fileNameStr.length(), 1, f);
	fclose(f);
}

char* StackTrace::getAutoDelListFileName(int saveDays)
{
	//get sysdate 
	time_t ltime;
	time( &ltime );

	//get autodel date
	ltime += (saveDays * 24 + 12) * 3600;
	struct tm*  curTime = localtime(&ltime);

	//get autodel listfile name
	char* pAutoDelListName = new char[40];
	strftime(pAutoDelListName, 40, "%Y%m%d_DumpFileToBeDeleted.txt", curTime);
	return pAutoDelListName;
}

ObjectData* StackTrace::getObjectData(int id)
{
	return (ObjectData*)objectDataes->at(id); 

}

bool StackTrace::isDuplicate(jvmtiFrameInfo *frameInfos, int pos, int count)
{
	//this monitor will be auto released at the end of this method
	AutoMonitor Monitoring(Global::configMonitor);

	//get current stack trace.
	SimpleStackTrace *currentSimpleStackTrace = new SimpleStackTrace(
		param->jni, param->jvmti, frameInfos, pos, count, param->thread);

	if (Global::stackTraceCache == NULL) {
		Global::stackTraceCache = new deque<SimpleStackTrace*>();
	}

	long compareTimeLimit = Global::getConfig()->getTimeLimit();

	//remove the old stack trace
	time_t currentTime;
	time(&currentTime);
	while (!Global::stackTraceCache->empty()) 
	{
		SimpleStackTrace *stackTrace = Global::stackTraceCache->front();

		if (stackTrace->time < (currentTime - compareTimeLimit)) 
		{
			Global::stackTraceCache->pop_front();
		} 
		else 
		{
			break;
		}
	}

	//compare current stack trace with old stack traces
	bool found = false;
	deque<SimpleStackTrace*>::iterator it;
	for (it = Global::stackTraceCache->begin(); it != Global::stackTraceCache->end(); it++)
	{
		SimpleStackTrace* simpleStackTrace = *it;
		if (*simpleStackTrace == *currentSimpleStackTrace) {
			found = true;
			break;
		}
	}

	if (!found)
	{
		//add current stack trace to stack trace cache.
		Global::stackTraceCache->push_back(currentSimpleStackTrace);
	} else {
		delete currentSimpleStackTrace;
	}

	return found;
}

//add by Qiu Song on 20090818 for Excat Version 3.0
//スレッド状態とCPU時間を取得する
void StackTrace::getThreadStatusAndCPUTime()
{
	//スレッド状態を取得する
	jvmtiError err;
	jint threadStatus = 0;
	err = param->jvmti->GetThreadState(param->thread, &threadStatus);
	if (err != JVMTI_ERROR_NONE)
	{
		Global::logger->getLogger()->logError("failed to call method GetThreadState, err code = %d",err);
		return;
	}
	param->threadStutas = threadStatus;

	//CPU時間を取得する
	jlong cpuTime = 0;
	err = param->jvmti->GetThreadCpuTime(param->thread, &cpuTime);
	if (err != JVMTI_ERROR_NONE)
	{
		Global::logger->getLogger()->logError("failed to call method GetThreadCpuTime, err code = %d",err);
		return;
	}
	param->cpuTime = cpuTime;

	//スレッド優先度を取得する
	jvmtiThreadInfo threadInfo;
	err = param->jvmti->GetThreadInfo(param->thread, &threadInfo);
	if(err != JVMTI_ERROR_NONE)
	{
		Global::logger->logError("failed to get thread info ,err cd =%d",err);
		return;
	}
	param->threadPriority = threadInfo.priority;
}

//object poolからオブジェクトIDを取得する
int StackTrace::getObjRefIdFromObjectPool(jobject obj,ObjectPool* objectPool)
{
	if(obj != NULL)
	{
		int refId = objectPool->getObjectId(obj);
		ObjectData* objData = NULL;

		//object poolに存在しない場合、追加する
		if(refId < 0)
		{
			 refId = objectPool->addObject(obj);
			 objData = getObjectData(refId);
			 //ダンプするデータ量を増えた
			 this->addDumpDataSize(objData->getDumpSize());
		}
		else
		{
			objData = getObjectData(refId);
		}
	    //展開可能の場合、オブジェクトを展開する
		if(!objData->getExpanded())
		{
			objectPool->expandObject(obj,objData,this,0);
		}
		return refId;
	}

	//NULLの場合
	return -1;
}

//モニターオブジェクトクラス名を設定する
void StackTrace::setMonitorClassName()
{
	ObjectData* objData = getObjectData(monitorObjRefId);
	monitorClassName = objData->getType();
}

//field情報を格納するオブジェクトを取得する
ObjectData* StackTrace::getFieldObjectData(const char* className)
{

	vector<ObjectData*>::iterator iter;
	for (iter = objectDataes->begin(); iter != objectDataes->end(); ++iter) 
	{
	 	ObjectData* objData = (ObjectData*)(*iter);
		if(strcmp(objData->getType(), className) == 0)
		{
			return objData;
		}
	}
	return NULL;
}

//指定位置からobjectを削除する
void StackTrace::removeObjectFromPos(int nPos)
{
	if(objectDataes != NULL)
	{
		for(int i= nPos ; i < objectDataes->size() - 1 ; i++)
		{
			ObjectData* objDataTemp = (ObjectData*)objectDataes->at(i);
			if(objDataTemp != NULL)
			{
				delete objDataTemp;
				objDataTemp = NULL;
			}
		}
		vector<ObjectData*>::iterator iter = objectDataes->end();
		for (int j = objectDataes->size(); j > nPos; j --) 
		{
			objectDataes->pop_back();
		}
		for(int k= 0 ; k < objectDataes->size() ; k++)
		{
			ObjectData* objDataTemp = (ObjectData*)objectDataes->at(k);
		}
	}
}

//ダンプ開始時間を取得する
void StackTrace::setDumpTime()
{
	if(curDumpTime == NULL)
	{
		curDumpTime = new DumpFileTime();
	}
	else
	{
		return;
	}
	//struct _timeb timebuffer;
	//_ftime(&timebuffer);
	struct timeb timebuffer;
	ftime(&timebuffer);	struct tm *gt;
	gt = localtime(&(timebuffer.time));

	curDumpTime->m_year = gt->tm_year + 1900;
	curDumpTime->m_month = gt->tm_mon + 1;
	curDumpTime->m_day = gt->tm_mday;
	curDumpTime->m_hour = gt->tm_hour;
	curDumpTime->m_min = gt->tm_min;
	curDumpTime->m_sec = gt->tm_sec;
	curDumpTime->m_millianSec = timebuffer.millitm;
}

DumpFileTime* StackTrace::getDumpFileTime()
{
	return curDumpTime;
}

bool StackTrace::isThreadStatusMatch()
{
	OutputSetting *outputSetting = Global::getConfig()->getOutputSetting();
	char* strThreadStatus = outputSetting->getDumpAllThreads();
	if(strThreadStatus == NULL)
	{
		return false;
	}
	char* curThreadStatus = HelperFunc::getThreadStatusString(param->threadStutas);
	if(strcmp(strThreadStatus, "all") == 0 )
	{
		return true;
	}
	else if(strcmp(strThreadStatus, "no") == 0 || strcmp(strThreadStatus, "current") == 0)
	{
		return false;
	}
	else if(strcmp(strThreadStatus, "runnable") == 0 &&
		    strcmp(strThreadStatus, curThreadStatus) == 0)
	{
        return true;
	}
	else if(strcmp(strThreadStatus, "wait") == 0 && strcmp(curThreadStatus, "waiting") == 0
		    /*|| strcmp(curThreadStatus, "timed_waiting") == 0)*/)
	{
		return true;
	}
	return false;
}
//end of add by Qiu Song on 20090818  Excat Version 3.0

ExceedMemLimitException::ExceedMemLimitException(const char *param)
{
    if(param == NULL)
	{
       msg = NULL;
	}
	else
    {
		msg = HelperFunc::strdup(param);
    }
}

ExceedMemLimitException::~ExceedMemLimitException() throw()
{
	if(msg != NULL)
	{
		delete msg;
		msg = NULL;
	}
}

