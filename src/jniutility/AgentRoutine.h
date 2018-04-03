#if !defined(_AGENTROUTINE_H)
#define _AGENTROUTINE_H

#include <jni.h>
#include <jvmti.h>
#include "../output/StackTrace.h"

#ifdef __cplusplus
extern "C" {
#endif


JNIEXPORT jstring JNICALL Java_Callbacks_dumpstack
  (JNIEnv *, jclass, jobject, jclass, jstring,jstring,jobject);

JNIEXPORT jstring JNICALL Java_Callbacks_dumpstackForMethod
  (JNIEnv *, jclass, jobject, jstring);


JNIEXPORT void JNICALL Java_Callbacks_registerInstance
  (JNIEnv *, jclass, jobject, jobject, jstring, jstring);

JNIEXPORT void JNICALL Java_Callbacks_errorLog
  (JNIEnv *, jclass, jstring);

bool doesFileExist(const char* filePath);
bool readConfigFile(char* options,char* installPath);
bool checkLicenseFile(char* installPath);
bool addBootClassPath(jvmtiEnv* jvmti,char* installPath);
void checkMagicfile(char* installPath);
void dumpstackStackOverFlowError(JNIEnv*, jthread , jclass );
void JNICALL dumpAllThreadsForSignal(jvmtiEnv* jvmti, JNIEnv* jni, void* arg);
void  activateAgentThread();
void  sendMailForException(JNIEnv *jni,char* exceptionName,char* dumpFilePath);
void  sendMailMethod(JNIEnv *jni,char* dumpFilePath);
void JNICALL dumpAllThreadsForCondition(jvmtiEnv* jvmti, JNIEnv* jni, 
		         jthread currentThread,const char* dumpDir,int dumpKind,StackTraceParam* sparam);

void dumpStackOneThreadForSignal(StackTraceParam* sparam,JNIEnv *jni,
								 const char* dumpDir);

void dumpStackOneThreadForCondition(StackTraceParam* sparam,JNIEnv *jni,
						   const char* dumpDir,bool currentThreadFlag);

//Throwableを監視する時にダンプ
void  dumpstackForAny(JNIEnv *jni, jobject thread, jclass exceptionCls, char*exceptionClassName, char* catchClassSig);

//モニターを取得する関数
jobject getWaitMonitor(jvmtiEnv* jvmti, JNIEnv* jni, jthread thread);

//add by Qiu Song on 20091006 for モニター所有スレッド名の取得
char* getMonitorUseThreadName(jvmtiEnv* jvmti, JNIEnv* jni, jthread* threadList, int threadCount, jobject monitorObj);
bool isCurThreadOwenTheObject(jvmtiEnv* jvmti, JNIEnv* jni, jthread curThread, jobject monitorObj);
//end of add by Qiu Song on 20091006 for モニター所有スレッド名の取得
#ifdef __cplusplus
}
#endif

#endif  //_AGENTROUTINE_H

