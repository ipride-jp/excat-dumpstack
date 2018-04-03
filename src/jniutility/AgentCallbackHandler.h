#pragma warning(disable : 4503)
#pragma warning(disable : 4786) //警告C4786を禁止 
#if !defined(_AGENTCALLBACKHANDLER_H)
#define _AGENTCALLBACKHANDLER_H

#include <jni.h>
#include <jvmti.h>
#include <vector>

#include "../common/Config.h"
#include "../common/MonitoringMethod.h"

USE_NS(NS_COMMON)

BEGIN_NS(NS_JNI_UTILITY)

class ExceptionTableMan;

class AgentCallbackHandler
{
private:
	AgentCallbackHandler(void) {};
	virtual ~AgentCallbackHandler(void) {};
	static bool setMonitorClass(JNIEnv* jni,Config* pConfig);
	static void setMailSetting(JNIEnv* jni);
    static void redefineClass(jvmtiEnv *jvmti,JNIEnv* jni, string name, string classLoadString);
	static void redefineClasses(jvmtiEnv *jvmti,JNIEnv* jni, vector<MonitoringClass*>* oldDef);
	static void redefineClasses(jvmtiEnv *jvmti,JNIEnv* jni, MonitoringClass* oldDef);
	static void redefineClassesNew(jvmtiEnv *jvmti,JNIEnv* jni, MonitoringClass* newDef);
    static void redefineClass(jvmtiEnv *jvmti,JNIEnv* jni, jobject cl, const char* name);
    static void addMethodDumpInfo(MonitoringMethod* method, string classUrl);
	static void delDumpFile(const char* delTaskFilePath);
    static void doOldTaskOfDelDumpFile(const char* delTaskFilePath, char* pCurrentTime);
	static bool isValidTaskFileName(const char* fileName);
private:
	static jobject lastException;
	static jthread lastThread;//add by Qiu Song on 20090811 for 重複ダンプ防止
public:
    static jobject currentException;//add by Qiu Song on 20090811 for 例外オブジェクトダンプ
public:
    static int WM_Init_Is_Ok;
	static int WM_Death_Started;
	static jclass callbacksclass;
	static jmethodID getClassUrlMethodId;
    static map<jint, jobject>* loaders;
    static map<string, short>* loadersSig;
    static vector<string>* classToRedefine;
	//key: classUrl
	static map<string, MonitoringMethodInfo*>* methodDumpInfo;
	//key: className + '#' + classLoadString
	static map<string, MonitoringClassLoadInfo*>* loadedClassInfo;
	//key: className + '#' + classUrl
	static map<string, vector<jweak>*>* instanceMap;

	static class ExceptionTableMan* exceptionTableMan;

	//add by Qiu Song on 20090907
	//除外例外クラスリスト
	static vector<string>* excludeExceptionList;
	//end of add by Qiu Song on 20090907

	//add by Qiu Song on 20090924 for 例外オブジェクト重複ダンプ防止
	static map<jthread, jobject>* dumpedExceptionObjList;
	//end of add by Qiu Song on 20090924 for 例外オブジェクト重複ダンプ防止
public:
    static void JNICALL classFileLoadHook(jvmtiEnv *jvmti, JNIEnv* jni, 
			jclass classBeingRedefined, jobject loader, const char* name, 
			jobject protectionDomain, jint classDataLen, const unsigned char* classData,
			jint* newClassDataLen, unsigned char** newClassData);

    static void JNICALL exceptionEvent(jvmtiEnv *jvmti, JNIEnv* jni, 
	    jthread thread, jmethodID method, jlocation location, jobject exception, 
        jmethodID catch_method, jlocation catch_location);

	static void JNICALL vmInitCallBack(jvmtiEnv *jvmti, JNIEnv* jni,jthread thread);

	static void JNICALL vmDeathCallBack(jvmtiEnv *jvmti, JNIEnv* jni);

	static void JNICALL dataDumpRequest(jvmtiEnv *jvmti);

	static void JNICALL monitorThread(jvmtiEnv* jvmti_env, JNIEnv* jni_env, void* arg);

	static void JNICALL dumpFileDelThread(jvmtiEnv* jvmti_env, JNIEnv* jni_env, void* arg);

	static MethodDumpInfo* getMethodDumpInfo(string classUrl, char* methodName, char* methodSig);

	//static bool haveRedefined(string& classUrl, const char* classLoadString);

	static void JNICALL threadEnd(jvmtiEnv* jvmti, JNIEnv* jni, jthread thread);

	static vector<jclass>* FindClass(JNIEnv* jni, char* className);

	static void removeInstance(JNIEnv* jni, char* className, char* classLoadString);

	static bool isClassLoaderClass(char* className);

	//add by Qiu Song on 20090811 for 重複ダンプ防止
	static bool isSameExceptionObject(JNIEnv* jni, jthread thread, jobject exception);
	static void getExcludeExceptionList();
	//指定ファイルから内容を呼び出し、文字列に保存する
	static bool getExcludeExceptionListFromFile(char* fileName);
	//除外例外クラスかどうか判断する
	static bool isExcludeException(char* exceptionClassName);
	//end of add by Qiu Song on 20091006 for モニター所有スレッド名の取得
};
END_NS

#endif  //_AGENTCALLBACKHANDLER_H


