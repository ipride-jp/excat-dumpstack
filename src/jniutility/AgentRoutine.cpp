#include <time.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jni.h>
#include <jvmti.h>
#include <sys/timeb.h>

#include "Monitor.h"
#include "../common/Define.h"
#include "../common/JniLocalRefAutoRelease.h"
#include "../common/JvmtiAutoRelease.h"
#include "../common/Global.h"
#include "../common/HelperFunc.h"
#include "AgentCallbackHandler.h"
#include "../common/Define.h"
#include "../common/ReadCfgFileException.h"
#include "../output/StackTrace.h"
#include "xercesc/framework/LocalFileFormatTarget.hpp"
#include "license/license.h"
#include "../common/SystemInfo.h"
#include "../common/ObjectAutoRelease.h"
#include "../common/JvmtiAutoRelease.h"
#include "../common/SimpleStackTrace.h"


using namespace std;
USE_NS(NS_JNI_UTILITY)
USE_NS(NS_COMMON)
USE_NS(NS_OUTPUT)

#include "AgentRoutine.h"

#define TAG_DUMP	"Dump"
#define MAGIC_FILE  "magic"
//modified by Qiu Song on 20090810 for Version 3.0
#define VERSION_INFO "DumpStack Version 3.0.0 for Java 5.0"
#define VERSION_INFO_TRIAL   "DumpStack Version 3.0.0 for Java 5.0(Trial Version)"
//end of modified by Qiu Song on 20090810 for Version 3.0

string dumpStackForException(StackTraceParam* sparam,JNIEnv *jni);
string dumpStackForMethod(StackTraceParam* sparam,JNIEnv *jni);
void dumpStackForSignal(StackTraceParam* sparam,JNIEnv *jni);
char* makeDumpDir(char* middleDirName1,char* middleDirName2);
//modified by Qiu Song on 20090908 for バグ：ファイル名と内容に表示される時間は一致しない
void appendDumpFileName(string& fileNameBuf, DumpFileTime* nowtime,JNIEnv *jni);
//end of modified by Qiu Song on 20090908 for バグ：ファイル名と内容に表示される時間は一致しない
bool parseAgentOptions(char* options, char** jvmid, 
					   char** configPath, string& errMsg);
char* getOptionValue(char** pStart);
bool isAlphaOrNumber(const char* pInput);

/**
 * Jvm起動時に呼び出される。
 * @param vm JVM
 * @param options コマンドラインのオプション
 * @param reserved 保留
 * @return エラーを示す。
 *			<br>値が0である場合、成功
 *		    <br>値が0でない場合、エラー発生
 */
JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) 
{

    jint rc;
    jvmtiError err;
    jvmtiEventCallbacks callbacks;
	jvmtiCapabilities capa;
	jvmtiEnv *jvmti;
    bool errFlag = false;
    string logBuf;
    bool ret = false;

	//initial log instance
	Global::logger = Logger::getLogger();

	//check enviroment variable:EXCAT_HOME
	char *installPath = getenv(EXCAT_HOME); 
#if defined(DEFAULT_EXCAT_HOME)
	if (installPath == NULL) {
	  installPath = DEFAULT_EXCAT_HOME;
	}
#endif /* defined(DEFAULT_EXCAT_HOME) */
	Global::logger->setPath(installPath);
    if(installPath == NULL)
	{
		LOG4CXX_FATAL(Global::logger, "Environment variable EXCAT_HOME is not setup." );
		errFlag = true;
	}

	//Check license
	if(!errFlag)
	{
        ret = checkLicenseFile(installPath);
		if(!ret)
		{
			errFlag = true;
		}
	}

	//read config file
	if(!errFlag)
	{
		ret = readConfigFile(options,installPath);
		if(!ret)
		{
			errFlag = true;
		}
	}

	if(errFlag)
	{
		//エラーがあ場合、Ccatを起動しない
        return 0;
	}


	//ダンプファイルを暗号化するかどうかをチェック
	checkMagicfile(installPath);

	//Get jvmti environment
	rc = vm->GetEnv((void **)&jvmti, JVMTI_VERSION);
	if (rc != JNI_OK) 
	{
	    Global::logger->logFatal("can't get jvmti enviroment,error code=%d.",rc);
		return 0;
	}

	Global::jvmti = jvmti;
    Global::savedvm = vm;
	//add bootclasspath
    ret = addBootClassPath(jvmti,installPath);
	if(!ret){
		return 0;
	}

    err = jvmti->CreateRawMonitor("configMonitor", &(Global::configMonitor));
	if (err != JVMTI_ERROR_NONE)
	{
		Global::logger->logFatal("failed to call CreateRawMonitor,error code=%d",err);
		return 0;
	}

	//Set jvmti capabilities:
	memset(&capa, 0, sizeof(jvmtiCapabilities));
	capa.can_generate_all_class_hook_events = 1;
	capa.can_generate_exception_events = 1; 
	capa.can_redefine_classes = 1;
	capa.can_get_line_numbers = 1;
	capa.can_get_bytecodes = 1;
	capa.can_tag_objects = 1;//add tag
    capa.can_get_source_file_name = 1;
	capa.can_suspend = 1;
	capa.can_access_local_variables = 1;
	
	//add by Qiu Song on 20090811 for Excat Version3.0
	capa.can_get_thread_cpu_time = 1;//CPU時間の取得
	capa.can_get_current_contended_monitor = 1;//モニターオブジェクトの取得
	capa.can_get_owned_monitor_info = 1;//所有モニター情報の取得
	//end of add by Qiu Song on 20090811 for Excat Version3.0
	
	err = Global::jvmti->AddCapabilities(&capa);
	if (err != JVMTI_ERROR_NONE)
	{
		Global::logger->logFatal("failed to call AddCapabilities,error code=%d",err);
		return 0;
	}

	//Enable VMInit event callback
	err = jvmti->SetEventNotificationMode(JVMTI_ENABLE, 
		JVMTI_EVENT_VM_INIT, NULL);
	if (err != JVMTI_ERROR_NONE)
	{
		Global::logger->logFatal("can't enable VMInit envent notification,error code=%d.",err);
		return 0;
	}		

	//Enable VMDeath event callback
	err = jvmti->SetEventNotificationMode(JVMTI_ENABLE, 
		JVMTI_EVENT_VM_DEATH, NULL);
	if (err != JVMTI_ERROR_NONE)
	{
		Global::logger->logFatal("can't enable VMDeath envent notification,error code=%d.",err);
		return 0;
	}		

	//Enable class file load event notification
	err = jvmti->SetEventNotificationMode(JVMTI_ENABLE, 
		JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, NULL);
	if (err != JVMTI_ERROR_NONE)
	{
		Global::logger->logFatal("can't enable class load event notification,error code=%d.",err);
		return 0;
	}		

	//Enable Exception event callback
	err = jvmti->SetEventNotificationMode(JVMTI_ENABLE, 
		JVMTI_EVENT_EXCEPTION, NULL);
	if (err != JVMTI_ERROR_NONE)
	{
		Global::logger->logFatal("can't enable Exception envent notification,error code=%d.",err);
		return 0;
	}		

	//Enable DataDumpRequest event callback
	err = jvmti->SetEventNotificationMode(JVMTI_ENABLE, 
		JVMTI_EVENT_DATA_DUMP_REQUEST, NULL);
	if (err != JVMTI_ERROR_NONE)
	{
		Global::logger->logFatal("can't enable DataDumpRequest envent notification,error code=%d.",err);
		return 0;
	}

	//Enable ThreadEnd callback
	err = jvmti->SetEventNotificationMode(JVMTI_ENABLE, 
		JVMTI_EVENT_THREAD_END, NULL);
	if (err != JVMTI_ERROR_NONE)
	{
		Global::logger->logFatal("can't enable Thread event notification,error code=%d.",err);
		return 0;
	}

	//Add ClassFileHook callback  and VMInit event callback
	memset(&callbacks, 0, sizeof(callbacks));
	callbacks.VMInit = &AgentCallbackHandler::vmInitCallBack;
	callbacks.VMDeath = &AgentCallbackHandler::vmDeathCallBack;
	callbacks.DataDumpRequest = &AgentCallbackHandler::dataDumpRequest;
	callbacks.ClassFileLoadHook = &AgentCallbackHandler::classFileLoadHook;
	callbacks.Exception = &AgentCallbackHandler::exceptionEvent;
	callbacks.ThreadEnd = &AgentCallbackHandler::threadEnd;
	err = jvmti->SetEventCallbacks(&callbacks, sizeof(callbacks));
	if (err != JVMTI_ERROR_NONE)
	{
        Global::logger->logFatal("can't add class file load or vminit hook,error code=%d.",err);
		return 0;
	}

	LOG4CXX_INFO(Global::logger,"Excat agent is activated.");
	return 0;
}

/**
設定ファイルの読み込み
char* options agentのパラメータ
char* installPath ccatがインストールされたパス
**/
bool readConfigFile(char* options,char* installPath)
{
	Config* configDump = NULL;
	string logBuf;
	try
	{
        char* jvmid = NULL;
		char* pConfigFilePath = NULL;
		if(options != NULL && strlen(options) > 0)
        {
			//get config file name from agent_lib's optiones
			string errMsg = "";
			bool  bRet = parseAgentOptions(options, &jvmid, &pConfigFilePath, errMsg);
			if (!bRet)
			{
				if (jvmid != NULL)
				{
					delete[] jvmid;
					jvmid = NULL;
				}
				if (pConfigFilePath != NULL)
				{
					delete[] pConfigFilePath;
					pConfigFilePath = NULL;
				}
				LOG4CXX_FATAL(Global::logger, errMsg.c_str());
				return false;
			}
		}
		SystemInfo* systemInfo = SystemInfo::getInstance();
		if (jvmid != NULL){
			systemInfo->setJvmID(jvmid);
		}else{
			systemInfo->setJvmID("1");
		}

        //we first try to find the config file from enviroment variable:EXCAT_HOME        
        char* configFileName = CONFIG_FILE_NAME;
		string configFilePath;
		if (pConfigFilePath == NULL)
		{
			 int len = strlen(installPath);
			 configFilePath = installPath;
			 if(installPath[len -1] != FILE_SPERATOR_C)
			 {
				 configFilePath += FILE_SPERATOR;
			 }
			 configFilePath += DUMPSTACK_DIR;
			 configFilePath += FILE_SPERATOR;
		}
		else
		{
			configFilePath = pConfigFilePath;
			configFilePath += FILE_SPERATOR;
			delete[] pConfigFilePath;
		}
		configFilePath += configFileName;
	    if(!doesFileExist(configFilePath.c_str()))
		{
			 logBuf = "Config file ";
			 logBuf += configFilePath;
			 logBuf += " doesn't exist";
			 LOG4CXX_FATAL(Global::logger, logBuf.c_str());
			 return false;
		 }
		 configDump = new Config();
		 configDump->init((char*)configFilePath.c_str());
		 Global::setConfig(configDump);
		 //to write into log file
		 if(Global::isTrial){
			  LOG4CXX_INFO(Global::logger, VERSION_INFO_TRIAL);
		 }else{
			  LOG4CXX_INFO(Global::logger, VERSION_INFO);
		 }
		 
		 configDump->logConfig();

	}catch(ReadCfgFileException& e )
	{
		if(configDump != NULL)
		{
			delete configDump;
            configDump = NULL;
		}
		LOG4CXX_FATAL(Global::logger, e.getErrorMsg());
		return false;
    }

	return true;
}

//add jar file(dump.jar,mail.jar,activation.jar) to bootclasspath
bool addBootClassPath(jvmtiEnv* jvmti,char* installPath)
{
	string jarFilePath1;
	string jarFilePath2;
	string jarFilePath3;
	string logBuf;
    bool hasError = false;
    jvmtiError err;

	int len = strlen(installPath);
	jarFilePath1 = installPath;
	if(installPath[len -1] != FILE_SPERATOR_C)
	{
		jarFilePath1 += FILE_SPERATOR;

	}
    jarFilePath1 += DUMPSTACK_DIR;
    jarFilePath1 += FILE_SPERATOR;
    jarFilePath2 = jarFilePath1;
    jarFilePath3 = jarFilePath1;
    jarFilePath1 += "dump.jar";
    jarFilePath2 += "mail.jar";
	jarFilePath3 += "activation.jar";
	
	if(!doesFileExist(jarFilePath1.c_str())){
		Global::logger->logFatal("file %s doesn't exist:",jarFilePath1.c_str());
		return false;
	}
	if(!doesFileExist(jarFilePath2.c_str())){
		Global::logger->logFatal("file %s doesn't exist:",jarFilePath2.c_str());
		return false;
	}
	if(!doesFileExist(jarFilePath3.c_str())){
		Global::logger->logFatal("file %s doesn't exist:",jarFilePath3.c_str());
		return false;
	}

	err = jvmti->AddToBootstrapClassLoaderSearch(jarFilePath1.c_str());
	if (err != JVMTI_ERROR_NONE)
	{
		Global::logger->logFatal("failed to call bootclasspath %s,error code=%d",
			jarFilePath1.c_str(),err);
		return false;
	}
	err = jvmti->AddToBootstrapClassLoaderSearch(jarFilePath2.c_str());
	if (err != JVMTI_ERROR_NONE)
	{
		Global::logger->logFatal("failed to call bootclasspath %s,error code=%d",
			jarFilePath2.c_str(),err);
		return false;
	}
	err = jvmti->AddToBootstrapClassLoaderSearch(jarFilePath3.c_str());
	if (err != JVMTI_ERROR_NONE)
	{
		Global::logger->logFatal("failed to call bootclasspath %s,error code=%d",
			jarFilePath3.c_str(),err);
		return false;
	}

	return true;
}

/**
ライセンスファイルのチェック
char* installPath ccatがインストールされたパス
2008/6/16 仕様変更
ライセンスファイルが存在しない、或いは無効である場合、試用版として扱う
*/
bool checkLicenseFile(char* installPath)
{
	string licFilePath;
	string logBuf;
    bool hasError = false;

	int len = strlen(installPath);
	licFilePath = installPath;
	if(installPath[len -1] != FILE_SPERATOR_C)
	{
		licFilePath += FILE_SPERATOR;

	}
    licFilePath += DUMPSTACK_DIR;
    licFilePath += FILE_SPERATOR;
    licFilePath += CERT_FILE;
	
	if(!doesFileExist(licFilePath.c_str()))
	{
		logBuf = "License file ";
        logBuf += licFilePath;
        logBuf += " doesn't exist.";
		LOG4CXX_FATAL(Global::logger,logBuf.c_str());
		//add by Qiu Song on 20090929 for ライセンス無効の場合、Excat起動不可
		return false;
		//end of add by Qiu Song on 20090929 for ライセンス無効の場合、Excat起動不可
	}

	if (!hasError && !license_is_valid_file(licFilePath.c_str(), EXCATVERSION))
	{
		logBuf = "License file ";
        logBuf += licFilePath;
        logBuf += " is invalid.";
		LOG4CXX_FATAL(Global::logger,logBuf.c_str());
		//add by Qiu Song on 20090929 for ライセンス無効の場合、Excat起動不可
		return false;
		//end of add by Qiu Song on 20090929 for ライセンス無効の場合、Excat起動不可
	}
	
	//get license key (16bytes)for encode
	unsigned char defaultKey[ENCODE_KEY_NUMBER + 1] ={0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,
	   0x9,0xA,0xB,0xC,0xD,0xE,0xF,0};
    char* tempKey = NULL;
    //add by Qiu Song on 20090929 for MACアドレスのチェック
	char* macAddrKey = NULL;
	macAddrKey = license_get_mac_address(licFilePath.c_str());
	if(macAddrKey != NULL && strlen(macAddrKey) != 0)
	{
		if(HelperFunc::CheckMacAddress(macAddrKey) == true)
		{
			Global::logger->logInfo("Check MAC address %s", macAddrKey);
		}
		else
		{
			LOG4CXX_FATAL(Global::logger, "invalid MAC address!");
			delete macAddrKey;
			macAddrKey = NULL;
			return false;
		}
	}
	if(macAddrKey != NULL && strlen(macAddrKey) != 0)
	{
		delete macAddrKey;
		macAddrKey = NULL;
	}
    //end of add by Qiu Song on 20090929 for MACアドレスのチェック
	if(Global::isTrial){
        tempKey = (char*)defaultKey;
	}else{
        tempKey = license_get_public_key(licFilePath.c_str());
		if(tempKey == NULL )
		{
			LOG4CXX_FATAL(Global::logger, "can't get public key from license file");
			//add by Qiu Song on 20090929 for ライセンス無効の場合、Excat起動不可
			return false;
			//end of add by Qiu Song on 20090929 for ライセンス無効の場合、Excat起動不可
		}
	}
    //暗号キーの設定
	Global::setEncodeKey((unsigned char*)tempKey,16); 
	if(!hasError){
		Global::isTrial = false;
	}
	
	return true;
}

//Check if need to encode the dump file
void checkMagicfile(char* installPath)
{
	string magicFilePath;
    int len = strlen(installPath);
	magicFilePath = installPath;
	if(installPath[len -1] != FILE_SPERATOR_C)
	{
        magicFilePath += FILE_SPERATOR;
	}

	magicFilePath += DUMPSTACK_DIR;
    magicFilePath += FILE_SPERATOR;
    magicFilePath += MAGIC_FILE;

	if(!doesFileExist(magicFilePath.c_str()))
	{
		Global::setNeedEndode(true); 
	}else
	{
        Global::setNeedEndode(false); 
	}
}

/**
 * Jvm終了時に呼び出される。
 * @param vm JVM
 */
JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *vm) 
{	
	Global::isJVMToUnload = 1;
	if (Global::monitorThreadStatus != -1)
	{
		while (Global::monitorThreadStatus != 0)
		{
			HelperFunc::mySleep(1);
		}
	}
	if (Global::dumpFileDelThreadstatus != -1)
	{
		while (Global::dumpFileDelThreadstatus != 0)
		{
			HelperFunc::mySleep(1);
		}
	}

	if (Global::getConfig() != NULL)
	{
		delete Global::getConfig();
		Global::setConfig(NULL);
	}

	if (Global::jvmti != NULL)
	{
		if (Global::configMonitor != NULL)
		{
			Global::jvmti->DestroyRawMonitor(Global::configMonitor);
			Global::configMonitor = NULL;
		}
		Global::jvmti->DisposeEnvironment();
	}

	if(Global::logger != NULL)
	{
       delete Global::logger;
       Global::logger = NULL;
	}

	if (Global::stackTraceCache != NULL) 
	{
		deque<SimpleStackTrace*>::iterator iter = Global::stackTraceCache->begin();
		while (iter != Global::stackTraceCache->end()) 
		{
			delete *iter;
			iter++;
		}

		Global::stackTraceCache->clear();
		delete Global::stackTraceCache;
		Global::stackTraceCache = NULL;
	}

	SystemInfo::deleteInstance();
}

JNIEXPORT void JNICALL Java_Callbacks_errorLog (JNIEnv *jni, jclass jclazz, jstring errmsg)
{
	char* localMsg = HelperFunc::getJStringValue(jni,errmsg);
	if(localMsg != NULL)
	{
		LOG4CXX_ERROR(Global::logger, localMsg);
		delete[] localMsg;
		localMsg = NULL;
		LOG4CXX_ERROR(Global::logger, "failed to send mail");
	}
	
}

/**
 * 監視対象クラスに挿入されるJavaコード（Callbacks.class）に呼ばれる
 * 関数（callback）
 */
JNIEXPORT jstring JNICALL Java_Callbacks_dumpstack(JNIEnv *jni, jclass jclazz, 
						jobject thread, jclass exceptionCls, jstring exceptionName,jstring monitorClassName,jobject exceptionObj)
{
	//this monitor will be auto released at the end of this method
	AutoMonitor Monitoring(Global::configMonitor);
	if(Global::isErrorStatus())
	{
		LOG4CXX_DEBUG(Global::logger, "Now is in erorr status,no dump");
		return NULL;
	}

	jvmtiEnv *jvmti = Global::jvmti;

	jvmtiPhase phase;
    jvmti->GetPhase(&phase); 
    if(JVMTI_PHASE_LIVE != phase)
	{
        LOG4CXX_DEBUG(Global::logger, "jvm is in not in live phase.");
		return NULL;
	}

	StackTraceParam param;

	param.jni = jni;
	param.jvmti = jvmti;
	param.thread = thread;
	param.dumpType = DUMP_KIND_EXCEPTION;  //for exception
	param.exceptionCls = exceptionCls;
	param.utf8ExceptionName =  jni->GetStringUTFChars(exceptionName, NULL);
	param.exceptionName = HelperFunc::getJStringValue(jni,exceptionName);

	//add by Qiu Song on 20090811 for 例外オブジェクトダンプ
	param.exceptionObj = exceptionObj;
	//param.exceptionObj = AgentCallbackHandler::currentException;
	//end of add by Qiu Song on 20090811 for 例外オブジェクトダンプ
    param.monitorObj = NULL;//add by Qiu Song on 20090819 for モニターオブジェクトダンプ
	param.useMonitorThread = NULL;//add by Qiu Song on 20091006 for 競合しているモニターの所有スレッド取得
    param.packageName = NULL;//add by Qiu Song on 20091009 for バグ：メモリリック

	if (param.exceptionName == NULL)
	{
		if(param.utf8ExceptionName != NULL)
			jni->ReleaseStringUTFChars(exceptionName, param.utf8ExceptionName);
		Global::logger->getLogger()->logError("Cann't dump for exception: %s", param.utf8ExceptionName);
		return NULL;
	}
	param.monitorClassName = HelperFunc::getJStringValue(jni,monitorClassName);
	if (param.monitorClassName == NULL)
	{
		if(param.utf8ExceptionName != NULL)
			jni->ReleaseStringUTFChars(exceptionName, param.utf8ExceptionName);
		delete[] param.exceptionName;
		param.exceptionName = NULL;
		Global::logger->getLogger()->logError("Cann't dump for exception: %s", param.utf8ExceptionName);
		return NULL;
	}

	//important!! to set the current output setting
	Global::getConfig()->setOutputSetting(param.monitorClassName);

	param.dumpInstance = Global::getConfig()->getOutputSetting()->getDumpInstance();
	param.dumpCount = 0;

	string filePath = dumpStackForException(&param,jni);

	if(param.utf8ExceptionName != NULL)
		jni->ReleaseStringUTFChars(exceptionName, param.utf8ExceptionName);
    
	//send mail
	if (!filePath.empty() && param.dumpCount > 0){
	    sendMailForException(jni,param.exceptionName,(char*)filePath.c_str());
	}

	if (param.exceptionName != NULL)
	{
		delete[] param.exceptionName;
		param.exceptionName = NULL;
	}
	if (param.monitorClassName != NULL)
	{
		delete[] param.monitorClassName;
		param.monitorClassName = NULL;
	}

	if(param.useMonitorThread != NULL)
	{
		delete[] param.useMonitorThread;
		param.useMonitorThread = NULL;
	}

    return NULL;
}

bool doesFileExist(const char* filePath)
{
	return HelperFunc::isValidFile(filePath);
}

//起動オプションの分析関数
bool parseAgentOptions(char* options, char** jvmid, 
					   char** configPath, string& errMsg)
{
	bool bRet = false;
	errMsg = "DumpStack library's option's format should be \"jvmid=?,configpath=?\"";

	char* pIndex = options;
	char* pTmp = NULL;
	bool  isJvmId = false;
	bool  isConfigPath = false;
	bool  hasJvmId = false;
	bool  hasConfigPath = false;
	
	while (strlen(pIndex) > 0)
	{
		isJvmId = false;
		isConfigPath = false;

		if (strncmp(pIndex, "jvmid=", strlen("jvmid=")) == 0)
		{
			if (hasJvmId)
				return false;
			else
				hasJvmId = true;

			isJvmId = true;
			pIndex +=strlen("jvmid=");
		}
		else if (strncmp(pIndex, "configpath=", strlen("configpath=")) == 0)
		{
			if (hasConfigPath)
				return false;
			else
				hasConfigPath = true;

			isConfigPath = true;
			pIndex +=strlen("configpath=");
		}
		else
			return false;

		pTmp = getOptionValue(&pIndex);
		if (NULL == pTmp)
		{
			return false;
		}
		
		if (isJvmId)
		{
			//check jvmid
			if ((strlen(pTmp) > 4) || (strlen(pTmp) <= 0))
			{
				errMsg = "Jvmid option is invalid.";
				return false;
			}
			if (isAlphaOrNumber(pTmp) == false)
			{
				errMsg = "Jvmid option is invalid.";
				return false;
			}
			*jvmid = pTmp;
		}
		else
		{
			//check config path
			if (strlen(pTmp) <= 0)
			{
				errMsg = "Configpath option is invalid.";
				return false;
			}
			if (HelperFunc::isValidDir(pTmp) == false)
			{
				errMsg = "Configpath option is invalid. Directory is not exist:";
				errMsg.append(pTmp);
				return false;
			}
			*configPath = pTmp;
		}
	}
	errMsg = "";
	return true;
}

//起動オプションに設定されたデータを取得する関数
char* getOptionValue(char** pOptions)
{
	char* pRet = NULL;
	char* pIndex = NULL;
	char* options = *pOptions;

	if (*options == '"')
	{
		pIndex = strchr(options+1, '"');
		if (pIndex == NULL)
			return NULL;

		char ch = *(pIndex + 1);
		if ((ch != ',') && (ch != 0))
			return NULL;

		pRet = new char[pIndex - options];
		memcpy(pRet, options+1, pIndex - options - 1);
		pRet[pIndex - options - 1] = 0;
		options = pIndex + 1;

		if (ch == ',')
		{
			options++;
		}
	}
	else
	{
		pIndex = strchr(options, ',');
		if (pIndex == NULL)
		{
			pRet = new char[strlen(options) + 1];
			strcpy(pRet, options);
			options += strlen(options);
		}
		else
		{
			pRet = new char[pIndex - options + 1];
			memcpy(pRet, options, pIndex - options);
			pRet[pIndex - options] = 0;
			options = pIndex + 1;
		}
	}
	*pOptions = options;
	return pRet;
}

bool isAlphaOrNumber(const char* pInput)
{
	if (pInput == NULL)
		return false;

	int len = strlen(pInput);
	for(int i = 0;i < len;i++)
	{
		char ch = *(pInput + i);
		if( ('0' <= ch && ch <='9') ||
			('a' <= ch && ch <='z') ||
			('A' <= ch && ch <='Z'))
		{
			;//do nothing
		}else
		{
			return false;
		}
	}
	return true;
}

void  sendMailForException(JNIEnv *jni,char* exceptionName,char* dumpFilePath)
{

	//check if to send mail
    OutputSetting *setting = Global::getConfig()->getOutputSetting();
	map<string, string> *mailSetting = Global::getConfig()->getMailSetting();

    if(setting->getMail() && mailSetting != NULL)
	{
		//if to attach dump file
		bool attachFile = setting->getAttachFile();
		bool hasError = false;
	    jclass classCallbacks = jni->FindClass("Callbacks");
		jmethodID sendMailForException = NULL;
		if(classCallbacks == NULL)
		{
			LOG4CXX_FATAL(Global::logger, "can't get class Callbacks");
			hasError = true;
		}
	
		if(!hasError)
		{
			sendMailForException = jni->GetStaticMethodID(classCallbacks,"sendMailForException",
			"(Ljava/lang/String;Ljava/lang/String;Z)Z");
			if(sendMailForException == NULL)
			{
				hasError = true;
				LOG4CXX_FATAL(Global::logger, "can't get method Callbacks::sendMailForException");
			}
		}
		if(!hasError)
		{
			jstring exampeNameUTF = jni->NewStringUTF(exceptionName); 
			jstring dumpPathUTF = jni->NewStringUTF(dumpFilePath);
            jboolean attachFileBool = JNI_TRUE;
            if(!attachFile)
			{
                attachFileBool = JNI_FALSE;
			}

            jni->CallStaticBooleanMethod(
		        classCallbacks,sendMailForException,
				exampeNameUTF,dumpPathUTF,attachFileBool);
		}

    }

}

/**
 * java.lang.StackOverflowError例外が発生する時に呼び出される。
 * called in AgentCallBack::exceptionEvent()
 * @param jni              JNIEnv
 * @param thread           jthread
 * @param exceptionCls     jclass
 */
void dumpstackStackOverFlowError(JNIEnv *jni, jthread thread, jclass exceptionCls)
{
	if(Global::isErrorStatus())
	{
		LOG4CXX_ERROR(Global::logger, "Now is in erorr status,no dump");
		return;
	}

	jvmtiEnv *jvmti = Global::jvmti;

	jvmtiPhase phase;
    jvmti->GetPhase(&phase); 
    if(JVMTI_PHASE_LIVE != phase)
	{
        LOG4CXX_DEBUG(Global::logger, "jvm is in not in live phase.");
		return;
	}

	AutoMonitor Monitoring(Global::configMonitor);

	//important!! to set the current output setting
	bool bRet = Global::getConfig()->setStackOverflowErrorOutputSetting();
	if (!bRet)
	{
		LOG4CXX_DEBUG(Global::logger, 
			"OutputSetting for java.lang.StackOverflowError is not setted.");
        return;
	}

	StackTraceParam param;
	param.jni = jni;
	param.jvmti = jvmti;
	param.thread = thread;
	param.dumpType = DUMP_KIND_EXCEPTION;  //for exception
	param.exceptionCls = exceptionCls;
	param.utf8ExceptionName = HelperFunc::strdup("java.lang.StackOverflowError");
	param.exceptionName = HelperFunc::strdup("java.lang.StackOverflowError");
	param.monitorClassName = HelperFunc::strdup("java.lang.StackOverflowError");
    param.dumpType = DUMP_KIND_EXCEPTION;

	param.dumpInstance = Global::getConfig()->getOutputSetting()->getDumpInstance();
	param.dumpCount = 0;

	//add by Qiu Song on 20090811 for 例外オブジェクトダンプ
	param.exceptionObj = AgentCallbackHandler::currentException;
    //end of add by Qiu Song on 20090811 for 例外オブジェクトダンプ
	param.monitorObj = NULL;//add by Qiu Song on 20090819 for モニターオブジェクトダンプ
	param.useMonitorThread = NULL;//add by Qiu Song on 20091006 for 競合しているモニターの所有スレッド取得
    param.packageName = NULL;

    string filePath = dumpStackForException(&param,jni);

	/*stackoverflowでは、Javaのメソッドを呼び出せない
	//send mail
	if (!filePath.empty()) 
	    sendMailForException(jni,param.exceptionName,(char*)filePath.c_str());
    */

	if(param.utf8ExceptionName != NULL)
	{
		delete[] (char*)(param.utf8ExceptionName);
		param.utf8ExceptionName = NULL;
	}
	if(param.exceptionName != NULL)
	{
		delete[] param.exceptionName;
		param.exceptionName = NULL;
	}
	if(param.monitorClassName != NULL)
	{
		delete[] param.monitorClassName;
		param.monitorClassName = NULL;
	}

	if(param.useMonitorThread != NULL)
	{
		delete[] param.useMonitorThread;
		param.useMonitorThread = NULL;
	}
}

/**
 * 監視対象メソッドに挿入されるJavaコード（Callbacks.class）に呼ばれる
 * 関数（callback）
 */
JNIEXPORT jstring JNICALL Java_Callbacks_dumpstackForMethod(JNIEnv *jni, jclass jclazz, 
														   jobject thread, jstring classUrl)
{
    LOG4CXX_DEBUG(Global::logger, "come into Java_Callbacks_dumpstackForMethod");

	if(Global::isErrorStatus())
	{
		LOG4CXX_DEBUG(Global::logger, "Now is in erorr status,no dump");
		return NULL;
	}

	jvmtiEnv *jvmti = Global::jvmti;
	jvmtiPhase phase;
    jvmti->GetPhase(&phase); 
    if(JVMTI_PHASE_LIVE != phase)
	{
        LOG4CXX_DEBUG(Global::logger, "jvm is in not in live phase.");
		return NULL;
	}

	AutoMonitor Monitoring(Global::configMonitor);

	const char* pTmp = jni->GetStringUTFChars((jstring)classUrl, NULL);
	string urls = pTmp;
	jni->ReleaseStringUTFChars((jstring)classUrl,pTmp);

    //get output setting
	if (AgentCallbackHandler::methodDumpInfo == NULL)
	{
		return NULL;
	}
	map<string,MonitoringMethodInfo*>::const_iterator iter;
    iter = AgentCallbackHandler::methodDumpInfo->find(urls);
	if (iter == AgentCallbackHandler::methodDumpInfo->end())
	{
		return NULL;
	}
	MonitoringMethodInfo* pMethodInfo = iter->second;
	
	StackTraceParam param;
	param.jni = jni;
	param.jvmti = jvmti;
	param.thread = thread;
	param.dumpType = DUMP_KIND_METHOD;  //for method
	param.exceptionCls = NULL;
	param.exceptionName = NULL;
	param.utf8ExceptionName = NULL;
	param.monitorClassName = NULL; 
    param.classUrl = urls;
	param.classSuffix = pMethodInfo->classSuffix;
    param.exceptionObj = NULL;//add by Qiu Song on 20090811 for 例外オブジェクトダンプ
	param.monitorObj = NULL;//add by Qiu Song on 20090819 for モニターオブジェクトダンプ
	param.useMonitorThread = NULL;//add by Qiu Song on 20091006 for 競合しているモニターの所有スレッド取得
    param.packageName = NULL;

	//important!! to set the current output setting
	Global::getConfig()->setOutputSetting(pMethodInfo->getOutputSetting());

	param.dumpInstance = Global::getConfig()->getOutputSetting()->getDumpInstance();
	param.dumpCount = 0;
	string filePath = dumpStackForMethod(&param,jni);

	if (filePath.empty() || param.dumpCount == 0) 
		return NULL;
	else
	{
		sendMailMethod(jni,(char*)filePath.c_str());
        return NULL;
	}
}

void  sendMailMethod(JNIEnv *jni,char* dumpFilePath)
{
	//check if to send mail
    OutputSetting *setting = Global::getConfig()->getOutputSetting();
	map<string, string> *mailSetting = Global::getConfig()->getMailSetting();
    if(setting->getMail() && mailSetting!= NULL)
	{
		//if to attach dump file
		bool attachFile = setting->getAttachFile();
		bool hasError = false;
	    jclass classCallbacks = jni->FindClass("Callbacks");
		jmethodID sendMailForMethod = NULL;
		if(classCallbacks == NULL)
		{
			LOG4CXX_FATAL(Global::logger, "can't get class Callbacks");
			hasError = true;
		}
	
		if(!hasError)
		{
			sendMailForMethod = jni->GetStaticMethodID(classCallbacks,"sendMailForMethod",
			"(Ljava/lang/String;Z)Z");
			if(sendMailForMethod == NULL)
			{
				hasError = true;
				LOG4CXX_FATAL(Global::logger, "can't get method Callbacks::sendMailForMethod");
			}
		}
		if(!hasError)
		{
			jstring dumpPathUTF = jni->NewStringUTF(dumpFilePath);
            jboolean attachFileBool = JNI_TRUE;
            if(!attachFile)
			{
                attachFileBool = JNI_FALSE;
			}

            jni->CallStaticBooleanMethod(
		        classCallbacks,sendMailForMethod,
				dumpPathUTF,attachFileBool);
		}

    }
}

//全スレッドダンプ
void JNICALL dumpAllThreadsForSignal(jvmtiEnv* jvmti, JNIEnv* jni, void* arg)
{
	LOG4CXX_DEBUG(Global::logger,"Enter dumpAllThreads.");

	jthread currentThread = (jthread)arg;

	if(Global::isErrorStatus())
	{
		LOG4CXX_ERROR(Global::logger, "Now is in erorr status,no dump");
	    jni->DeleteGlobalRef(currentThread);
		return;
	}

	jvmtiPhase phase;
    jvmti->GetPhase(&phase); 
    if(JVMTI_PHASE_LIVE != phase)
	{
        LOG4CXX_DEBUG(Global::logger, "jvm is in not in live phase.");
		jni->DeleteGlobalRef(currentThread);
		return;
	}

	 //this monitor will be auto released at the end of this method
	 AutoMonitor Monitoring(Global::configMonitor);
  
      //important!! to set the current output setting
     Global::getConfig()->setSignalOutputSetting();

	 int dumpKind = Global::getConfig()->getSignalDumpKind();
	 time_t time1;
	 time(&time1);
	 struct tm *gt;
	 gt = localtime(&time1);
	 char* middleNameBuf = new char[MAX_BUF_LEN];
	 if (dumpKind == 1){
		sprintf(middleNameBuf,"AllThreads%02d%02d%02d",
		    gt->tm_hour,gt->tm_min,gt->tm_sec);
	 }else{
		sprintf(middleNameBuf,"MonitoringObjects%02d%02d%02d",
		    gt->tm_hour,gt->tm_min,gt->tm_sec);
	 }

	 char* dumpDir = makeDumpDir(middleNameBuf,NULL);
	 if(dumpDir == NULL){
		 return;
	 }

	 //get all threads
	 jint threadsCount = 0;
	 jthread* threads = NULL;
	 jvmtiError err;
	 jvmtiError* errResults = NULL;
	 err = jvmti->GetAllThreads(&threadsCount,&threads);
	 if(err != JVMTI_ERROR_NONE)
	 {
		 Global::logger->logError("failed to get all threads,err cd =%d",err);
		 jni->DeleteGlobalRef(currentThread);
		 delete dumpDir;
		 return;
	 }
	 AUTO_REL_JVMTI_OBJECT(threads);

	 errResults = new jvmtiError[threadsCount];
	 jthread* objectThreads = new jthread[threadsCount];
	 int i=0;
	 int objThreadsCount = 0;
	 for(i=0; i<threadsCount;i++)
	 {
		 if(jni->IsSameObject(currentThread,threads[i]))
		 {
			 ;//found same thread
		 }
		 else
		 {
			 objectThreads[objThreadsCount++] = threads[i];
		 }
	 }
	
	 err =  jvmti->SuspendThreadList(objThreadsCount,objectThreads,errResults);
 
	 if(err != JVMTI_ERROR_NONE)
	 {
		 Global::logger->logError("failed to suspend all threads,err cd =%d",err);
	 } 

	 if (dumpKind == 1)  //ALL Thread dump
	 {	 
		 for(i =0; i  < objThreadsCount;i++)
		 {
			//add by Qiu Song on 20090819 for 指定タイプのスレッドをダウンロード
			//char* dumpThreadStatus = Global::getConfig()->getThreadStatus(); 障害#457
			//int dumpThreadPriority = Global::getConfig()->getThreadPriority(); 障害#457
			char* dumpThreadStatus = Global::getConfig()->getOutputSetting()->getDumpAllThreads();
			int dumpThreadPriority = Global::getConfig()->getOutputSetting()->getThreadPriority();
			if(HelperFunc::shouldDumpCurrentThread(jvmti,objectThreads[i],dumpThreadStatus, dumpThreadPriority) == false)
			{
				continue;
			}
			//end of add by Qiu Song on 20090819 for 指定タイプのスレッドをダウンロード
			jvmtiThreadInfo threadInfo;
			err = jvmti->GetThreadInfo(objectThreads[i],&threadInfo);
			if(err != JVMTI_ERROR_NONE)
			{
				Global::logger->logError("failed to get thread info ,err cd =%d",err);
				continue;
			}
        
			char* exceptionName = HelperFunc::utf8_to_native(threadInfo.name);
			if (exceptionName == NULL)
				exceptionName = HelperFunc::strdup(threadInfo.name);

			StackTraceParam param;

			//suspend the thread
			//Global::jvmti->SuspendThread(threads[i]);
			param.jni = jni;
			param.jvmti = jvmti;
			param.thread = objectThreads[i];
			param.exceptionCls = NULL;
			param.utf8ExceptionName = HelperFunc::strdup(threadInfo.name);
			param.exceptionName = exceptionName;
			param.dumpType = DUMP_KIND_THREAD;
			param.dumpInstance = false;
			param.dumpCount = 0;
			param.exceptionObj = NULL;//add by Qiu Song on 20090811 for 例外オブジェクトダンプ
            param.monitorObj = NULL;//add by Qiu Song on 20090819 for モニターオブジェクトダンプ
			param.useMonitorThread = NULL;//add by Qiu Song on 20091006 for モニターオブジェクトダンプ
			param.packageName = NULL;//add by Qiu Song on 20091007 for バグ：メモリリック

			//add by Qiu Song on 20090818 for モニターオブジェクトダンプ
			param.monitorObj = getWaitMonitor(jvmti, jni, param.thread);
			param.useMonitorThread = getMonitorUseThreadName(jvmti, jni, objectThreads, objThreadsCount, param.monitorObj);
			//end of add by Qiu Song on 20090818 for モニターオブジェクトダンプ

			dumpStackOneThreadForSignal(&param,jni,(const char*)dumpDir);
        
			if(param.utf8ExceptionName != NULL)
			{
				delete[] (char*)(param.utf8ExceptionName);
				param.utf8ExceptionName = NULL;
			}
			if(param.exceptionName != NULL)
			{
				delete[] param.exceptionName;
				param.exceptionName = NULL;
			}
			if(param.useMonitorThread != NULL)
			{
				delete[] param.useMonitorThread;
				param.useMonitorThread = NULL;
			}
			jvmti->Deallocate((unsigned char*)(threadInfo.name));

			if(threadInfo.thread_group != NULL)
			{
				 jni->DeleteLocalRef(threadInfo.thread_group);
			}
			if(threadInfo.context_class_loader != NULL)
			{
				 jni->DeleteLocalRef(threadInfo.context_class_loader);
			}
		}
	 }
	 else
	 {
		 StackTraceParam param;
		 
		 //suspend the thread
		 //Global::jvmti->SuspendThread(threads[i]);
		 param.jni = jni;
		 param.jvmti = jvmti;
		 param.thread = currentThread;
		 param.exceptionCls = NULL;
		 param.utf8ExceptionName = NULL;
		 param.exceptionName = NULL;
		 param.dumpType = DUMP_KIND_THREAD;
		 param.dumpInstance = true;
		 param.exceptionObj = NULL;//add by Qiu Song on 20090811 for 例外オブジェクトダンプ
		 param.monitorObj = getWaitMonitor(jvmti, jni, param.thread);//add by Qiu Song on 20090819 for モニターオブジェクトダンプ
         param.useMonitorThread = getMonitorUseThreadName(jvmti, jni, objectThreads, objThreadsCount, param.monitorObj);
		 param.packageName = NULL;//add by Qiu Song on 20091007 for バグ：メモリリック

		 dumpStackOneThreadForSignal(&param,jni,(const char*)dumpDir);
		 if(param.useMonitorThread != NULL)
		 {
			 delete param.useMonitorThread;
			 param.useMonitorThread = NULL;
		 }
	 }
	 
	jvmti->ResumeThreadList(objThreadsCount,objectThreads,errResults);
	delete[] errResults;
	delete[] objectThreads;
	jni->DeleteGlobalRef(currentThread);

	for(i = 0; i < threadsCount;i++)
	{
		 jni->DeleteLocalRef(threads[i]);
	}

	delete dumpDir;
    //_CrtDumpMemoryLeaks();
}

/**
 *指定した条件（例外、メソッド）ですべてのスレッドをダンプする
 */
void JNICALL dumpAllThreadsForCondition(jvmtiEnv* jvmti, JNIEnv* jni, 
					jthread currentThread,const char* dumpDir,int dumpKind,StackTraceParam* sparam)
{
	if(Global::isErrorStatus())
	{
		LOG4CXX_ERROR(Global::logger, "Now is in erorr status,no dump");
		return;
	}

	jvmtiPhase phase;
    jvmti->GetPhase(&phase); 
    if(JVMTI_PHASE_LIVE != phase)
	{
        LOG4CXX_DEBUG(Global::logger, "jvm is in not in live phase.");
		return;
	}

	 //this monitor will be auto released at the end of this method
	 AutoMonitor Monitoring(Global::configMonitor);
  
     //get all threads
	 jint threadsCount = 0;
	 jthread* threads = NULL;
	 jvmtiError err;
	 jvmtiError* errResults = NULL;
	 err = jvmti->GetAllThreads(&threadsCount,&threads);
	 if(err != JVMTI_ERROR_NONE)
	 {
		 Global::logger->logError("failed to get all threads,err cd =%d",err);
		 return;
	 }
	 AUTO_REL_JVMTI_OBJECT(threads);

	 errResults = new jvmtiError[threadsCount];
	 jthread* objectThreads = new jthread[threadsCount];
	 int i=0;
	 int objThreadsCount = 0;
	 int currentThreadIndex = 0;
	 for(i=0; i<threadsCount;i++)
	 {
		 if(jni->IsSameObject(currentThread,threads[i]))
		 {
			 currentThreadIndex = i;//found same thread
		 }else
		 {
			 objectThreads[objThreadsCount++] = threads[i];
		 }
	 }
	
	 err =  jvmti->SuspendThreadList(objThreadsCount,objectThreads,errResults);
 
	 if(err != JVMTI_ERROR_NONE)
	 {
		 Global::logger->logError("failed to suspend all threads,err cd =%d",err);
	 } 

	 for(i =0; i  < threadsCount;i++)
	 {
		jvmtiThreadInfo threadInfo;
		err = jvmti->GetThreadInfo(threads[i],&threadInfo);
		if(err != JVMTI_ERROR_NONE)
		{
			Global::logger->logError("failed to get thread info ,err cd =%d",err);
			continue;
		}
    
		//add by Qiu Song on 20091022 for ダンプスレッド優先度の指定
		char* threadStatusConfig =	Global::getConfig()->getOutputSetting()->getDumpAllThreads();
        int threadPriorityConfig = Global::getConfig()->getOutputSetting()->getThreadPriority();
		if( HelperFunc::shouldDumpCurrentThread(jvmti, threads[i], threadStatusConfig, threadPriorityConfig) == false)
		{
			continue;
		}
		//end of add by Qiu Song on 20091022 for ダンプスレッド優先度の指定

		char* exceptionName = HelperFunc::utf8_to_native(threadInfo.name);
		if (exceptionName == NULL){
			exceptionName = HelperFunc::strdup(threadInfo.name);
        }

		StackTraceParam param;

		param.jni = jni;
		param.jvmti = jvmti;
		param.thread = threads[i];
		param.exceptionCls = NULL;
		param.useMonitorThread = NULL;
		param.utf8ExceptionName = HelperFunc::strdup(threadInfo.name);
		param.exceptionName = exceptionName;
		param.dumpType = dumpKind;
		param.dumpInstance = Global::getConfig()->getOutputSetting()->getDumpInstance();
		//param.dumpInstance = false;
        param.dumpCount = 0;
        param.exceptionObj = sparam->exceptionObj;//add by Qiu Song on 20090811 for 例外オブジェクトダンプ
		param.packageName = HelperFunc::strdup(sparam->packageName);//add by Qiu Song on 20091007 for バグ：メモリリック

		//add by Qiu Song on 20090818 for モニターオブジェクトダンプ
		param.monitorObj = getWaitMonitor(jvmti, jni, param.thread);
		
		if(param.useMonitorThread != NULL)
		{
			delete param.useMonitorThread;
		}
		param.useMonitorThread = getMonitorUseThreadName(jvmti, jni, objectThreads, objThreadsCount, param.monitorObj);
		//end of add by Qiu Song on 20090818 for モニターオブジェクトダンプ

		dumpStackOneThreadForCondition(&param,jni,dumpDir,i==currentThreadIndex);
        sparam->dumpCount = sparam->dumpCount + param.dumpCount;
		if(param.utf8ExceptionName != NULL)
		{
			delete[] (char*)(param.utf8ExceptionName);
			param.utf8ExceptionName = NULL;
		}
		if(param.exceptionName != NULL)
		{
			delete[] param.exceptionName;
			param.exceptionName = NULL;
		}
		if(param.packageName != NULL)
		{
			delete param.packageName;
			param.packageName = NULL;
		}
		if(param.useMonitorThread != NULL)
		{
			delete param.useMonitorThread;
			param.useMonitorThread = NULL;
		}
		jvmti->Deallocate((unsigned char*)(threadInfo.name));

		if(threadInfo.thread_group != NULL)
		{
			 jni->DeleteLocalRef(threadInfo.thread_group);
		}
		if(threadInfo.context_class_loader != NULL)
		{
			 jni->DeleteLocalRef(threadInfo.context_class_loader);
		}
	}
	 
	jvmti->ResumeThreadList(objThreadsCount,objectThreads,errResults);
	delete[] errResults;
	delete[] objectThreads;

	for(i = 0; i < threadsCount;i++)
	{
		 jni->DeleteLocalRef(threads[i]);
	}

    //_CrtDumpMemoryLeaks();
}

/**
 * 監視対象メソッドに挿入されるJavaコード（Callbacks.class）に呼ばれる
 * 関数（callback）
 */
JNIEXPORT void JNICALL Java_Callbacks_registerInstance(JNIEnv *jni, jclass jclazz, 
														jobject thread, jobject insObj,
														jstring className, 
														jstring classUrl)
{
	if (AgentCallbackHandler::WM_Death_Started == 1)
		return;

	if(Global::isErrorStatus())
		return;

	jvmtiEnv *jvmti = Global::jvmti;
	
	jvmtiPhase phase;
    jvmti->GetPhase(&phase); 
    if(JVMTI_PHASE_LIVE != phase && JVMTI_PHASE_START != phase)
	{
        LOG4CXX_DEBUG(Global::logger, "jvm is in not in live or start phase.");
		return ;
	}

	//get class name from insobj
	jclass clazz = jni->GetObjectClass(insObj);
	if(clazz == NULL)
    {
		Global::logger->logError("failed to call GetObjectClass in Java_Callbacks_registerInstance.");
		return ;
	}
    AUTO_REL_JNI_LOCAL_REF(jni, clazz);
    
	char *sig = NULL;
	jvmtiError err = jvmti->GetClassSignature(clazz, &sig, NULL);
	if(err != JVMTI_ERROR_NONE)
	{
		Global::logger->logError("failed to call GetClassSignature in Java_Callbacks_registerInstance,error cd =%d.",err);
		return ;
	}
	AUTO_REL_JVMTI_OBJECT(sig);

	char* tmp = HelperFunc::convertClassSig(sig);
	string key = tmp;
	delete[] tmp;
	const char* pClassName = jni->GetStringUTFChars((jstring)className, NULL);
	if (strcmp(pClassName, key.c_str()) != 0)
	{
		jni->ReleaseStringUTFChars((jstring)className,pClassName);
		return;
	}
	jni->ReleaseStringUTFChars((jstring)className,pClassName);
	
	key.append("#");
	const char* pTmp = jni->GetStringUTFChars((jstring)classUrl, NULL);
	key.append(pTmp);
	jni->ReleaseStringUTFChars((jstring)classUrl,pTmp);

	jweak ref = jni->NewWeakGlobalRef(insObj);
	
	map<string, vector<jweak>*>* pMap = AgentCallbackHandler::instanceMap;
	//AgentCallbackHandler::instanceMapに<className#classUrl, vector<jweak>>要素を挿入する。
	if (pMap == NULL)
	{
		pMap = new map<string, vector<jweak>*>;
		AgentCallbackHandler::instanceMap = pMap;
	}
	else
	{
		map<string, vector<jweak>*>::const_iterator it;
		it = pMap->find(key);
		
		if (it != pMap->end())
		{
			vector<jweak>* pVec = it->second;
			if (pVec->size() == 0)
			{
				pVec->push_back(ref);
				return;
			}

			if (jni->IsSameObject(ref, (*pVec)[pVec->size() -1]) == JNI_TRUE)
			{
				jni->DeleteWeakGlobalRef(ref);
				return;
			}

			//delete by Qiu Song on 20090909 for インスタンス制限の削除
			//if (pVec->size() < MAX_INSTANCE_COUNT)
			//{
				pVec->push_back(ref);
				return;
			//}
			//vectorのサイズが32であれば、解放されたオブジェクトをvectorから削除する。
			//vectorのサイズが32であれば、insObjを保存しない。
			/*vector<jweak>::iterator it = pVec->end();
			vector<jweak>::iterator it0;
			for (it--;it != pVec->begin();)
			{
				if (jni->IsSameObject(*it, NULL) == JNI_TRUE)
				{
					it0 = it;
					it--;
					pVec->erase(it0);
				}
				else
				{
					it--;
				}
			}
			if (jni->IsSameObject(*it, NULL) == JNI_TRUE)
			{
				pVec->erase(it);
			}
			if (pVec->size() < MAX_INSTANCE_COUNT)
			{
				pVec->push_back(ref);
			}
			else
			{
				jni->DeleteWeakGlobalRef(ref);
			}
			return;*/
			//end of delete by Qiu Song on 20090909 for インスタンス制限の削除
		}
	}
	vector<jweak>* pVec = new vector<jweak>;
	pVec->push_back(ref);
	pMap->insert(pair<string, vector<jweak>*>(key, pVec));
}


static int dumpCount = 0;

//make directory for dump file like the following
//yyyymmdd\middleDirName1\middleDirName2
char* makeDumpDir(char* middleDirName1,char* middleDirName2)
{
	if(middleDirName1 == NULL)
		return NULL;
    
    const char *filePath = Global::getConfig()->getDumpFilePath();
    
	time_t nowtime;
	time(&nowtime);
	struct tm *gt;
	gt = localtime(&nowtime);
   
	//make a directory with the name of "yyyymmdd"
	char* dirNameBuf = new char[MAX_BUF_LEN];
	AUTO_REL_OBJECT_ARR(dirNameBuf);
	string namebuf;
	string errmsgbuf;
	namebuf = filePath;
    namebuf += FILE_SPERATOR;

	sprintf(dirNameBuf,"%04d%02d%02d",
		gt->tm_year+1900,gt->tm_mon+1,gt->tm_mday);
	namebuf += dirNameBuf;

    if(!HelperFunc::makeDirectory((char*)namebuf.c_str()))
	{
		errmsgbuf = "Can't make directory: ";
        errmsgbuf += namebuf;
		LOG4CXX_ERROR_NATIVE(Global::logger, errmsgbuf.c_str());
		return NULL;
    }

	//make a directory with middleDirName1
    namebuf += FILE_SPERATOR;
	namebuf += middleDirName1;
	if(!HelperFunc::makeDirectory((char*)namebuf.c_str()))
	{
		errmsgbuf = "Can't make directory: ";
		errmsgbuf += namebuf;
		LOG4CXX_ERROR_NATIVE(Global::logger, errmsgbuf.c_str());
		return NULL;
	}

	//make a directory with middlDirName2
	if(middleDirName2 != NULL)
	{
		namebuf += FILE_SPERATOR;
		namebuf += middleDirName2;
		if(!HelperFunc::makeDirectory((char*)namebuf.c_str()))
		{
			errmsgbuf = "Can't make directory: ";
			errmsgbuf += namebuf;
			LOG4CXX_ERROR_NATIVE(Global::logger, errmsgbuf.c_str());
			return NULL;
		}
	}
    int pathLen = namebuf.size();
	char* path = new char[pathLen + 1];
	strcpy(path,(char*)namebuf.c_str());
    return path;
}

//add necessaray info to dumpfile name
void appendDumpFileName(string& fileNameBuf,DumpFileTime* nowtime,JNIEnv *jni)
{
	dumpCount++;
	//get jvm information  
	SystemInfo* systemInfo = SystemInfo::getInstance();
    systemInfo->getJVMInfo(jni);

	//delete by Qiu Song on 2009.08.10 for ダンプファイル名の変更(ミリ秒にアップ)
	//struct tm *gt;
	//gt = localtime(&nowtime);
	//end of delete by Qiu Song on 2009.08.10 for ダンプファイル名の変更(ミリ秒にアップ)
	
	fileNameBuf += "_";
	fileNameBuf += systemInfo->getSystemID();
	fileNameBuf += "_";
	fileNameBuf += systemInfo->getJvmID();
	fileNameBuf += "_";
	fileNameBuf += systemInfo->getJvmVersion();
    fileNameBuf += "_";

	char* dirNameBuf = new char[MAX_BUF_LEN];

	//modified by Qiu Song on 2009.08.10 for ダンプファイル名の変更(ミリ秒にアップ)
	if(nowtime == NULL)
	{
		//struct _timeb timebuffer;
		//_ftime(&timebuffer);
		struct timeb timebuffer;
		ftime(&timebuffer);
		unsigned short millitm1 = timebuffer.millitm;
		struct tm *gt;
		gt = localtime(&(timebuffer.time));

		sprintf(dirNameBuf,"%02d%02d%02d%03d_%03d.dat",
			gt->tm_hour,gt->tm_min,gt->tm_sec,millitm1,dumpCount);
	}
	else
	{
		sprintf(dirNameBuf,"%02d%02d%02d%03d_%03d.dat",
		        nowtime->m_hour, nowtime->m_min, nowtime->m_sec, nowtime->m_millianSec,dumpCount);
	}
	//end of modified by Qiu Song on 2009.08.10 for ダンプファイル名の変更(ミリ秒にアップ)
	
	fileNameBuf += dirNameBuf;

	delete[] dirNameBuf;
}

//dump stack info for exception or stackoverflow error
string dumpStackForException(StackTraceParam* sparam,JNIEnv *jni)
{
	OutputSetting *outputSetting = Global::getConfig()->getOutputSetting();
	StackTrace *stackTrace = NULL;

	string fileNameBuf = "";
	try{
		time_t time1;
		time(&time1);

		LOG4CXX_DEBUG(Global::logger, "dumpStackForException begin");

		stackTrace = new StackTrace(sparam);

		//modified by Qiu Song on 20090917 for ダンプスレッド状態指定
		//if(outputSetting->getDumpAllThreads()){
		char* strDumpAllThread = outputSetting->getDumpAllThreads();
		if(strDumpAllThread != NULL && strcmp(strDumpAllThread, "current") != 0){
		//end of modified by Qiu Song on 20090917 for ダンプスレッド状態指定
			//全スレッドをダンプするオプションの場合、まずダンプ対象かどうか判断
			if(!stackTrace->shouldDumpException()){
				delete stackTrace;
				stackTrace = NULL;
				return fileNameBuf;
			}else{
                LOG4CXX_INFO(Global::logger, "dump all threads")
				delete stackTrace;
				stackTrace = NULL;
				//make dump directories for dump files
                char* exceptionNamedir = HelperFunc::replaceChar(
				sparam->exceptionName,'.','_');
			    AUTO_REL_OBJECT_ARR(exceptionNamedir);
			    struct tm *gt;
	            gt = localtime(&time1);
				char* middleNameBuf = new char[MAX_BUF_LEN];
				sprintf(middleNameBuf,"AllThreads%02d%02d%02d",
		            gt->tm_hour,gt->tm_min,gt->tm_sec);

				//modified by Qiu Song on 20091009 for バグ：自動例外監視のスレッドダンプのフォルダ名不正
				//char* dumpDir = makeDumpDir(exceptionNamedir,middleNameBuf);
				char* dumpDir;
				if(sparam->dumpType == DUMP_KIND_THROWABLE && sparam->packageName != NULL)
				{
					dumpDir = makeDumpDir(sparam->packageName, middleNameBuf);
				}
				else
				{
					dumpDir = makeDumpDir(exceptionNamedir,middleNameBuf);
				}
				//end of add by Qiu Song on 20091009 for バグ：自動例外監視のスレッドダンプのフォルダ名不正
				delete middleNameBuf;
				
				if(dumpDir != NULL){
					//dump all thread for exception
				    dumpAllThreadsForCondition(sparam->jvmti,jni,sparam->thread,
						dumpDir,DUMP_KIND_EXCEPTION_ALL,sparam);
					fileNameBuf = dumpDir;
					delete dumpDir;
					dumpDir = NULL;
				}
			}
		}else{
			//dump one thread
		    stackTrace->init();
			time_t time2;
			time(&time2);

			if(stackTrace->getMethodNum() > 0)
			{
				const char *filePrefix = Global::getConfig()->getDumpFilePrefix();
				char* exceptionNamedir = HelperFunc::replaceChar(
					sparam->exceptionName,'.','_');
				AUTO_REL_OBJECT_ARR(exceptionNamedir);
				//add by Qiu Song on 20090924 for 自動監視機能のパッケージ名出力
				char* dumpDir = NULL;
				if(sparam->dumpType == DUMP_KIND_THROWABLE && sparam->packageName != NULL)
				{
					dumpDir = makeDumpDir(sparam->packageName, exceptionNamedir);
				}
				else
				{
					dumpDir = makeDumpDir(exceptionNamedir,NULL);
				}
				//end of add by Qiu Song on 20090924 for 自動監視機能のパッケージ名出力
				if(dumpDir != NULL)
				{
					fileNameBuf = dumpDir;
					fileNameBuf += FILE_SPERATOR;
					fileNameBuf += filePrefix;
					//add by Qiu Song on 20091105 for タスクprefix出力
					char* taskPrefix = Global::getConfig()->getOutputSetting()->getTaskPrefix();
					if(taskPrefix != NULL)
					{
						fileNameBuf += "_";
						fileNameBuf += taskPrefix;
					}
					//end of add by Qiu Song on 20091105 for タスクprefix出力
 					appendDumpFileName(fileNameBuf, stackTrace->getDumpFileTime(),jni);
		
       				LOG4CXX_DEBUG(Global::logger, "write dump data into file....");
					bool ret = stackTrace->writeToFile(fileNameBuf.c_str(),jni);
					if(ret)
					{
						time_t time3;
						time(&time3);
						char logbuf[256];
						sprintf(logbuf,"dump time:time1 = %ld,time2 =%ld,id = %d",
							time2-time1,time3-time2,dumpCount);
						LOG4CXX_DEBUG(Global::logger, logbuf);
						sparam->dumpCount += 1;
					}
					else
					{
						fileNameBuf = "";
					}
					delete[] dumpDir;
				}//of if(dumpDir != NULL)
			}//of if(stackTrace->getMethodNum() > 0)
			delete stackTrace;
		}

		LOG4CXX_DEBUG(Global::logger, "dumpStackForException:end");
	}catch(exception& e)
	{
		if(stackTrace != NULL)
		{
			delete stackTrace;
			stackTrace = NULL;
		}
		fileNameBuf = "";
        LOG4CXX_ERROR(Global::logger, e.what());
		Global::setErrorStatus(true); 
	}catch(...)
	{
		if(stackTrace != NULL)
		{
			delete stackTrace;
			stackTrace = NULL;
		}
		fileNameBuf = "";
		LOG4CXX_ERROR(Global::logger,"Unknown error happened when dumping.");
		Global::setErrorStatus(true); 
	}

	return fileNameBuf;
}

void dumpStackOneThreadForCondition(StackTraceParam* sparam,JNIEnv *jni,
						   const char* dumpDir,
						   bool currentThreadFlag)
{
	StackTrace* stackTrace = NULL;
	try{
		time_t time1;
		time(&time1);

        stackTrace = new StackTrace(sparam);
		stackTrace->dumpThread(currentThreadFlag);

		time_t time2;
		time(&time2);
        
        if(stackTrace->getMethodNum() > 0)
        {
			const char *filePrefix = Global::getConfig()->getDumpFilePrefix();
            string fileNameBuf = dumpDir;
			char* threadname = HelperFunc::strdup(sparam->exceptionName);
			HelperFunc::convertInvalidFileNameChar(threadname);
			
            fileNameBuf += FILE_SPERATOR;

			if(currentThreadFlag){
				fileNameBuf += "@";
			}

            fileNameBuf += filePrefix;
			//add by Qiu Song on 20091105 for タスクprefix出力
			char* taskPrefix = Global::getConfig()->getOutputSetting()->getTaskPrefix();
			if(taskPrefix != NULL)
			{
				fileNameBuf += "_";
				fileNameBuf += taskPrefix;
			}
			//end of add by Qiu Song on 20091105 for タスクprefix出力
            fileNameBuf += "_";
            fileNameBuf += threadname;  //thread name
			delete[] threadname;
			appendDumpFileName(fileNameBuf, stackTrace->getDumpFileTime(), jni);

            bool ret = stackTrace->writeToFile(fileNameBuf.c_str(),jni);
			if(ret)
			{
				time_t time3;
				time(&time3);
				char logbuf[256];
				sprintf(logbuf,"dump time:time1 = %ld,time2 =%ld,id = %d",
					time2-time1,time3-time2,dumpCount);
				LOG4CXX_DEBUG(Global::logger, logbuf);
				sparam->dumpCount += 1;
			}
        }
		delete stackTrace;
		stackTrace = NULL;
	
    }catch(exception& e)
	{
		if(stackTrace != NULL)
		{
			delete stackTrace;
			stackTrace = NULL;
		}
        LOG4CXX_ERROR(Global::logger, e.what());
		Global::setErrorStatus(true); 
	}catch(...)
	{
		if(stackTrace != NULL)
		{
			delete stackTrace;
			stackTrace = NULL;
		}
		LOG4CXX_ERROR(Global::logger,"Unknown error happened when dumping.");
		Global::setErrorStatus(true); 
	}
}

void dumpStackOneThreadForSignal(StackTraceParam* sparam,JNIEnv *jni,const char* dumpDir)
{
	StackTrace* stackTrace = NULL;
	try{
		time_t time1;
		time(&time1);

		LOG4CXX_DEBUG(Global::logger, "dumpStackOneThreadForSignal begin");
        stackTrace = new StackTrace(sparam);
		if (sparam->dumpInstance)
		{
			stackTrace->dumpInstanceForSignal();
		}
		else
		{
			stackTrace->dumpThread(false); //we don't know which is current thread
		}
		time_t time2;
		time(&time2);
        
        if(stackTrace->getMethodNum() > 0)
        {
			const char *filePrefix = Global::getConfig()->getDumpFilePrefix();
            string fileNameBuf = dumpDir;

			char* threadname = HelperFunc::strdup(sparam->exceptionName);
			HelperFunc::convertInvalidFileNameChar(threadname);
			
            fileNameBuf += FILE_SPERATOR;
            fileNameBuf += filePrefix;
			//add by Qiu Song on 20091105 for タスクprefix出力
			char* taskPrefix = Global::getConfig()->getOutputSetting()->getTaskPrefix();
			if(taskPrefix != NULL)
			{
				fileNameBuf += "_";
				fileNameBuf += taskPrefix;
			}
			//end of add by Qiu Song on 20091105 for タスクprefix出力
            fileNameBuf += "_";
            fileNameBuf += threadname;  //thread name
			delete[] threadname;
			appendDumpFileName(fileNameBuf, stackTrace->getDumpFileTime(), jni);

            bool ret = stackTrace->writeToFile(fileNameBuf.c_str(),jni);
			if(ret)
			{
				time_t time3;
				time(&time3);
				char logbuf[256];
				sprintf(logbuf,"dump time:time1 = %ld,time2 =%ld,id = %d",
					time2-time1,time3-time2,dumpCount);
				LOG4CXX_DEBUG(Global::logger, logbuf);
			}
 
        }//if(stackTrace->getMethodNum()
		else if (stackTrace->getInstanceNum() > 0)
		{
			const char *filePrefix = Global::getConfig()->getDumpFilePrefix();
            string fileNameBuf = dumpDir;
            fileNameBuf += FILE_SPERATOR;
            fileNameBuf += filePrefix;
            fileNameBuf += "_";
			//add by Qiu Song on 20091105 for タスクprefix出力
			char* taskPrefix = Global::getConfig()->getOutputSetting()->getTaskPrefix();
			if(taskPrefix != NULL)
			{
				fileNameBuf += taskPrefix;
				fileNameBuf += "_";
			}
			//end of add by Qiu Song on 20091105 for タスクprefix出力
			appendDumpFileName(fileNameBuf, stackTrace->getDumpFileTime(), jni);
			
            bool ret = stackTrace->writeToFile(fileNameBuf.c_str(),jni);
			if(ret)
			{
				time_t time3;
				time(&time3);
				char logbuf[256];
				sprintf(logbuf,"dump time:time1 = %ld,time2 =%ld,id = %d",
					time2-time1,time3-time2,dumpCount);
				LOG4CXX_DEBUG(Global::logger, logbuf);
			}
		}
		if(stackTrace != NULL)
		{
			delete stackTrace;
			stackTrace = NULL;
		}
    }catch(exception& e)
	{
		if(stackTrace != NULL)
		{
			delete stackTrace;
			stackTrace = NULL;
		}
        LOG4CXX_ERROR(Global::logger, e.what());
		Global::setErrorStatus(true); 
	}catch(...)
	{
		if(stackTrace != NULL)
		{
			delete stackTrace;
			stackTrace = NULL;
		}
		LOG4CXX_ERROR(Global::logger,"Unknown error happened when dumping.");
		Global::setErrorStatus(true); 
	}
}

char* makeDumpDirForMethod(StackTraceParam* sparam){
	//use class name as dir
	char* classNameDirTmp = HelperFunc::replaceChar(sparam->className.c_str(),'.','_');
	char* classNameDir = HelperFunc::replaceChar(classNameDirTmp,'$','#');
	AUTO_REL_OBJECT_ARR(classNameDirTmp);
	AUTO_REL_OBJECT_ARR(classNameDir);

	string classNamebuf = classNameDir;
	if (sparam->classSuffix.length() > 0)
	{
		classNamebuf += "_";
		classNamebuf += sparam->classSuffix;
	}
	//use method name as dir
	string methodNameBuf ="";
	const char* methodNameDir = sparam->methodName.c_str();
	if (strcmp(methodNameDir, "<init>") == 0)
	{
		//get constructor name
		const char* constructorName = strrchr(sparam->className.c_str(), '.');
		if (constructorName == NULL)
		{
			constructorName = sparam->className.c_str();
			char* methodNameDir = HelperFunc::replaceChar(constructorName,'$','#');
			AUTO_REL_OBJECT_ARR(methodNameDir);
			methodNameBuf += methodNameDir;
		}
		else
		{
			char* methodNameDir = HelperFunc::replaceChar(constructorName + 1,'$','#');
			AUTO_REL_OBJECT_ARR(methodNameDir);
			methodNameBuf += methodNameDir;
		}
	}
	else
	{
		char* methodNameDir = HelperFunc::replaceChar(sparam->methodName.c_str(),'$','#');
		AUTO_REL_OBJECT_ARR(methodNameDir);
		methodNameBuf += methodNameDir;
	}
	if (sparam->methodSuffix.length() > 0)
	{
		methodNameBuf += "_";
		methodNameBuf += sparam->methodSuffix;
	}

	char* nativeClassName = HelperFunc::utf8_to_native(classNamebuf.c_str());
	char* nativeMethodName = HelperFunc::utf8_to_native(methodNameBuf.c_str());
	AUTO_REL_OBJECT_ARR(nativeClassName);
	AUTO_REL_OBJECT_ARR(nativeMethodName);

	char* dumpDir = makeDumpDir(nativeClassName,nativeMethodName);
	return dumpDir;
}

//dump stack info for exception or stackoverflow error
string dumpStackForMethod(StackTraceParam* sparam,JNIEnv *jni)
{
	OutputSetting *outputSetting = Global::getConfig()->getOutputSetting();
	StackTrace *stackTrace = NULL;

	string fileNameBuf = "";
	try{
		time_t time1;
		time(&time1);

		LOG4CXX_DEBUG(Global::logger, "dumpStackForMethod begin");

		stackTrace = new StackTrace(sparam);
		//modified by Qiu Song on 20090917 for ダンプスレッド状態指定
		//if(outputSetting->getDumpAllThreads()){
		char* strDumpAllThread = outputSetting->getDumpAllThreads();
		if(strDumpAllThread != NULL && strcmp(strDumpAllThread, "current") != 0){
		//end of modified by Qiu Song on 20090917 for ダンプスレッド状態指定
			//全スレッドをダンプするオプションの場合、まずダンプ対象かどうか判断
			if(!stackTrace->shouldDumpMethod()){
				delete stackTrace;
				stackTrace = NULL;
				return fileNameBuf;
			}else{
				LOG4CXX_INFO(Global::logger, "dump all threads")
				delete stackTrace;
				stackTrace = NULL;
				//make dump directories for dump files
				char* dumpDirPre = makeDumpDirForMethod(sparam);
				if(dumpDirPre == NULL){
                    return fileNameBuf;
				}
				struct tm *gt;
	            gt = localtime(&time1);
				char* middleNameBuf = new char[MAX_BUF_LEN];
				sprintf(middleNameBuf,"AllThreads%02d%02d%02d",
		            gt->tm_hour,gt->tm_min,gt->tm_sec);
				string dumpDirStr = dumpDirPre;
                dumpDirStr += FILE_SPERATOR;
                dumpDirStr += middleNameBuf;
				delete[] middleNameBuf;
				delete[] dumpDirPre;
				if(!HelperFunc::makeDirectory((char*)dumpDirStr.c_str()))
				{
					string errmsgbuf = "Can't make directory: ";
					errmsgbuf += dumpDirStr;
					LOG4CXX_ERROR_NATIVE(Global::logger, errmsgbuf.c_str());
					return fileNameBuf;
				}
				//add by Qiu Song on 20090924 for モニターオブジェクトの取得
				sparam->monitorObj = getWaitMonitor(sparam->jvmti, jni, sparam->thread);
				sparam->useMonitorThread = NULL;
				//end of add by Qiu Song on 20090924 for モニターオブジェクトの取得
				
				//dump all thread for method
				dumpAllThreadsForCondition(sparam->jvmti,jni,sparam->thread,
						dumpDirStr.c_str(),DUMP_KIND_METHOD_ALL,sparam);
				fileNameBuf = dumpDirStr;
			}
		}else{

			stackTrace->init();

			time_t time2;
			time(&time2);
			
			if(stackTrace->getMethodNum() > 0)
			{
				const char *filePrefix = Global::getConfig()->getDumpFilePrefix();
				char* dumpDir = makeDumpDirForMethod(sparam);

				if(dumpDir != NULL)
				{
					fileNameBuf = dumpDir;
					fileNameBuf += FILE_SPERATOR;
					fileNameBuf += filePrefix;
					//add by Qiu Song on 20091105 for タスクprefix出力
					char* taskPrefix = Global::getConfig()->getOutputSetting()->getTaskPrefix();
					if(taskPrefix != NULL)
					{
						fileNameBuf += "_";
						fileNameBuf += taskPrefix;
					}
					//end of add by Qiu Song on 20091105 for タスクprefix出力

					appendDumpFileName(fileNameBuf, stackTrace->getDumpFileTime(), jni);

       				LOG4CXX_DEBUG(Global::logger, "write dump data into file....");
					bool ret = stackTrace->writeToFile(fileNameBuf.c_str(),jni);
					if(ret)
					{
						time_t time3;
						time(&time3);
						char logbuf[256];
						sprintf(logbuf,"dump time:time1 = %ld,time2 =%ld,id = %d",
							time2-time1,time3-time2,dumpCount);
						LOG4CXX_DEBUG(Global::logger, logbuf);
						sparam->dumpCount += 1;
					}
					else
					{
						fileNameBuf = "";
					}
					delete[] dumpDir;
					dumpDir = NULL;
				}//of if(dumpDir != NULL)
			}
		
			delete stackTrace;
		}

		LOG4CXX_DEBUG(Global::logger, "dumpStackForMethod:end");

	}catch(exception& e)
	{
		if(stackTrace != NULL)
		{
			delete stackTrace;
			stackTrace = NULL;
		}
		fileNameBuf = "";
        LOG4CXX_ERROR(Global::logger, e.what());
		Global::setErrorStatus(true); 
	}catch(...)
	{
		if(stackTrace != NULL)
		{
			delete stackTrace;
			stackTrace = NULL;
		}
		fileNameBuf = "";
		LOG4CXX_ERROR(Global::logger,"Unknown error happened when dumping.");
		Global::setErrorStatus(true); 
	}

	return fileNameBuf;
}

void activateAgentThread()
{
	if(Global::savedvm == NULL)
	    return;
     
	JNIEnv* jni = NULL;
    int res;
    res = Global::savedvm->AttachCurrentThread((void**)&jni, NULL);
    if (res < 0) 
	{
         Global::logger->logError("Failed to attach current thread to get JNIEnv in activeAgentThread,error cd=%d",res);
         return ;
    }

	//run agent thread 
	jclass classThread = jni->FindClass("java/lang/Thread");
	if(classThread == NULL)
	{
		LOG4CXX_ERROR(Global::logger,"can't find java/lang/Thread.");
		return;
	}
	jmethodID constructor = jni->GetMethodID(classThread,"<init>","()V");
	if(constructor == NULL)
	{
		LOG4CXX_ERROR(Global::logger,"can't find <init> method of thread.");
		return;	
	}
	
	jthread agentThread = (jthread)jni->NewObject(classThread,constructor);
	jthread globalAgentThread = jni->NewGlobalRef(agentThread);

	jvmtiError err = Global::jvmti->RunAgentThread(agentThread,&dumpAllThreadsForSignal,
		globalAgentThread,JVMTI_THREAD_MAX_PRIORITY);

	if(err != JVMTI_ERROR_NONE)
	{
		Global::logger->logError("can't run agent thread in activeAgentThread,error cd =%d",err);
	}
}

/**
 * Throwableを監視する時に、ダンプする
  */
void  dumpstackForAny(JNIEnv *jni, jobject thread, jclass exceptionCls, char*exceptionClassName, char* catchClassSig)
{

	//this monitor will be auto released at the end of this method
	AutoMonitor Monitoring(Global::configMonitor);
	if(Global::isErrorStatus())
	{
		LOG4CXX_DEBUG(Global::logger, "Now is in erorr status,no dump");
		return;
	}

	jvmtiEnv *jvmti = Global::jvmti;

	jvmtiPhase phase;
    jvmti->GetPhase(&phase); 
    if(JVMTI_PHASE_LIVE != phase)
	{
        LOG4CXX_DEBUG(Global::logger, "jvm is in not in live phase.");
		return;
	}

	StackTraceParam param;

	param.jni = jni;
	param.jvmti = jvmti;
	param.thread = thread;
	param.dumpType = DUMP_KIND_THROWABLE;  //for all exception(Throwable)
	param.exceptionCls = exceptionCls;
	param.utf8ExceptionName =  exceptionClassName;
	param.exceptionName = exceptionClassName;
	param.monitorClassName = "java.lang.Throwable";

	param.exceptionObj = AgentCallbackHandler::currentException;
    param.monitorObj = getWaitMonitor(jvmti, jni, param.thread);//add by Qiu Song on 20090819 for モニターオブジェクトダンプ
    param.useMonitorThread = NULL;
	//add by Qiu Song on 20090924 for 自動監視機能のパッケージ名出力
    param.packageName = catchClassSig;
    //end of add by Qiu Song on 20090924 for 自動監視機能のパッケージ名出力


	//important!! to set the current output setting
	Global::getConfig()->setOutputSetting(param.monitorClassName);

	param.dumpInstance = Global::getConfig()->getOutputSetting()->getDumpInstance();
	param.dumpCount = 0;

	string filePath = dumpStackForException(&param,jni);

	//send mail
   if(!filePath.empty() && param.dumpCount > 0) 
	    sendMailForException(jni,param.exceptionName,(char*)filePath.c_str());

    return ;
}

//モニターを取得する関数
jobject getWaitMonitor(jvmtiEnv* jvmti, JNIEnv* jni, jthread thread)
{
	jobject contendMonitorObject = NULL;
	jvmtiError err = jvmti->GetCurrentContendedMonitor(thread, &contendMonitorObject);
	if(err != JVMTI_ERROR_NONE)
	{
		Global::logger->logError("failed to get contended monitor,err cd =%d",err);
		return NULL;
	}
	if(contendMonitorObject != NULL)
	{
		return contendMonitorObject; 
	}
	return NULL;
}

//モニター所有スレッド名の取得
char* getMonitorUseThreadName(jvmtiEnv* jvmti, JNIEnv* jni, jthread* objectThreads, 
							  int threadCount, jobject monitorObj)
{
	if(objectThreads == NULL || threadCount == 0 || monitorObj == NULL)
	{
		return NULL;
	}
    for(int i = 0; i < threadCount; i++)
	{
		if(isCurThreadOwenTheObject(jvmti, jni, objectThreads[i], monitorObj) == true)
		{
			jvmtiThreadInfo threadInfo;
			jvmtiError err = jvmti->GetThreadInfo(objectThreads[i], &threadInfo);
			if(err != JVMTI_ERROR_NONE)
			{
				Global::logger->logError("failed to get thread info ,err cd =%d",err);
				continue;
			}
			char* useThreadName = HelperFunc::utf8_to_native(threadInfo.name);
			return useThreadName;
		}
	}
	return NULL;
}

//該当オブジェクトはスレッド所有かどうか判断する
bool isCurThreadOwenTheObject(jvmtiEnv* jvmti, JNIEnv* jni, jthread curThread, jobject monitorObj)
{
	if(curThread == NULL || monitorObj == NULL)
	{
		return false;
	}
	jint monitorObjCount = 0;
	jobject* monitorObjList = NULL;
    jvmtiError err = jvmti->GetOwnedMonitorInfo(curThread, &monitorObjCount, &monitorObjList);
	if(err != JVMTI_ERROR_NONE)
	{
		Global::logger->logError("failed to get owned monitor info ,err cd =%d",err);
		return false;
	}
	
	for(int i = 0; i < monitorObjCount; i++)
	{
		if(jni->IsSameObject(monitorObj, monitorObjList[i]))
		{
			return true;
		}
	}
	return false;
}