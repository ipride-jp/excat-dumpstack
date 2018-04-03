#include "AgentCallbackHandler.h"
#include "ExceptionNameMan.h"
#include "ExceptionTableMan.h"

#include "../common/Global.h"
#include "../common/Define.h"
#include "../common/JniLocalRefAutoRelease.h"
#include "../common/JvmtiAutoRelease.h"
#include "../common/ObjectAutoRelease.h"
#include "../classfileoperation/ClassFile.h"
#include "../common/HelperFunc.h"
#include "../common/Config.h"
#include "../common/ReadCfgFileException.h"
#include "../common/SystemInfo.h"
#include "../common/MonitoringMethod.h"
#include "../common/SimpleStackTrace.h"
#include "../output/StackTrace.h"
#include "AgentRoutine.h"

#ifdef _WINDOWS
#include <Windows.h>
#endif

#ifdef _LINUX
#include <sys/file.h>
#include <dirent.h>
#include <fcntl.h>
#include <assert.h>
#endif

#include <algorithm>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <time.h>
#include <jvmti.h>

USE_NS(NS_JNI_UTILITY)
USE_NS(NS_CLASS_FILE_OPERATION)
USE_NS(NS_COMMON)

int    AgentCallbackHandler::WM_Init_Is_Ok = -1;
int    AgentCallbackHandler::WM_Death_Started = 0;
jclass AgentCallbackHandler::callbacksclass = NULL;
jmethodID AgentCallbackHandler::getClassUrlMethodId = NULL;
map<jint, jobject>* AgentCallbackHandler::loaders = NULL;//ロードされたクラス配列
map<string, short>* AgentCallbackHandler::loadersSig = NULL;
vector<string>*  AgentCallbackHandler::classToRedefine = NULL;
map<string, MonitoringMethodInfo*>*    AgentCallbackHandler::methodDumpInfo = NULL;
map<string, MonitoringClassLoadInfo*>* AgentCallbackHandler::loadedClassInfo = NULL;
map<string, vector<jweak>*>* AgentCallbackHandler::instanceMap = NULL;
jobject AgentCallbackHandler::lastException = NULL;
jthread AgentCallbackHandler::lastThread = NULL;//add by Qiu Song on 20090811 for 重複ダンプ防止
jobject AgentCallbackHandler::currentException = NULL;//add by Qiu Song on 20090811 for 例外オブジェクトダンプ
class ExceptionTableMan* AgentCallbackHandler::exceptionTableMan = new ExceptionTableMan();
vector<string>*  AgentCallbackHandler::excludeExceptionList = NULL;//add by Qiu Song on 20090907 for 除外クラス名(自動監視機能用)

map<jthread, jobject>* AgentCallbackHandler::dumpedExceptionObjList = NULL;//add by Qiu Song on 20090924 for 例外オブジェクト重複ダンプ

void JNICALL AgentCallbackHandler::classFileLoadHook(jvmtiEnv *jvmti, JNIEnv* jni, 
			jclass classBeingRedefined, jobject loader, const char* name, 
			jobject protectionDomain, jint classDataLen, const unsigned char* classData,
			jint* newClassDataLen, unsigned char** newClassData)
{
	if (WM_Death_Started == 1)
		return;

	//to get exception table for all methods
	if(exceptionTableMan == NULL)	
		return;
	ClassFile* pClassFile = new ClassFile(classDataLen, classData,exceptionTableMan);
	Logger::getLogger()->logTrace("register exception table for class:%s",
		 pClassFile->getClassNameOFMyself());
	delete pClassFile;

	//AutoMonitor Monitoring(Global::configMonitor);
	

    //すべてのクラスロードを記録する
    //性能のため、HashCodeを利用する。
	jobject gloader = loader;
    if (loader != NULL)
	{
		jint hashCode =0;
		jvmtiError err = jvmti->GetObjectHashCode(loader,&hashCode);
		if(err != JVMTI_ERROR_NONE)
		{
			return;
		}

		bool newLoader = false;
		if (loaders == NULL)
		{
			loaders = new map<jint, jobject>;
			gloader = jni->NewGlobalRef(loader);
			loaders->insert(pair<jint,jobject>(hashCode, gloader));
			newLoader = true;
		}
		else
		{
			map<jint, jobject>::const_iterator it = loaders->find(hashCode);
			
			//既にロードしたかどうか判断する
			if (it == loaders->end())
			{
				gloader = jni->NewGlobalRef(loader);
				loaders->insert(pair<jint,jobject>(hashCode, gloader));
				newLoader = true;				
			}
			else
			{
				gloader = it->second;//クラスオブジェクトデータを取得する
			}
		}
		if (newLoader)
		{
			jclass classLoader = jni->GetObjectClass(gloader);
			if (classLoader != NULL)
			{
				AUTO_REL_JNI_LOCAL_REF(jni, classLoader);
				char *classLoaderSig = NULL;
				if (JVMTI_ERROR_NONE == jvmti->GetClassSignature(classLoader, &classLoaderSig, NULL))
				{
					AUTO_REL_JVMTI_OBJECT(classLoaderSig);
					if (loadersSig == NULL)  loadersSig = new map<string, short>;
					loadersSig->insert(pair<string,short>(classLoaderSig, 1));
				} 
			}
		}
	}

    if (name == NULL)
		return;

    //Judge the class to be redefined by name
	if(false == Global::getConfig()->isRedefineClassName(name))
		return;

	Logger::getLogger()->logDebug("Enter classFileLoadHook.");
	Logger::getLogger()->logDebug(" class name: %s", name);

	//Callbacks class hasn't been loaded.
    if (callbacksclass == NULL)
	{
       if (classToRedefine == NULL)
		   classToRedefine = new vector<string>;
	   string className = name;
       classToRedefine->push_back(className);
       return;
	}

	//Call method Callbacks::getClassUrl to get class's url
	jobject path = jni->CallStaticObjectMethod(callbacksclass,getClassUrlMethodId,
											loader,jni->NewStringUTF(name));
    if (path == NULL)
	{
		Logger::getLogger()->logFatal("Failed to get file path of class: %s", name);
		return;
	}

	const char* value = jni->GetStringUTFChars((jstring)path, NULL);
	string classUrl = value;
    jni->ReleaseStringUTFChars((jstring)path,value);

	Logger::getLogger()->logDebug(" class's url: %s", classUrl.c_str());
 	//Judge the class to be redefined by name and url
	vector<MonitoringClass*>* methodV = Global::getConfig()->getMonitorClassInfo(name, classUrl.c_str());
	if (methodV->size() == 0)
	{
		delete methodV;
		return;
	}
	else if (methodV->size() > 1)
	{
		LOG4CXX_WARN(Global::logger, "One class matches duplicate MonitoringMethod or DumpInstance's definition.");
		Logger::getLogger()->logWarn("Class url: %s", classUrl.c_str());
		methodV->clear();
		delete methodV;
		return;
	}

	MonitoringClass* method = methodV->at(0);
	methodV->clear();
	delete methodV;

	vector<MethodDumpCondition*>* methodInfo = NULL;
	int   modifyType = method->getMonitoringType();
	
	//1:インスタンス監視 2:メソッド監視 3:メソッド監視+インスタンス監視
	if (modifyType != 1)
	{
		methodInfo = ((MonitoringMethod*)method)->getMethodInfo();
	}
	//if (true == haveRedefined(classUrl, method->getClassLoadString()))
	//{
	//	Logger::getLogger()->logDebug("This class has been redefined.");
	//	return;
	//}

	Logger::getLogger()->logDebug("Match one of monitoring method's definitions. class: %s  classLoadString: %s",
								   name, method->getClassLoadString());

	//judge class is interface or not
	ClassFile classFile(classDataLen, classData);
	if (true == classFile.isInterface())
	{
	    Logger::getLogger()->logError("Interface class should not be monitored. (class: %s  classUrl: %s)", name, classUrl.c_str());
		return;
	}

	Logger::getLogger()->logInfo("Find monitor class: %s classUrl: %s", name, classUrl.c_str());

	//insert calling callback method to monitorMethods 
	//prepare parameter
	string paramClassUrl = classUrl;
	char*  pTmp = HelperFunc::replaceChar(name, '/', '.');
	string paramClassName = pTmp;
	delete[] pTmp;
	
    int     insertMethodsCount = 0;
	string* insertMethodsName = NULL;
	string* insertMethodsSignature = NULL;
	int*    insertMethodsPosition = NULL;//add by Qiu Song on 20090828 for メソッド監視
	if (methodInfo != NULL)
	{
		insertMethodsCount = methodInfo->size();
		insertMethodsName = new string[insertMethodsCount];
		insertMethodsSignature = new string[insertMethodsCount];
		insertMethodsPosition = new int[insertMethodsCount];//add by Qiu Song on 20090828 for メソッド監視
    
		int index = 0;
		bool bRet = true;
		char buf[MAX_BUF_LEN];
		vector<MethodDumpCondition*>::iterator p;
		for (p = methodInfo->begin(); p != methodInfo->end(); ++p) 
		{
			insertMethodsName[index] = (*p)->getMethodName();
			insertMethodsSignature[index] = (*p)->getMethodSignature();
			insertMethodsPosition[index] = (*p)->getDumpPosition();//add by Qiu Song on 20090828 for メソッド監視
			buf[0] = 0;
			//judge method to be modified is valid or not
			bRet = classFile.isMethodDefValid(insertMethodsName[index].c_str(),
				insertMethodsSignature[index].c_str(), buf);
			if (false == bRet)
			{
				Logger::getLogger()->logError("%s  (class: %s classUrl: %s methodName: %s methodSignature: %s)", 
					buf, name, classUrl.c_str(),insertMethodsName[index].c_str(),insertMethodsSignature[index].c_str());
				delete [] insertMethodsName;
				delete [] insertMethodsSignature;
				delete[] insertMethodsPosition;//add by Qiu Song on 20090828 for メソッド監視
				return;
			}
			index++;
		}
	}

	if (Global::logger->isDebugEnabled())
	{
		LOG4CXX_DEBUG(Global::logger, "Come to insertByteCodeToClass");
		for (int i = 0; i < insertMethodsCount; i++)
		{
			LOG4CXX_DEBUG(Global::logger, insertMethodsName[i].c_str());
			LOG4CXX_DEBUG(Global::logger, insertMethodsSignature[i].c_str());
		}
	}

	unsigned char* newData = NULL;
	long newLen = 0;
	classFile.insertByteCodeToClass(modifyType, insertMethodsCount, insertMethodsName, 
									insertMethodsSignature, insertMethodsPosition,
									paramClassName, paramClassUrl, &newData, &newLen);
//test add byte code (comment by Qiu Song ：コメントべきだ TODO)
/*	Global::logger->logError("output the class file------start---------");
	char filePath[MAX_BUF_LEN];
	strcpy(filePath, "C:\\modify.class");
	
	FILE* stream = fopen(filePath, "wb");
	fwrite(newData, newLen,1,stream);
    fclose(stream);
	Global::logger->logError("output the class file-------end----------");*/
//end of test add byte code (comment by Qiu Song ：コメントべきだ TODO)
	
	if (insertMethodsCount > 0)
	{
		delete [] insertMethodsName;
		delete [] insertMethodsSignature;
		delete[] insertMethodsPosition;//add by Qiu Song on 20090828 for メソッド監視
	}

	if (Global::logger->isDebugEnabled())
	{	
		char* tmp = new char[128 + strlen(name)];
		sprintf(tmp, "redefine class name: %s", name);
		LOG4CXX_DEBUG(Global::logger, tmp);

		sprintf(tmp, "old len: %ld  new len:%ld", classDataLen, newLen);
		LOG4CXX_DEBUG(Global::logger, tmp);
		delete[] tmp;
	}

	unsigned char *jvmtiSpace;
	jvmti->Allocate(newLen, &jvmtiSpace);
	memcpy(jvmtiSpace, newData, newLen);

	*newClassData = jvmtiSpace;
	*newClassDataLen = (jint)newLen;

	delete [] newData;

	//add redefined class info to MonitoringMethod and AgentCallbackHandler::methodDumpInfo
	if (loadedClassInfo == NULL)
	{
		loadedClassInfo = new map<string, MonitoringClassLoadInfo*>;
	}
	map<string, MonitoringClassLoadInfo*>::const_iterator it;
	string key = method->getClassName();
	key.append("#");
	key.append(method->getClassLoadString());
	it = loadedClassInfo->find(key);
	if (it != loadedClassInfo->end())
	{
        MonitoringClassLoadInfo* p = it->second;
		p->addClassLoader(jni, gloader);
		vector<string>* urls = p->getClassUrls();

		int beforeSize = 0;
		if (urls != NULL)
			beforeSize = urls->size();

		p->addClassUrl(classUrl);

		urls = p->getClassUrls();
		int afterSize = 0;
		if (urls != NULL)
			afterSize = urls->size();

		if ((afterSize > beforeSize) && (afterSize > 1))
		{
			Logger::getLogger()->logWarn("Duplicated classes match the same MonitoringMethod's definition. className:%s  classLoadString: %s",
										 method->getClassName(), method->getClassLoadString());
			vector<string>::iterator it;
			int k = -1;
			for (it=urls->begin(); it != urls->end(); it++)
			{
				k++;
				Logger::getLogger()->logWarn("  url[%d] : %s", k, it->c_str());
			}
		}
	}
	else
	{
        MonitoringClassLoadInfo* p = new MonitoringClassLoadInfo();
		p->addClassLoader(jni, gloader);
		p->addClassUrl(classUrl);
        loadedClassInfo->insert(pair<string, MonitoringClassLoadInfo*>(key, p));
	}
	if (insertMethodsCount > 0)
	{
		addMethodDumpInfo((MonitoringMethod*)method, classUrl);
	}
}

void JNICALL AgentCallbackHandler::exceptionEvent(jvmtiEnv *jvmti, JNIEnv* jni, 
			jthread thread, jmethodID method, jlocation location, jobject exception, 
            jmethodID catch_method, jlocation catch_location)
{
    if (WM_Init_Is_Ok < 0 || WM_Death_Started == 1)
		return;
	//get exception signature
	jclass exceptionClazz  = jni->GetObjectClass(exception);
	if(exceptionClazz  == NULL)
    {
		Global::logger->logError("failed to call GetObjectClass in AgentCallbackHandler::exceptionEvent.");
		return;
	}
    AUTO_REL_JNI_LOCAL_REF(jni, exceptionClazz );
    
	//modified by Qiu Song on 20090811 for 重複ダンプ防止
	if(true == isSameExceptionObject(jni, thread, exception))
	{
		return;
	}

	//jrockitの場合、同じExceptionが重複発生する可能性がある
	/*if(Global::getIsJrockitJvm()){
		if(lastException == NULL){
			lastException = jni->NewGlobalRef(exception);
		}else{
			if(jni->IsSameObject(lastException,exception)){
				return;
			}else{
				jni->DeleteGlobalRef(lastException);
				lastException = jni->NewGlobalRef(exception);
			}
		}
	}*/
    //end of modified by Qiu Song on 20090811 for 重複ダンプ防止

	//add by Qiu Song on 20090811 for 例外オブジェクトダンプ
	if(currentException != NULL)
	{
		jni->DeleteGlobalRef(currentException);
	}
	currentException = jni->NewGlobalRef(exception);
	//end of add by Qiu Song on 20090811 for 例外オブジェクトダンプ

	char *exceptionClassSig  = NULL;
	jvmtiError err = jvmti->GetClassSignature(exceptionClazz , &exceptionClassSig , NULL);
	if(err != JVMTI_ERROR_NONE)
	{
		Global::logger->logError("failed to call GetClassSignature in AgentCallbackHandler::exceptionEvent,error cd =%d.",err);
		return ;
	}
	AUTO_REL_JVMTI_OBJECT(exceptionClassSig );

	//すべてのExceptionを監視する?
	if(Global::getConfig()->isMonitorAllExceptionMode())
	{
		//例外クラス名：Javaフォーマット
		char *newExceptionClassSig = HelperFunc::convertClassSig(exceptionClassSig);  
		AUTO_REL_OBJECT(newExceptionClassSig);

		
        //以下の固有Exceptionをダンプしない 
		//to do:外出し
	    //   java.lang.ClassNotFoundException
	    //   java.lang.NoSuchMethodException
	    //   java.lang.NoSuchFieldException
	    //   javax.naming.NamingException
	    //   java.net.MalformedURLException 
	    //   com.sun.org.apache.xerces.internal.xni.parser.XMLConfigurationException

		if(AgentCallbackHandler::isExcludeException(newExceptionClassSig) == true)
		{
			return;
		}
		/*JVMTIがcatch_methodをNULLに設定しても、実際当Exceptionをcatchメソッドが存在することもあります
			//Global::logger->logInfo("exception happened:%s",newExceptionClassSig);
			if(catch_method == NULL){
				dumpstackForAny(jni,thread,exceptionClazz,newExceptionClassSig);
				return;
			}
		*/
		
		//ダンプすべきかどうか、判断する
		if(exceptionTableMan == NULL){
			return;
		}
		ExceptionNameMan exceptionNameMan(jvmti,jni,thread,exceptionClazz,exceptionClassSig,
			newExceptionClassSig,location); 
		exceptionNameMan.setExceptionTableMan(exceptionTableMan);

		//add by Qiu Song on 20090924 for パッケージ名と同じフォルダを作成する
     	char* catchClassSig = new char[MAX_BUF_LEN];
		memset(catchClassSig, 0, MAX_BUF_LEN);
		//end of add by Qiu Song on 20090924 for パッケージ名と同じフォルダを作成する
		
		if(!exceptionNameMan.shouldDump(catchClassSig)){
			if(catchClassSig != NULL)
			{
				delete[] catchClassSig;
				catchClassSig = NULL;
			}
			return;
		}
		//add by Qiu Song on 20090924 for パッケージ名と同じフォルダを作成する
		if(strlen(catchClassSig) == 0)
		{
			catchClassSig = HelperFunc::getMethodDeclaringClassName(jvmti, jni, method);
		}
		char* newCatchClassSig = HelperFunc::convertClassSig(catchClassSig);
		if(catchClassSig != NULL)
		{
			delete catchClassSig;
			catchClassSig = NULL;
		}
		catchClassSig = HelperFunc::getPackageNameFromClassName(newCatchClassSig);
		if(newCatchClassSig != NULL)
		{
			delete newCatchClassSig;
			newCatchClassSig = NULL;
		}
        //end of add by Qiu Song on 20090924 for パッケージ名と同じフォルダを作成する
        
		//dump thread's stack
		dumpstackForAny(jni,thread,exceptionClazz,newExceptionClassSig, catchClassSig);
		if(catchClassSig != NULL)
		{
			delete[] catchClassSig;
			catchClassSig = NULL;
		}
	}
	else
	{
 		if(strcmp(exceptionClassSig ,"Ljava/lang/StackOverflowError;") == 0)
		{
			//LOG4CXX_ERROR(Global::logger, "stack over flow error happened.");
			dumpstackStackOverFlowError(jni,thread,exceptionClazz);
			return;
		}

		//find the Callback class and method
		jclass classCallbacks = jni->FindClass("Callbacks");
		if(classCallbacks == NULL)
		{
			LOG4CXX_ERROR(Global::logger, "exceptionEvent:can't get class Callbacks");
			return ;
		}

		jmethodID callbackMethod = jni->GetStaticMethodID(classCallbacks,"callback",
			"(Ljava/lang/Object;)V");
		if(callbackMethod == NULL)
		{
			LOG4CXX_ERROR(Global::logger, "can't get method callback");
			return ;
		}

		//AutoMonitor Monitoring(Global::configMonitor); //delete for bug185
		jni->CallStaticVoidMethod(classCallbacks,callbackMethod, exception);
	}
}

//this function will be called after VM Init event happened
void JNICALL AgentCallbackHandler::vmInitCallBack(jvmtiEnv *jvmti, JNIEnv* jni,jthread thread)
{
	//get jvm information  
	SystemInfo* systemInfo = SystemInfo::getInstance();
	systemInfo->getJVMInfo(jni);
    char* jvmVendor= systemInfo->getJvmVendor();
    if(strstr(jvmVendor,"IBM") != NULL ||strstr(jvmVendor,"ibm") != NULL )
	{
		Global::setIsIBMJvm(true);
	}else
	{
		Global::setIsIBMJvm(false);
	}

    if(strstr(jvmVendor,"BEA") != NULL ||strstr(jvmVendor,"bea") != NULL )
	{
		Global::setIsJrockitJvm(true);
	}else
	{
		Global::setIsJrockitJvm(false);
	}

	//add by Qiu Song on 20090910 for openJDK対応
	char* jvmName = systemInfo->getJvmName();
    if(strstr(jvmName,"OpenJDK") != NULL)
	{
		Global::setIsOpenJDK(true);
	}
	else
	{
		Global::setIsOpenJDK(false);
	}
	//end of add by Qiu Song on 20090910 for openJDK対応

	try
	{
		jvmtiError err;

		LOG4CXX_DEBUG(Global::logger, "VMInit event callback begined.");

		//load call back class : Callbacks
        jclass classCallbacks = jni->FindClass("Callbacks");
		if (classCallbacks == NULL)
		{
			LOG4CXX_FATAL(Global::logger, "vmInitCallBack:can't get class Callbacks.");
			jni->ExceptionClear();
			return;
		}

		//get method id of Callbacks::getClassUrl
		jmethodID callbackMethod = jni->GetStaticMethodID(classCallbacks,"getClassUrl",
			"(Ljava/lang/ClassLoader;Ljava/lang/String;)Ljava/lang/String;");
		if(callbackMethod == NULL)
		{
			LOG4CXX_FATAL(Global::logger, "cann't find method Callbacks::getClassUrl");
			return;
		}

		//try to call method Callbacks::getClassUrl
		jobject path = jni->CallStaticObjectMethod(classCallbacks,callbackMethod,
			NULL,jni->NewStringUTF("java.lang.Object"));
		if (path == NULL)
		{
			LOG4CXX_FATAL(Global::logger, "cann't get file path of class java.lang.Object");
			return;
		}

        //make global reference of Callbacks class
		callbacksclass = (jclass)(jni->NewGlobalRef(classCallbacks));
		if (callbacksclass == NULL)
		{
			LOG4CXX_ERROR(Global::logger, "can't make global ref of Callbacks class.");
			return;
		}
        //make global reference of Callbacks::getClassUrl
		getClassUrlMethodId = callbackMethod;

        //redefine classes that cann't be redefined at classFileLoadHook method
        if (classToRedefine != NULL)
		{
			int count = classToRedefine->size();
			int i = 0;
			string loadString = "";
			for (i=0; i< count;i++)
			{
				redefineClass(jvmti, jni, classToRedefine->at(i), loadString);
			}
			classToRedefine->clear();
			delete classToRedefine;
			classToRedefine = NULL;
		}

		//send exception names to java class
		if(!setMonitorClass(jni,Global::getConfig()))
		{
			return;
		}

		//set mail setting to java class
		setMailSetting(jni);

		int sleepTime = Global::getConfig()->getSleepTime();
		if(sleepTime <= 0)
		{
			 LOG4CXX_DEBUG(Global::logger, "Not to monitor config file because sleep time is 0.");
			 
		}

		//run monitoring thread 
		jclass classThread = jni->FindClass("java/lang/Thread");
		jmethodID constructor = jni->GetMethodID(classThread,"<init>","()V");
		jthread monitorThread = (jthread)jni->NewObject(classThread,constructor);
		err = jvmti->RunAgentThread(monitorThread,&AgentCallbackHandler::monitorThread,
			NULL,JVMTI_THREAD_NORM_PRIORITY);
		if (!HelperFunc::validateJvmtiError(jvmti, err, "Fail to call RunAgentThread for motoringThread."))
		{
			return;
		}	

		//run dumpFileDel thread
		string dumpFilePath = Global::getConfig()->getDumpFilePath();
		SystemInfo* systemInfo = SystemInfo::getInstance();
		char* jvmid = systemInfo->getJvmID();
		dumpFilePath += FILE_SPERATOR;
		dumpFilePath += jvmid;
		dumpFilePath += "_DumpFileDelTask";
	
		time_t ltime;
		time(&ltime);
		char  szCurrentTime[9];
		strftime(szCurrentTime, 9, "%Y%m%d", localtime(&ltime));

		//modified by Qiu Song on 20090810 for 自動削除機能の改善(起動する時)
		doOldTaskOfDelDumpFile(dumpFilePath.c_str(), szCurrentTime);
		/*jthread dumpFileDelThread = (jthread)jni->NewObject(classThread,constructor);
		err = jvmti->RunAgentThread(dumpFileDelThread,&AgentCallbackHandler::dumpFileDelThread,
			NULL,JVMTI_THREAD_NORM_PRIORITY);
		if (!HelperFunc::validateJvmtiError(jvmti, err, "Fail to call RunAgentThread for dumpFileDelThread."))
		{
			return;
		}*/
		//end of modified by Qiu Song on 20090810 for 自動削除機能の改善(起動する時)

		//add by Qiu Song on 20090907 for 除外例外一覧を呼び込む
		getExcludeExceptionList();
		//end of add by Qiu Song on 20090907 for 除外例外一覧を呼び込む
		WM_Init_Is_Ok = 1;
		LOG4CXX_DEBUG(Global::logger, "VMInit event callback ended.");
	}catch(exception& e)
	{
        LOG4CXX_ERROR(Global::logger, e.what());
		Global::setErrorStatus(true); 
	}catch(...)
	{
		LOG4CXX_ERROR(Global::logger,"Unknown error happened in vmInitCallBack");
		Global::setErrorStatus(true); 
	}	

}

bool AgentCallbackHandler::setMonitorClass(JNIEnv* jni,Config* pConfig)
{
   //get the exception names to be monitored
    vector<MonitoringTarget*> *monitorTarget = pConfig->getMonitoringTargets();

	//find the inserted class and method
	/*jclass classCallbacks = jni->FindClass("Callbacks");
	if(classCallbacks == NULL)
	{
		LOG4CXX_FATAL(Global::logger, "can't get class Callbacks");
		return false;
	}
	*/
	jmethodID setExeptionsMethod = jni->GetStaticMethodID(callbacksclass,"setMonitorExceptions",
		"([Ljava/lang/String;)V");

    jclass classString = jni->FindClass("java/lang/String");
    
    int allExceptionNumber = monitorTarget->size();

	jstring defaultString = jni->NewStringUTF(""); 
    jobjectArray exceptionArray = jni->NewObjectArray(allExceptionNumber,
		classString,defaultString); 

  	for (int index = 0; index < monitorTarget->size(); index++) 
	{
		MonitoringTarget *target = (MonitoringTarget *)monitorTarget->at(index);
		jstring exceptionNameString = jni->NewStringUTF(target->getThrowableClass()); 
        //監視する例外
		jni->SetObjectArrayElement(exceptionArray,index,exceptionNameString);
	}
	
	//send the exception names to the inserted class in java
 	jni->CallStaticVoidMethod(callbacksclass,setExeptionsMethod,exceptionArray);

	return true;
	
}

void AgentCallbackHandler::setMailSetting(JNIEnv *jni) {
	//set mail setting to java.
    map<string, string> *mailSetting = Global::getConfig()->getMailSetting();

	if (mailSetting == NULL) {
		return;
	}

	//create an instance of hashmap.
    jclass classHashMap = jni->FindClass("java/util/HashMap");
	jmethodID methodInit = jni->GetMethodID(classHashMap,"<init>",
		"()V");
	jobject mapObj = jni->NewObject(classHashMap, methodInit);
	jmethodID methodPut = jni->GetMethodID(classHashMap, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

	map<string, string>::iterator iter = mailSetting->begin();
	while (iter != mailSetting->end()) {
		jni->CallObjectMethod(mapObj, methodPut, 
			jni->NewStringUTF(iter->first.c_str()), jni->NewStringUTF(iter->second.c_str()));
		++iter;
	}
	
	jmethodID methodSetMailSetting = jni->GetStaticMethodID(callbacksclass, "setNotifierSetting", "(Ljava/util/HashMap;)V");
	jni->CallStaticVoidMethod(callbacksclass, methodSetMailSetting, mapObj);
}

//*arg is not used
void JNICALL AgentCallbackHandler::monitorThread(jvmtiEnv* jvmti_env, JNIEnv* jni_env, void* arg)
{
      //added by gancl
	  if (Global::isJVMToUnload == 1)
	  {
		  Global::monitorThreadStatus = 0;
		  return;
	  }
	  Global::monitorThreadStatus = 1;

	  //end of added by gancl
	  LOG4CXX_DEBUG(Global::logger, "Monitor thread begin...");

	 if(Global::isErrorStatus())
	 {
		LOG4CXX_ERROR(Global::logger, "Now is in erorr status,Check config thread will exit.");
		Global::monitorThreadStatus = 0;  //added by gancl
		return;
	 }

	 int sleepTime = Global::getConfig()->getSleepTime();
     if(sleepTime <= 0)
	 {
         LOG4CXX_INFO(Global::logger, "Not to monitor config file because sleep time is 0.");
		 Global::monitorThreadStatus = 0;  //added by gancl
         return;
	 }else
	 {
		char buf[MAX_BUF_LEN];
		sprintf(buf, "The monitor thread's sleep time is %d ", sleepTime);
		LOG4CXX_DEBUG(Global::logger, buf);

	 }
	 const char* configFile = Global::getConfig()->getConfigFileName();

	 struct stat st;
	 int ret = stat(configFile,&st);
	 if(ret != 0)
	 {
		 LOG4CXX_ERROR(Global::logger, "Can't get config file infomation.");
		 Global::monitorThreadStatus = 0;  //added by gancl
		 return;
	 }

	 //the last  modified time
	 time_t   modifyTime  = st.st_mtime;
	 
	 while(true)
	 {
        //mySleep(sleepTime);
		for (int i= sleepTime; i > 0; i--)
		{
			HelperFunc::mySleep(1);
			if (Global::isJVMToUnload == 1)
			{
				Global::monitorThreadStatus = 0;
				return;
			}
		}
		if(Global::isErrorStatus())
		{
			LOG4CXX_ERROR(Global::logger, "Now is in erorr status,Check config thread will exit.");
		    Global::monitorThreadStatus = 0;  //added by gancl
			return;
		}

	   //add by Qiu Song on 20091009 for 自動削除タイミングの追加(毎日夜12時と深夜24時)
	   time_t nowtime;
	   time(&nowtime);
	   struct tm *nowtimeGt;
	   nowtimeGt = localtime(&nowtime);
       if(nowtimeGt->tm_min == 0 && nowtimeGt->tm_sec == 0 && 
		   ( nowtimeGt->tm_hour == 0 || nowtimeGt->tm_hour == 12 || nowtimeGt->tm_hour == 24) )
	   {
		   string dumpFilePath = Global::getConfig()->getDumpFilePath();
		   SystemInfo* systemInfo = SystemInfo::getInstance();
		   char* jvmid = systemInfo->getJvmID();
		   dumpFilePath += FILE_SPERATOR;
		   dumpFilePath += jvmid;
		   dumpFilePath += "_DumpFileDelTask";
		   
		   char  szCurrentTime[9];
		   strftime(szCurrentTime, 9, "%Y%m%d", nowtimeGt);
		   doOldTaskOfDelDumpFile(dumpFilePath.c_str(), szCurrentTime);
	   }
	   //end of add by Qiu Song on 20091009 for 自動削除タイミングの追加(毎日夜12時と深夜24時)

//		if(Global::getConfig() == NULL)
//		{
//			return;//vm ended
//		}

		configFile = Global::getConfig()->getConfigFileName();

		ret = stat(configFile,&st);
		if(ret != 0)
		{
           LOG4CXX_WARN(Global::logger, "Can't get config file information after sleep.");
		}else
		{
           time_t   newTime  = st.st_mtime;
		   if(newTime == modifyTime)
		   {
              LOG4CXX_DEBUG(Global::logger, "The config file is not changed");
		   }else
		   {
			   //設定変更する時の処理
			   LOG4CXX_INFO(Global::logger, "The config file may be changed");
               modifyTime = newTime;

			   Config* configNew = NULL;
			   try{
				   configNew = new Config();
				   configNew->init((char*)configFile);

				   //delete by Qiu Song on 20090929 for configファイル更新判断機能の改善
				   /*bool bRet = Global::getConfig()->hasMonitoringClassesChanged(*configNew);
				   if (bRet == false)
				   {
					   if(*configNew == *(Global::getConfig()))
					   {
						   LOG4CXX_INFO(Global::logger,"The config content is same with the old one.");
						   delete configNew;
						   configNew = NULL;
						   continue;
					   }
				   }
				   LOG4CXX_INFO(Global::logger, "The config file was really changed");*/
				   LOG4CXX_INFO(Global::logger, "The config file was changed");
                   //end of delete by Qiu Song on 20090929 for configファイル更新判断機能の改善

				   //we must call setMonitorClass first,because a lock in java
				   //will be held
				   //send exeption class to java
				   setMonitorClass(jni_env,configNew);

                   Config* temp = Global::getConfig();
				   //reset config after get the mutex
				   AutoMonitor Monitoring(Global::configMonitor);
                   Global::setConfig(configNew);
				   configNew->logConfig();
				   //set mail setting to java
				   setMailSetting(jni_env);
				   //if (bRet) //when bRet is false, change output setting
				   {
					   redefineClasses(jvmti_env, jni_env, temp->getMonitorMethodDefine());
				   }
				   delete temp;
				   int newSleepTime = Global::getConfig()->getSleepTime();
				   if(newSleepTime <= 0 )
				   {
                      LOG4CXX_ERROR(Global::logger,"The sleep time can't be less than 0 and the sleep time will not be changed.");
                   }else
				   {
                      sleepTime = newSleepTime;
				   }
                   configFile = Global::getConfig()->getConfigFileName();

				   //add by Qiu Song on 20090810 for 自動削除機能の改善(設定変更する時)
				   string dumpFilePath = Global::getConfig()->getDumpFilePath();
				   SystemInfo* systemInfo = SystemInfo::getInstance();
				   char* jvmid = systemInfo->getJvmID();
				   dumpFilePath += FILE_SPERATOR;
				   dumpFilePath += jvmid;
				   dumpFilePath += "_DumpFileDelTask";
				   
				   time_t ltime;
				   time(&ltime);
				   char  szCurrentTime[9];
				   strftime(szCurrentTime, 9, "%Y%m%d", localtime(&ltime));

				   doOldTaskOfDelDumpFile(dumpFilePath.c_str(), szCurrentTime);
				   //end of add by Qiu Song 20090810 for 自動削除機能の改善(設定変更する時)
			   }catch(ReadCfgFileException& e )
			   {
				    if(configNew != NULL)
					{
					    delete configNew;
					    configNew = NULL;
					}
                    LOG4CXX_ERROR(Global::logger,"The config file is not correct.");
		            LOG4CXX_ERROR(Global::logger, e.getErrorMsg());
			   }catch(exception& e)
			   {
				    if(configNew != NULL)
					{
					   delete configNew;
					   configNew = NULL;
					}
				    LOG4CXX_ERROR(Global::logger, e.what());
					Global::setErrorStatus(true); 
			   }catch(...)
			   {
				    if(configNew != NULL)
					{
					   delete configNew;
					   configNew = NULL;
					}
					LOG4CXX_ERROR(Global::logger,"Unknown error happened in monitorThread");
					Global::setErrorStatus(true); 
			   }
			   
		   }
		}
	 }
}

//廃棄しました(comment by Qiu Song on 20090810 for 自動削除機能の改善)
//*arg is not used
void JNICALL AgentCallbackHandler::dumpFileDelThread(jvmtiEnv* jvmti_env, JNIEnv* jni_env, void* arg)
{
	if (Global::isJVMToUnload == 1)
	{
		Global::dumpFileDelThreadstatus = 0;
		return;
	}
	Global::dumpFileDelThreadstatus = 1;
	
	LOG4CXX_DEBUG(Global::logger, "DumpFileDel thread begin...");
	
	string dumpFilePath = Global::getConfig()->getDumpFilePath();
	SystemInfo* systemInfo = SystemInfo::getInstance();
	char* jvmid = systemInfo->getJvmID();
	dumpFilePath += FILE_SPERATOR;
	dumpFilePath += jvmid;
	dumpFilePath += "_DumpFileDelTask";
	
	time_t ltime;
	time(&ltime);
	char  szCurrentTime[9];
	strftime(szCurrentTime, 9, "%Y%m%d", localtime(&ltime));
	doOldTaskOfDelDumpFile(dumpFilePath.c_str(), szCurrentTime);
	if (Global::isJVMToUnload == 1)
	{
		Global::dumpFileDelThreadstatus = 0;
		return;
	}
	
	int sleepTime = 300;  //seconds
	while(true)
	{
		//sleep for 5 miniutes
		for (int i= sleepTime; i > 0; i--)
		{
			HelperFunc::mySleep(1);
			if (Global::isJVMToUnload == 1)
			{
				Global::dumpFileDelThreadstatus = 0;
				return;
			}
		}

		time(&ltime);
		strftime(szCurrentTime, 9, "%Y%m%d", localtime(&ltime));

		//check if the deltask file is exist
		string delTaskPath = Global::getConfig()->getDumpFilePath();
		delTaskPath += FILE_SPERATOR;
		delTaskPath += jvmid;
		delTaskPath += "_DumpFileDelTask";

		if (strcmp(delTaskPath.c_str(), dumpFilePath.c_str()) != 0)
		{
			doOldTaskOfDelDumpFile(delTaskPath.c_str(), szCurrentTime);
			if (Global::isJVMToUnload == 1)
			{
				Global::dumpFileDelThreadstatus = 0;
				return;
			}
			dumpFilePath = delTaskPath;
		}

		if (false == HelperFunc::isValidDir((char*)(delTaskPath.c_str())))
		{
			continue;
		}

		delTaskPath += FILE_SPERATOR;
		delTaskPath += szCurrentTime;
		delTaskPath += "_DumpFileToBeDeleted.txt";
		if (false == HelperFunc::isValidFile(delTaskPath.c_str()))
		{
			continue;
		}
		delDumpFile(delTaskPath.c_str());
		if (Global::isJVMToUnload == 1)
		{
			Global::dumpFileDelThreadstatus = 0;
			return;
		}
	}
}

void AgentCallbackHandler::doOldTaskOfDelDumpFile(const char* delTaskFilePath, char* pCurrentTime)
{
	//check dir
	if (false == HelperFunc::isValidDir((char*)delTaskFilePath))
	{
		return;
	}

#ifdef _LINUX
	//search task file
	DIR* p=opendir(delTaskFilePath);
	if (p == NULL)
	{
		return;
	}

	struct dirent*  dirlist;
	struct stat     filestat;
	while((dirlist=readdir(p))!=NULL)
	{   
		if  ((strcmp(dirlist->d_name,".")==0)
			||(strcmp(dirlist->d_name,"..")==0))
			continue;

		if (isValidTaskFileName(dirlist->d_name)
			&& (strncmp(dirlist->d_name, pCurrentTime, 8) <= 0))
		{
			string fileName = delTaskFilePath;
			fileName += FILE_SPERATOR;
			fileName += dirlist->d_name;

			stat(fileName.c_str(),&filestat);
			if(S_ISREG(filestat.st_mode))     
			{   
				delDumpFile(fileName.c_str());
			}
		} 
		if (Global::isJVMToUnload == 1)
		{
			closedir(p);
			return;
		}
	}   
	
	closedir(p);

#else
	//search task file
	WIN32_FIND_DATA  *fs= new WIN32_FIND_DATA;
	string path = delTaskFilePath;
	path += FILE_SPERATOR;
	path += "*_DumpFileToBeDeleted.txt";

	HANDLE  fhwnd=FindFirstFile(path.c_str() ,fs);
	if (fhwnd==NULL)
	{
		delete fs;
		return;
	}

	do {
		if (fs->dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
		{
			if (isValidTaskFileName(fs->cFileName)
				&& (strncmp(fs->cFileName, pCurrentTime, 8) <= 0))
			{
				string fileName = delTaskFilePath;
				fileName += FILE_SPERATOR;
				fileName += fs->cFileName;
				delDumpFile(fileName.c_str());
			}
		}
		if (Global::isJVMToUnload == 1)
		{
			FindClose(fhwnd);
			delete fs;
			return;
		}
	}while(FindNextFile(fhwnd,fs) != NULL );

	FindClose(fhwnd);
	delete fs;
#endif
}

void AgentCallbackHandler::delDumpFile(const char* delTaskFilePath)
{
#ifdef _LINUX
	FILE* taskFile = fopen(delTaskFilePath, "r+");
	if (NULL == taskFile)
	{
		string errMsg = "Failed to open DumpFileDelTask file: ";
		errMsg += delTaskFilePath;
		LOG4CXX_WARN(Global::logger, errMsg.c_str());
		return;
	}

	//lock file 
	int fd = fileno(taskFile);
	struct flock sLock;
	sLock.l_type = F_WRLCK;
	sLock.l_whence = SEEK_SET;
	sLock.l_start = 0;
	sLock.l_len = 0;
	if (-1 == fcntl(fd, F_SETLKW,&sLock))
	{
		fclose(taskFile);
		return;
	}
	sLock.l_type = F_UNLCK ;
	
	char* buf = new char[1024+1];
	AUTO_REL_OBJECT_ARR(buf);
	while (!feof(taskFile))
	{
		fgets(buf, 1025, taskFile);
		if (strlen(buf) > 1)
		{
			buf[strlen(buf) - 1] = 0;  //del '\n' at the end of line
			//modified by Qiu Song on 20091013 for 自動削除機能のフォルダ削除
			HelperFunc::removeDumpFile(buf);
			/*if (HelperFunc::isValidFile(buf))
			{
				//delete dump file
				HelperFunc::removeFile(buf);
			}*/
			//end of modified by Qiu Song on 20091013 for 自動削除機能のフォルダ削除
		}
		if (Global::isJVMToUnload == 1)
		{
			fcntl(fd,F_SETLKW,&sLock);
			fclose(taskFile);
			return;				
		}
	}

	fcntl(fd,F_SETLKW,&sLock);
	fclose(taskFile);

	//delete task file
	HelperFunc::removeFile(delTaskFilePath);
#else
	HANDLE hFile = CreateFile(delTaskFilePath, GENERIC_READ, 0, NULL, 
							  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		string errMsg = "Failed to open DumpFileDelTask file: ";
		errMsg += delTaskFilePath;
		LOG4CXX_WARN(Global::logger, errMsg.c_str());
		return;
	}
	LockFile(hFile,0,0,GetFileSize(hFile,NULL),0);

	char  lineEnd[3] = {0x0d, 0x0a, 0};
	char* readBuf = new char[1024+1];
	char* parseBuf = new char[1024+1];
	char* pReadBuf = readBuf;
	char* pParseBuf = parseBuf;
	AUTO_REL_OBJECT_ARR(readBuf);
	AUTO_REL_OBJECT_ARR(parseBuf);
	int byteToBeRead = 1024;
	unsigned long   readBytes = 0;
	int parseIndex = 0;
	readBuf[0] = 0;
	
	do{
		memset(readBuf+strlen(readBuf), 0, byteToBeRead+1);
		BOOL bRet = ReadFile(hFile, readBuf+strlen(readBuf), byteToBeRead, &readBytes, NULL);
		if (bRet == FALSE)
		{
			string errMsg = "Failed to read DumpFileDelTask file: ";
			errMsg += delTaskFilePath;
			LOG4CXX_WARN(Global::logger, errMsg.c_str());

			UnlockFile(hFile,0,0,GetFileSize(hFile,NULL),0);
			CloseHandle(hFile);
			return;
		}
		else if (readBytes > 0)
		{
			pReadBuf = readBuf;
			char* p = NULL;
			do{
				p = strstr(pReadBuf, lineEnd);
				if (p != NULL)
				{
					while  ((*p == 0x0d) || (*p == 0x0a))
					{
						*p = 0;
						p++;
					}

					//modified by Qiu Song on 20091013 for 自動削除機能のフォルダ削除
					HelperFunc::removeDumpFile(pReadBuf);
					/*if (HelperFunc::isValidFile(pReadBuf))
					{
						//delete dump file
						HelperFunc::removeFile(pReadBuf);
					}*/
					//end of modified by Qiu Song on 20091013 for 自動削除機能のフォルダ削除
					pReadBuf = p;
				}
				else if (readBytes < byteToBeRead)
				{
					if (strlen(pReadBuf) > 0)
					{
						//modified by Qiu Song on 20091013 for 自動削除機能のフォルダ削除
						HelperFunc::removeDumpFile(pReadBuf);
						/*if (HelperFunc::isValidFile(pReadBuf))
						{
							//delete dump file
							HelperFunc::removeFile(pReadBuf);
						}*/
						//end of modified by Qiu Song on 20091013 for 自動削除機能のフォルダ削除
					}
					readBytes = 0;
					break;
				}
				else
				{
					strcpy(pParseBuf, pReadBuf);
					strcpy(readBuf, pParseBuf);
					byteToBeRead = 1024 - strlen(readBuf);
					if (byteToBeRead == 0)
					{
						readBuf[0] = 0;
						byteToBeRead = 0;
					}
					break;
				}
			}while (true);
		}
		if (Global::isJVMToUnload == 1)
		{
			UnlockFile(hFile,0,0,GetFileSize(hFile,NULL),0);
			CloseHandle(hFile);
			return;
		}
	} while (readBytes > 0);

	UnlockFile(hFile,0,0,GetFileSize(hFile,NULL),0);
	CloseHandle(hFile);

	//delete task file
	HelperFunc::removeFile(delTaskFilePath);
#endif
}

bool  AgentCallbackHandler::isValidTaskFileName(const char* delTaskFilePath)
{
	if (strlen(delTaskFilePath) != strlen("_DumpFileToBeDeleted.txt") + 8)
		return false;

	char  szPrefix[9];
	strncpy(szPrefix, delTaskFilePath, 8);
	szPrefix[8] = 0;

	for (int i = 0; i < 8; i++)
	{
		if ((szPrefix[i] < '0')
			|| (szPrefix[i] > '9'))
		{
			return false;
		}
	}
	return true;
}


//this function will be called after data dump request event happened
void JNICALL AgentCallbackHandler::dataDumpRequest(jvmtiEnv *jvmti)
{
	Config* config = Global::getConfig();

	if(config->toMonitorSignal())
	{
		LOG4CXX_INFO(Global::logger, "Excat:Dump java thread info...");
		activateAgentThread();
	}
}

//this function will be called after VM Init event happened
void JNICALL AgentCallbackHandler::vmDeathCallBack(jvmtiEnv *jvmti, JNIEnv* jni)
{
	WM_Death_Started = 1;

	if(lastException != NULL){
		jni->DeleteGlobalRef(lastException);
	}

	if (loaders != NULL)
	{
		map<jint, jobject>::const_iterator it;
		for (it = loaders->begin(); it != loaders->end(); it++)
		{
			jni->DeleteGlobalRef(it->second);
		}
		loaders->clear();
		delete loaders;
		loaders = NULL;
	}
	
	if (loadersSig != NULL)
	{
		loadersSig->clear();
		delete loadersSig;
		loadersSig = NULL;
	}

	if (callbacksclass != NULL)
	{
	    jni->DeleteGlobalRef(callbacksclass);
        callbacksclass = NULL;
	}

    if (classToRedefine != NULL)
	{
		classToRedefine->clear();
		delete classToRedefine;
		classToRedefine = NULL;
	}

	if (methodDumpInfo != NULL)
	{
		map<string, MonitoringMethodInfo*>::const_iterator it;
		for (it = methodDumpInfo->begin(); it != methodDumpInfo->end(); it++)
		{
			delete it->second;
		}
		methodDumpInfo->clear();
		delete methodDumpInfo;
		methodDumpInfo = NULL;
	}
	if (loadedClassInfo != NULL)
	{
		map<string, MonitoringClassLoadInfo*>::const_iterator it;
		for (it = loadedClassInfo->begin(); it != loadedClassInfo->end(); it++)
		{
			delete it->second;
		}
		loadedClassInfo->clear();
		delete loadedClassInfo;
		loadedClassInfo = NULL;
	}
	if (instanceMap != NULL)
	{
		map<string, vector<jweak>*>::const_iterator it;
		for (it = instanceMap->begin(); it != instanceMap->end(); it++)
		{
			vector<jweak>* value = it->second;
			if (value != NULL)
			{
				vector<jweak>::iterator it1 = value->begin();
				while (it1 != value->end())
				{
					jni->DeleteWeakGlobalRef(*it1);
					it1++;
				}
				value->clear();
				delete value;
				value = NULL;
			}
		}
		instanceMap->clear();
		delete instanceMap;
		instanceMap = NULL;
	}

	//add by Qiu Song on 20090825 for メソッド監視
/*	if(Global::classFieldTable != NULL)
	{
		map<string, map<int, int>*>::const_iterator it;
		for(it = Global::classFieldTable->begin(); it != Global::classFieldTable->end(); it++)
		{
			map<int,int>* fieldTable = it->second;
			if(fieldTable != NULL)
			{
				fieldTable->clear();
				delete fieldTable;
				fieldTable = NULL;
			}
		}
		Global::classFieldTable->clear();
		delete Global::classFieldTable;
		Global::classFieldTable = NULL;
	}

	if(Global::classConstantPool != NULL)
	{
		map<string, map<int, string>*>::const_iterator it;
		for(it = Global::classConstantPool->begin(); it != Global::classConstantPool->end(); it++)
		{
			map<int,string>* ldcTable = it->second;
			if(ldcTable != NULL)
			{
				ldcTable->clear();
				delete ldcTable;
				ldcTable = NULL;
			}
		}
		Global::classConstantPool->clear();
		delete Global::classConstantPool;
	    Global::classConstantPool = NULL;
	}
*/
	if(AgentCallbackHandler::excludeExceptionList != NULL)
	{
		AgentCallbackHandler::excludeExceptionList->clear();
		delete AgentCallbackHandler::excludeExceptionList;
		AgentCallbackHandler::excludeExceptionList = NULL;
	}

	if(AgentCallbackHandler::dumpedExceptionObjList != NULL)
	{
		map<jthread, jobject>::const_iterator it;
		for (it = AgentCallbackHandler::dumpedExceptionObjList->begin(); it != AgentCallbackHandler::dumpedExceptionObjList->end(); it++)
		{
			jni->DeleteGlobalRef(it->first);
			jni->DeleteGlobalRef(it->second);
		}
		AgentCallbackHandler::dumpedExceptionObjList->clear();
		delete AgentCallbackHandler::dumpedExceptionObjList;
		AgentCallbackHandler::dumpedExceptionObjList = NULL;
	}
	//end of add by Qiu Song on 20090825 for メソッド監視

	delete exceptionTableMan;
}

void AgentCallbackHandler::addMethodDumpInfo(MonitoringMethod* method, string classUrl)
{
	if (methodDumpInfo == NULL)
	{
		methodDumpInfo = new map<string, MonitoringMethodInfo*>;
	}
	map<string, MonitoringMethodInfo*>::const_iterator it 
		= methodDumpInfo->find(classUrl);
	if (it != methodDumpInfo->end())
	{
		MonitoringMethodInfo* p = it->second;
		methodDumpInfo->erase(classUrl);
        delete p;
	}
    MonitoringMethodInfo* pInfo = new MonitoringMethodInfo(method);
	methodDumpInfo->insert(pair<string, MonitoringMethodInfo*>(classUrl, pInfo));
}


vector<jclass>* AgentCallbackHandler::FindClass(JNIEnv* jni, char* className)
{
	jmethodID callbackMethod = jni->GetStaticMethodID(callbacksclass,"getClassObject",
		"(Ljava/lang/ClassLoader;Ljava/lang/String;)Ljava/lang/Class;");
	if(callbackMethod == NULL)
	{
		LOG4CXX_FATAL(Global::logger, "Cann't find method Callbacks::getClassObject");
		return NULL;
	}
		
	vector<jclass>* classVec = NULL;
    jobject classobj = jni->CallStaticObjectMethod(callbacksclass,callbackMethod,
								NULL,jni->NewStringUTF(className));
	if (classobj != NULL)
	{
		classVec = new vector<jclass>();
		classVec->push_back((jclass)classobj);
	}
	
	if (loaders == NULL)
	{
		return classVec;
	}
	
	map<string, int>* foundUrls = new map<string, int>;
	//call Callbacks::getClassUrl()
	jobject path = jni->CallStaticObjectMethod(callbacksclass,getClassUrlMethodId,
		NULL,jni->NewStringUTF(className));
	if (path != NULL)
	{
		const char* pTmp = jni->GetStringUTFChars((jstring)path, NULL);
		string buf = pTmp;
		jni->ReleaseStringUTFChars((jstring)path,pTmp);
		foundUrls->insert(pair<string, int>(buf, 1));
	}

	map<jint, jobject>::const_iterator it_map;
	vector<jobject>* loadersTmp = new vector<jobject>;
	for(it_map = loaders->begin(); it_map != loaders->end(); it_map++)
	{
		loadersTmp->push_back(it_map->second);
	}
    vector<jobject>::iterator it;
	for(it = loadersTmp->begin(); it != loadersTmp->end(); it++)
	{
		//call Callbacks::getClassObject()
        classobj = jni->CallStaticObjectMethod(callbacksclass,callbackMethod,
			*it,jni->NewStringUTF(className));
		if (classobj != NULL)
		{
			//call Callbacks::getClassUrl()
			path = jni->CallStaticObjectMethod(callbacksclass,getClassUrlMethodId,
												*it,jni->NewStringUTF(className));
			if (path != NULL)
			{
				const char* pTmp = jni->GetStringUTFChars((jstring)path, NULL);
				string buf = pTmp;
				jni->ReleaseStringUTFChars((jstring)path,pTmp);
				map<string, int>::const_iterator it1 = foundUrls->find(buf);
				if (it1 != foundUrls->end())
				{
					continue;
				}
				else
				{
					foundUrls->insert(pair<string, int>(buf, 1));
				}
			}
			if (classVec == NULL)
			{
				classVec = new vector<jclass>();
			}
			classVec->push_back((jclass)classobj);
		}
	}
	loadersTmp->clear();
	delete loadersTmp;
    foundUrls->clear();
	delete foundUrls;
	return classVec;
}


void AgentCallbackHandler::redefineClass(jvmtiEnv *jvmti, JNIEnv* jni, string name, string loadString)
{
	LOG4CXX_DEBUG(Global::logger, "Enter AgentCallbackHandler::redefineClass 4");

	bool compareLoadString = false;
	if (loadString.length() > 0)
		compareLoadString = true;

	map<string, int>* foundUrls = new map<string, int>;

    jobject path = jni->CallStaticObjectMethod(callbacksclass,getClassUrlMethodId,
								NULL,jni->NewStringUTF(name.c_str()));
	if (path != NULL)
	{
		const char* pTmp = jni->GetStringUTFChars((jstring)path, NULL);
		string buf = pTmp;
		jni->ReleaseStringUTFChars((jstring)path,pTmp);

		if (!compareLoadString 
			|| (compareLoadString && (strstr(buf.c_str(), loadString.c_str()) != NULL)))
		{
			vector<MonitoringClass*>* method
				= Global::getConfig()->getMonitorClassInfo(name.c_str(), buf.c_str());
			if (method->size() == 0)
			{
				delete method;
			}
			else if (method->size() >= 1)
			{
				foundUrls->insert(pair<string, int>(buf, 1));
				redefineClass(jvmti, jni, NULL, name.c_str());
				method->clear();
				delete method;
			}
		}
	}

	if (loaders == NULL)
	{
        foundUrls->clear();
		delete foundUrls;
		foundUrls = NULL;
		return;
	}

	map<jint, jobject>::const_iterator it_map;
	vector<jobject>* loadersTmp = new vector<jobject>;
	for(it_map = loaders->begin(); it_map != loaders->end(); it_map++)
	{
		loadersTmp->push_back(it_map->second);
	}
	vector<jobject>::iterator it;
	for(it = loadersTmp->begin(); it != loadersTmp->end(); it++)
	{
		//call Callbacks::getClassUrl()
        jobject path = jni->CallStaticObjectMethod(callbacksclass,getClassUrlMethodId,
									*it,jni->NewStringUTF(name.c_str()));
		if (path != NULL)
		{
			const char* pTmp = jni->GetStringUTFChars((jstring)path, NULL);
			string buf = pTmp;
			jni->ReleaseStringUTFChars((jstring)path,pTmp);
           
			map<string, int>::const_iterator it1 = foundUrls->find(buf);
			if (it1 != foundUrls->end())
			{
				continue;
			}

			if (!compareLoadString 
				|| (compareLoadString && (strstr(buf.c_str(), loadString.c_str()) != NULL)))
			{
				vector<MonitoringClass*>* method
					= Global::getConfig()->getMonitorClassInfo(name.c_str(), buf.c_str());
				if (method->size() ==  0)
				{
					delete method;
				}
				else if (method->size() >= 1)
				{
					foundUrls->insert(pair<string, int>(buf, 1));
					redefineClass(jvmti, jni, *it, name.c_str());
					method->clear();
					delete method;
				}
			}
		}
	}
	loadersTmp->clear();
	delete loadersTmp;
    foundUrls->clear();
	delete foundUrls;
	LOG4CXX_DEBUG(Global::logger, "Exit AgentCallbackHandler::redefineClass 4");
	return;
}

void AgentCallbackHandler::redefineClass(jvmtiEnv *jvmti,JNIEnv* jni, jobject cl, const char* name)
{
	LOG4CXX_DEBUG(Global::logger, "Enter AgentCallbackHandler::redefineClass 3");
	string tempString = name;
	if(Global::getIsIBMJvm())
	{   //ibm jvmの場合、javaから始まるクラスを動的定義できない
		if(tempString.find("java.",0) == 0)
		{
			return;
		}
	}

	jmethodID callbackMethod = jni->GetStaticMethodID(callbacksclass,"getClassData",
		"(Ljava/lang/ClassLoader;Ljava/lang/String;)I");
	if(callbackMethod == NULL)
	{
		LOG4CXX_FATAL(Global::logger, "Cann't find method Callbacks::getClassData");
		return;
	}

    jint classDataLen = jni->CallStaticIntMethod(callbacksclass,callbackMethod,
												cl,jni->NewStringUTF(name));
	if (classDataLen == 0)
	{
		Logger::getLogger()->logError("Cann't get class file data : %s", name);
		return;
	}

    jfieldID fieldID = jni->GetStaticFieldID(callbacksclass, "classData", "[B");
	jobject data = jni->GetStaticObjectField(callbacksclass, fieldID);
	AUTO_REL_JNI_LOCAL_REF(jni, data);
	jbyte* elems = jni->GetByteArrayElements((jbyteArray)data, 0);

    unsigned char*  classData = new unsigned char[classDataLen+1];
	memset(classData, 0, classDataLen+1);
	memcpy(classData, elems, classDataLen);
	jni->ReleaseByteArrayElements((jbyteArray)data, elems, 0);

     //begin of add
    //get class name for the data of class file
    ClassFile classFile(classDataLen, classData);
    if (false == classFile.isClassNameEqual(name))
    {
            Logger::getLogger()->logError("Cann't load class : %s", name);
            delete[] classData;
            return;
    }
    //end of add
	callbackMethod = jni->GetStaticMethodID(callbacksclass,"getClassObject",
		"(Ljava/lang/ClassLoader;Ljava/lang/String;)Ljava/lang/Class;");
	if(callbackMethod == NULL)
	{
		LOG4CXX_FATAL(Global::logger, "Cann't find method Callbacks::getClassObject");
        //begin of add for memory leak
        delete[] classData;
         //end of add
		return;
	}

    jobject modifyClass = jni->CallStaticObjectMethod(callbacksclass,callbackMethod,
								            cl,jni->NewStringUTF(name));
	if (modifyClass == NULL)
	{
		Logger::getLogger()->logError("Cann't get class object for  : %s", name);
        //begin of add for memory leak
        delete[] classData;
        //end of add      
		return;
	}

	jvmtiClassDefinition clsd;
	clsd.klass = (jclass)modifyClass;
	clsd.class_byte_count = classDataLen;
	clsd.class_bytes = classData;

	jvmtiError err;
	err = Global::jvmti->RedefineClasses(1,&clsd);
	if(err != JVMTI_ERROR_NONE)
	{
		char buf[MAX_BUF_LEN];
		sprintf(buf,"Failed to redefine class,errocd =%d",err);
		LOG4CXX_ERROR(Global::logger, buf);
	}

	LOG4CXX_DEBUG(Global::logger, "Exit AgentCallbackHandler::redefineClass 3");

	return;
}

void AgentCallbackHandler::redefineClasses(jvmtiEnv *jvmti,JNIEnv* jni, MonitoringClass* oldDef)
{
	LOG4CXX_DEBUG(Global::logger, "Enter AgentCallbackHandler::redefineClasses 2");

    if (oldDef == NULL)
		return;

    //loadedClassInfoからロードされたクラスの情報を削除する。
	string key = oldDef->getClassName();
	key.append("#");
	key.append(oldDef->getClassLoadString());
    map<string, MonitoringClassLoadInfo*>::const_iterator it;
	if (loadedClassInfo == NULL)
	{
		return;
	}
	it = loadedClassInfo->find(key);
	if (it == loadedClassInfo->end())
		return;

    MonitoringClassLoadInfo* classLoadInfo = it->second;
	loadedClassInfo->erase(key);

	if(methodDumpInfo != NULL)
	{
		vector<string>* urls = classLoadInfo->getClassUrls();
		if (urls != NULL)
		{
			vector<string>::iterator it1;
			for (it1 = urls->begin(); it1 != urls->end(); it1++)
			{
				map<string, MonitoringMethodInfo*>::const_iterator it2;
				it2 = methodDumpInfo->find(*it1);
				if (it2 != methodDumpInfo->end())
				{
					MonitoringMethodInfo* p = it2->second;
					if (strcmp(p->classLoadString.c_str(), oldDef->getClassLoadString()) == 0)
					{
						methodDumpInfo->erase(*it1);
						delete p;
					}
				}
			}
		}

	}

	vector<jobject>* loader = classLoadInfo->getClassLoaders();
	if (loader != NULL)
	{
		vector<jobject>::iterator it1;
		for (it1 = loader->begin(); it1 != loader->end(); it1++)
		{
			redefineClass(jvmti, jni, *it1, oldDef->getClassName());
		}
	}
	delete classLoadInfo;

	LOG4CXX_DEBUG(Global::logger, "Exit AgentCallbackHandler::redefineClasses 2");
}

void AgentCallbackHandler::redefineClassesNew(jvmtiEnv *jvmti,JNIEnv* jni, MonitoringClass* newDef)
{
	LOG4CXX_DEBUG(Global::logger, "Enter AgentCallbackHandler::redefineClassesNew");

    if (newDef == NULL)
		return;

	string className = newDef->getClassName();
	string classLoadString = newDef->getClassLoadString();

    redefineClass(jvmti, jni, className, classLoadString);

	LOG4CXX_DEBUG(Global::logger, "Exit AgentCallbackHandler::redefineClassesNew");
}

void AgentCallbackHandler::redefineClasses(jvmtiEnv *jvmti,JNIEnv* jni, vector<MonitoringClass*>* oldDef)
{
	LOG4CXX_DEBUG(Global::logger, "Enter AgentCallbackHandler::redefineClasses 1");

	//lock will auto release at the end of this method.
	//AutoMonitor Monitoring(Global::configMonitor);

	vector<MonitoringClass*>* newDef = Global::getConfig()->getMonitorMethodDefine();

	vector<MonitoringClass*>::iterator it1;
	vector<MonitoringClass*>::iterator it2;

    for (it1 = oldDef->begin();it1 != oldDef->end();it1++)
    {
		char* classNameOld = (*it1)->getClassName();
		char* classSigOld = (*it1)->getClassLoadString();

		bool found = false;
	    for (it2 = newDef->begin();it2 != newDef->end();it2++)
	    {
			char* classNameNew = (*it2)->getClassName();
			char* classSigNew = (*it2)->getClassLoadString();
			int ret = strcmp(classNameOld, classNameNew);
			if (ret > 0)
			{
				continue;
			}
			else if (ret < 0)
			{
				break;
			}

			ret = strcmp(classSigOld, classSigNew);
			if (ret > 0)
			{
				continue;
			}
			else if (ret < 0)
			{
				break;
			}
			found = true;
			break;
	    }
	    if (!found)  //not exist in new config
    	{
			if (((*it1)->nMonitoringType == 1)
				|| ((*it1)->nMonitoringType == 3))
			{
				removeInstance(jni, classNameOld, classSigOld);
			}
			redefineClasses(jvmti, jni, *it1);
			continue;
    	}
	}

    for (it2 = newDef->begin();it2 != newDef->end();it2++)
    {
		char* classNameNew = (*it2)->getClassName();
		char* classSigNew = (*it2)->getClassLoadString();

		bool found = false;
	    for (it1 = oldDef->begin();it1 != oldDef->end();it1++)
	    {
			char* classNameOld = (*it1)->getClassName();
			char* classSigOld = (*it1)->getClassLoadString();
			int ret = strcmp(classNameNew, classNameOld);
			if (ret > 0)
			{
				continue;
			}
			else if (ret < 0)
			{
				break;
			}

			ret = strcmp(classSigNew, classSigOld);
			if (ret > 0)
			{
				continue;
			}
			else if (ret < 0)
			{
				break;
			}
			found = true;
			break;
	    }	    

	    if (!found)
    	{
			redefineClassesNew(jvmti, jni, *it2);
			continue;
    	}

		bool bRet = (*it2)->needRedefine(**it1);
		if (bRet)
		{
			if (loadedClassInfo == NULL)
			{
				redefineClassesNew(jvmti, jni, *it2);
				continue;
			}
	
			string key = classNameNew;
			key.append("#");
			key.append(classSigNew);
			map<string, MonitoringClassLoadInfo*>::const_iterator it;
			it = loadedClassInfo->find(key);
			if (it == loadedClassInfo->end())
			{
				redefineClassesNew(jvmti, jni, *it2);
				continue;
			}
	
			if ((((*it1)->nMonitoringType == 1)
				|| ((*it1)->nMonitoringType == 3))
				&& ((*it2)->nMonitoringType == 2))
			{
				removeInstance(jni, classNameNew, classSigNew);
			}
			redefineClasses(jvmti, jni, *it1);
			//redefineClassesNew(jvmti, jni, *it2);
		}
		else if ((*it2)->getMonitoringType() != 1)
		{
			string key = classNameNew;
			key.append("#");
			key.append(classSigNew);

			map<string, MonitoringClassLoadInfo*>::const_iterator it;
			if (loadedClassInfo != NULL)
			{
				it = loadedClassInfo->find(key);
				if (it != loadedClassInfo->end())
				{
					vector<string>* urls = it->second->getClassUrls();
					if (urls != NULL)
					{
						vector<string>::iterator vit;
						for (vit = urls->begin(); vit != urls->end(); ++vit)
						{
							addMethodDumpInfo((MonitoringMethod*)(*it2), *vit);
						}
					}
				}
			}
		}
    }
    LOG4CXX_DEBUG(Global::logger, "Exit AgentCallbackHandler::redefineClasses 1");
}

void AgentCallbackHandler::threadEnd(jvmtiEnv *jvmti, JNIEnv *jni, jthread thread) 
{
	//this monitor will be auto released at the end of this method
	AutoMonitor Monitoring(Global::configMonitor);
	
	if (Global::stackTraceCache == NULL) {
		return;
	}

	deque<SimpleStackTrace*>::iterator it = Global::stackTraceCache->begin();
	while (it != Global::stackTraceCache->end()) 
	{
		SimpleStackTrace *stackTrace = *it;
		if (jni->IsSameObject(stackTrace->thread, thread))
		{
			delete *it;
			it = Global::stackTraceCache->erase(it);
		}
		else
		{
			it++;
		}
	}
}

MethodDumpInfo* AgentCallbackHandler::getMethodDumpInfo(string classUrl, char* methodName, char* methodSig)
{
	if(methodDumpInfo == NULL)
	{
		return NULL;
	}
    map<string, MonitoringMethodInfo*>::const_iterator it;
    it = methodDumpInfo->find(classUrl);
	if (it == methodDumpInfo->end())
		return NULL;

    MonitoringMethodInfo* pInfo = it->second;
    MethodDumpInfo* pRet = pInfo->getMethodDumpInfo(methodName, methodSig);
	if ((pRet == NULL) && strlen(methodSig) != 0)
	{
	    pRet = pInfo->getMethodDumpInfo(methodName, "");
	}
	return pRet;
}

/*bool AgentCallbackHandler::haveRedefined(string& classUrl, const char* classLoadString)
{
	if (methodDumpInfo == NULL)
	{
		return false;
	}

    map<string, MonitoringMethodInfo*>::const_iterator it;
    it = methodDumpInfo->find(classUrl);
	if (it == methodDumpInfo->end())
	{
		return false;
	}

    MonitoringMethodInfo* pInfo = it->second;
	if (strcmp(pInfo->classLoadString.c_str(), classLoadString) == 0)
	{
		return true;
	}

	return false;
}*/

//AgentCallbackHandler::instanceMapから指定クラス名＋クラスロード
void AgentCallbackHandler::removeInstance(JNIEnv* jni, char* className, char* classLoadString)
{
	if (instanceMap == NULL)
	{
		//no instance registed
        return;
	}

	string prefix=className;
	prefix.append("#");

	map<string, vector<jweak>*>::const_iterator it;
	vector<jweak>* value = NULL;
	for (it = instanceMap->begin(); it != instanceMap->end(); it++)
	{
		string key = it->first;
		if (strncmp(key.c_str(), prefix.c_str(), prefix.length()) != 0)
			continue;

		if (strlen(classLoadString) != 0)
		{
			const char* url = key.c_str() + prefix.length();
			if (strstr(url, classLoadString) == NULL)
				continue;
		}
		value = it->second;
		instanceMap->erase(key);
		break;
	}

	if (value != NULL)
	{
		vector<jweak>::iterator it1 = value->begin();
		while (it1 != value->end())
		{
			jni->DeleteWeakGlobalRef(*it1);
			it1++;
		}
		value->clear();
		delete value;
		value = NULL;
	}
}

bool AgentCallbackHandler::isClassLoaderClass(char* className)
{
	if(className == NULL)
		return false;

	if(strcmp(className,"Ljava/lang/ClassLoader;")==0)
		return true;

	if (loadersSig != NULL)
	{
		map<string, short>::const_iterator p;
		p = loadersSig->find(className);
		if(p != loadersSig->end())
		{
			return true;
		}
	}
	return false;;
}

//add by Qiu Song on 20090811 for 重複ダンプ防止
bool AgentCallbackHandler::isSameExceptionObject(JNIEnv* jni, jthread thread, jobject exception)
{
	//既にダンプしたかどうか判断する
	if(AgentCallbackHandler::dumpedExceptionObjList != NULL)
	{
		map<jthread, jobject>::const_iterator it;
		for (it = AgentCallbackHandler::dumpedExceptionObjList->begin(); it != AgentCallbackHandler::dumpedExceptionObjList->end(); it++)
		{
			if( jni->IsSameObject(exception, it->second) && jni->IsSameObject(thread, it->first))
			{
				return true;
			}
		}
	}
	
	/*if(lastException != NULL && jni->IsSameObject(lastException,exception)
		&& jni->IsSameObject(lastThread,thread))
	{
		return true;
	}
	else*/
	{
		//例外オブジェクトとスレッドオブジェクトを保存する
		if(lastThread != NULL)
		{
            jni->DeleteGlobalRef(lastException);
		}
		lastException = jni->NewGlobalRef(exception);
		if(lastThread != NULL)
		{
			jni->DeleteGlobalRef(lastThread);
		}
		lastThread = jni->NewGlobalRef(thread);

		if(dumpedExceptionObjList == NULL)
		{
			dumpedExceptionObjList = new map<jthread, jobject>;
		}
		dumpedExceptionObjList->insert(pair<jthread,jobject>(lastThread, lastException));
	}
	return false;
}

//除外例外一覧を取得する
void AgentCallbackHandler::getExcludeExceptionList()
{
	return;//TODO(need delete here)
	char *filePath = HelperFunc::getExcludeExceptionListFilePath();
	if(filePath == NULL || getExcludeExceptionListFromFile(filePath) == false)
	{
		 LOG4CXX_DEBUG(Global::logger, "除外例外一覧ファイルの読み込むが失敗しました.");
	}
}

//指定ファイルから内容を呼び出し、文字列に保存する
bool AgentCallbackHandler::getExcludeExceptionListFromFile(char* fileName)
{
	//指定ファイルをオープンする
	FILE* srcFile = fopen(fileName, "r");
	if(srcFile == NULL)
	{
		Global::logger->logError("指定されたファイル%sオープン失敗しました。", fileName);
		return false;
	}

	//ファイルを読み込む
	char* buf = new char[1024+1];
	while (!feof(srcFile))
	{
		fgets(buf, 1025, srcFile);
		if (strlen(buf) > 1)
		{
			//最後の「\n」を削除する
			buf[strlen(buf) - 1] = 0;
			string strException = buf;
			if( AgentCallbackHandler::excludeExceptionList == NULL)
			{
				 AgentCallbackHandler::excludeExceptionList = new vector<string>;
			}
			AgentCallbackHandler::excludeExceptionList->push_back(strException);
		}
	}

	fclose(srcFile);
	delete[] buf;
	return true;
}

//除外例外クラスかどうか判断する
bool AgentCallbackHandler::isExcludeException(char* exceptionClassName)
{
    //TODO(抽出する)
 
	    //   java.lang.NoSuchMethodException
	    //   java.lang.NoSuchFieldException
	    //   javax.naming.NamingException
	    //   java.net.MalformedURLException 
	    //   com.sun.org.apache.xerces.internal.xni.parser.XMLConfigurationException
	if(strcmp(exceptionClassName, "java.lang.ClassNotFoundException") == 0 ||
	   strcmp(exceptionClassName, "java.lang.NoSuchMethodException") == 0 ||
	   strcmp(exceptionClassName, "java.lang.NoSuchFieldException") == 0 ||
	   strcmp(exceptionClassName, "java.lang.NamingException") == 0 ||
	   strcmp(exceptionClassName, "java.lang.MalformedURLException") == 0 ||
	   strcmp(exceptionClassName, "com.sun.org.apache.xerces.internal.xni.parser.XMLConfigurationException") == 0 )
	{
		return true;
	}
	else
	{
		return false;
	}
    //TODO
	int nCount = AgentCallbackHandler::excludeExceptionList->size();
	for(int i=0; i < nCount; i++)
	{
		string excludeException = AgentCallbackHandler::excludeExceptionList->at(i);
		if(strcmp(exceptionClassName, excludeException.c_str()) == 0)
		{
			return true;
		}
	}
	return false;
}
//end of add by Qiu Song on 20090811 for 重複ダンプ防止