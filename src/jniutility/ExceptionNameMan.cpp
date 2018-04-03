#pragma warning(disable : 4503)
#pragma warning(disable : 4786)

// ExceptionNameMan.cpp: ExceptionNameMan クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////
#include <jvmti.h>
#include "../common/Define.h"

#include "../common/Global.h"
#include "../common/Logger.h"
#include "../common/HelperFunc.h"
#include "../common/JvmtiAutoRelease.h"

#include <map>
#include <vector>
#include <string>

#include "../common/ObjectAutoRelease.h"
#include "../common/JniLocalRefAutoRelease.h"

#include "ExceptionNameMan.h"
#include "ExceptionTableMan.h"



USE_NS(NS_COMMON)
USE_NS(NS_JNI_UTILITY)
//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

ExceptionNameMan::ExceptionNameMan(jvmtiEnv * _jvmti,JNIEnv* _jni,jthread _thread,
								   jclass _exceptionClazz,const char* _exceptionClassSig,
								   const char* _newExceptionClassSig,jlocation _location)
								   :jvmti(_jvmti),jni(_jni),thread(_thread),exceptionClazz(_exceptionClazz),
								   exceptionClassSig(_exceptionClassSig),hasError(false),location(_location),
								   pExceptionTableMan(NULL),newExceptionClassSig(_newExceptionClassSig),
								   reachMonitorPackage(false)
{
    names = new vector<string*>();

	//まず自身の名前を入れる
	string* exceptionName = new string(exceptionClassSig);
    names->push_back(exceptionName);
    
	if(strcmp(exceptionClassSig,"Ljava/lang/Throwable;") == 0)
		return;

	jclass superClazz = jni->GetSuperclass(exceptionClazz);

	while(superClazz != NULL)
	{
 		char *superClassSig = NULL;
	    jvmtiError err = jvmti->GetClassSignature(superClazz, &superClassSig, NULL);
	    if(err != JVMTI_ERROR_NONE)
		{
	        break;
		}
		
		names->push_back(new string(superClassSig));

		if(strcmp(superClassSig,"Ljava/lang/Throwable;") == 0)
		{
            jvmti->Deallocate((unsigned char *)superClassSig);
			break;
		}else
		{
			jvmti->Deallocate((unsigned char *)superClassSig);
			
			jclass exceptionNow = superClazz;
			superClazz = jni->GetSuperclass(exceptionNow);
		}
	}
}

ExceptionNameMan::~ExceptionNameMan()
{
	 vector<string*>::iterator it;
	 for(it = names->begin(); it != names->end(); it++ )
	 {
		delete *it;
     }  
	 names->clear();
	 delete names;
}


//該当Exceptionの情報をダンプすべきかどうかを判断する。
bool ExceptionNameMan::shouldDump(char* catchClassSig)
{
    //1.除外判断（名前のみ）
	/*bool ret = Global::getConfig()->isExcludeClassForAllException(newExceptionClassSig);
    if(ret){
		return false;
	}*/
   
	//2.該当Exceptionをcatchするメソッドを確定する
	reachMonitorPackage = false;
    jmethodID catchMethod = seachCatchMethod(catchClassSig);
	if(hasError){
		return false;
	}
	if(catchMethod == NULL){
		//return true;
		return false;
	}
	//監視対象パッケージまで到達したが、キャッチされない例外をダンプする
	if(reachMonitorPackage)
	{
		return true;
	}

	//3.catchメソッドが指定したパッケージにあるかどうかを判断する
	jclass declaringClass = NULL;
	jvmtiError err = jvmti->GetMethodDeclaringClass(catchMethod, &declaringClass);
	if (err != JVMTI_ERROR_NONE){
		hasError = true;
		return false;
	}
	AUTO_REL_JNI_LOCAL_REF(jni, declaringClass);

	char *classSig = NULL;
	err = jvmti->GetClassSignature(declaringClass, &classSig, NULL);
	if (err != JVMTI_ERROR_NONE){
        hasError = true;
		return false;
	}
	AUTO_REL_JVMTI_OBJECT(classSig);

	//Javaフォーマットに変換
	char *newClassSig = HelperFunc::convertClassSig(classSig);  
	AUTO_REL_OBJECT(newClassSig);

	//監視対象パッケージであるかどうかを判断
	bool ret = Global::getConfig()->isMonitorPackageForAllException(newClassSig);
	if(ret){
		Global::logger->logInfo("Exception %s is caught by class %s",
			exceptionClassSig,newClassSig);
		strcpy(catchClassSig ,newClassSig);
	}
	return ret;
}


//catch methodの検索
jmethodID ExceptionNameMan::seachCatchMethod(char* catchClassSig)
{
    //1.すべてのフレームを取得

	//get frame count
	jint frameCount = 0;
	jvmtiError err  = jvmti->GetFrameCount(thread,&frameCount);
    if(err != JVMTI_ERROR_NONE)
	{
		hasError = true;
		Global::logger->logError("failed to call GetFrameCount in ExceptionNameMan::seachCatchMethod,error cd =%d.",err);
		return NULL;
	}

	jvmtiFrameInfo *frameInfos = new jvmtiFrameInfo[frameCount];
	AUTO_REL_OBJECT_ARR(frameInfos);
     
	//get all stack frames
	jint count;
 	err = jvmti->GetStackTrace(thread, 0, frameCount, frameInfos, &count);
    if(err != JVMTI_ERROR_NONE)
	{
		hasError = true;
		Global::logger->logError("failed to call GetStackTrace in StackTrace::init,error cd =%d.",err);
		return NULL;
	}

	//2.該当メソッドがcatchするException名は、namesと一致するかどうかをチェック
	for (jint index = 0; index < frameCount; index++)
	{
		jvmtiFrameInfo *frameInfo = frameInfos + index;
		jmethodID methodId = frameInfo->method;
		if(IsCathcMethodOrReachTarget(methodId,frameInfo->location, catchClassSig))
		{
		    if(hasError){
			    return NULL;
			}
            return methodId;
		}
	}
    
	return NULL;
}

//Catchメソッドであるかあるいは、監視対象パッケージまで到達したかどうか判断
bool ExceptionNameMan::IsCathcMethodOrReachTarget(jmethodID methodId,jlocation _location, char* catchClassSig)
{
	char *methodName = NULL, *methodSig = NULL;
	jvmtiError err;

	err = jvmti->GetMethodName(methodId, &methodName, &methodSig, NULL);
	if (err != JVMTI_ERROR_NONE){
        hasError = true;
		return false;
	}
	AUTO_REL_JVMTI_OBJECT(methodName);
	AUTO_REL_JVMTI_OBJECT(methodSig);

	jclass declaringClass = NULL;
	err = jvmti->GetMethodDeclaringClass(methodId, &declaringClass);
	if (err != JVMTI_ERROR_NONE){
		hasError = true;
		return false;
	}
	AUTO_REL_JNI_LOCAL_REF(jni, declaringClass);

	char *classSig = NULL;
	err = jvmti->GetClassSignature(declaringClass, &classSig, NULL);
	if (err != JVMTI_ERROR_NONE){
        hasError = true;
		return false;
	}
	AUTO_REL_JVMTI_OBJECT(classSig);

	//Javaフォーマットに変換
	char *newClassSig = HelperFunc::convertClassSig(classSig);  
	AUTO_REL_OBJECT(newClassSig);

	//監視対象パッケージであるかどうかを判断
	bool ret = Global::getConfig()->isMonitorPackageForAllException(newClassSig);
	if(ret){
		reachMonitorPackage = true;
		Global::logger->logInfo("Exception %s is not caught until class %s",
			exceptionClassSig,newClassSig);
		strcpy(catchClassSig, newClassSig);
		return true;
	}

	if(pExceptionTableMan->catchAndHandleException(classSig,methodName,methodSig,(int)_location,names)){
       
		Global::logger->logDebug("Exception %s is caught by class %s,at method:%s,loction=%d",
			exceptionClassSig,classSig,methodName,_location);
		strcpy(catchClassSig, classSig);
		return true;
	}

	return false;
}