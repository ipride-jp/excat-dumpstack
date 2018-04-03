#include <time.h>
#include "SimpleStackTrace.h"
#include "Global.h"
#include "HelperFunc.h"
#include "JvmtiAutoRelease.h"
#include "JniLocalRefAutoRelease.h"

USE_NS(NS_COMMON)

SimpleMethod::SimpleMethod(char *methodName, char *methodSig, 
						   char *classSig, jlocation location) 
						   : methodName(methodName), methodSig(methodSig),
						   classSig(classSig), location(location)

{
}

SimpleMethod::~SimpleMethod()
{
	if (methodName != NULL)
	{
		delete methodName;
		methodName = NULL;
	}

	if (methodSig != NULL)
	{
		delete methodSig;
		methodSig = NULL;
	}
		
	if (classSig != NULL)
	{
		delete classSig;
		classSig = NULL;
	}
}

int SimpleMethod::operator ==(SimpleMethod &rhs)
{
	return strcmp(rhs.methodName, methodName) == 0
		&& strcmp(rhs.methodSig, methodSig) == 0
		&& strcmp(rhs.classSig, classSig) == 0
		&& rhs.location == location;
}

int SimpleMethod::operator !=(SimpleMethod &rhs)
{
	return !(*this == rhs);
}

SimpleStackTrace::SimpleStackTrace(JNIEnv *jni, jvmtiEnv *jvmti, jvmtiFrameInfo *frameInfos, 
								   int pos, int count, jthread thread) : methods(NULL)
{
	::time(&this->time);

	this->thread = jni->NewGlobalRef(thread);
	this->jni = jni;
	this->jvmti = jvmti;
	
	string logBuf;

	//methodî•ñ‚Ìƒ_ƒ“ƒv
	for (jint index = count - 1; index >= pos; index--)
	{
		jvmtiFrameInfo *frameInfo = frameInfos + index;
		
		//get method name and signature
		jmethodID methodId = frameInfo->method;
		char *methodName = NULL, *methodSig = NULL;
		if (!HelperFunc::validateJvmtiError(jvmti, 
			jvmti->GetMethodName(methodId, &methodName, &methodSig, NULL), ""))
		{
			LOG4CXX_ERROR(Global::logger, "Can't get method name from jvmti");
			return;
		}
		AUTO_REL_JVMTI_OBJECT(methodName);
		AUTO_REL_JVMTI_OBJECT(methodSig);
		
		//get class signature
		jclass declaringClass = NULL;
		if (!HelperFunc::validateJvmtiError(jvmti,
			jvmti->GetMethodDeclaringClass(methodId, &declaringClass), ""))
		{
			logBuf = "Can't get decalaring class from jvmti for method: ";
			logBuf += methodName;
			LOG4CXX_ERROR(Global::logger, logBuf.c_str());
			return;
		}  
		AUTO_REL_JNI_LOCAL_REF(jni, declaringClass);
		
		char *classSig = NULL;
		if (!HelperFunc::validateJvmtiError(jvmti, 
			jvmti->GetClassSignature(declaringClass, &classSig, NULL), ""))
		{
			logBuf = "Can't get class signature from jvmti for method: ";
			logBuf += methodName;
			LOG4CXX_ERROR(Global::logger, logBuf.c_str());
			return;
		} 
		AUTO_REL_JVMTI_OBJECT(classSig);
		
		if (methods == NULL)
		{
			methods = new vector<SimpleMethod*>();
		}

		SimpleMethod *simpleMethod = new SimpleMethod(
			HelperFunc::strdup(methodName), 
			HelperFunc::strdup(methodSig), 
			HelperFunc::strdup(classSig), 
			frameInfo->location);
		methods->push_back(simpleMethod);
	}
}

SimpleStackTrace::~SimpleStackTrace()
{
	if (methods != NULL) 
	{
		vector<SimpleMethod*>::iterator it;
		for	(it = methods->begin(); it != methods->end(); it++)
		{
			delete *it;
		}

		methods->clear();

		delete methods;
		methods = NULL;
	}

	jni->DeleteGlobalRef(thread);
}

int SimpleStackTrace::operator ==(SimpleStackTrace& rhs) 
{
	if (!jni->IsSameObject(rhs.thread, thread)) 
	{
		if (Global::getConfig()->isDumpDuplicationWhenThreadDiff()) 
		{
			return false;
		}
	}

	if (this->methods->size() != rhs.methods->size())
	{
		return false;
	}

	vector<SimpleMethod*>::iterator itl, itr;
	for (itl = this->methods->begin(), itr = rhs.methods->begin();
		itl != this->methods->end() && itr != rhs.methods->end();
		itl++, itr++)
	{
		if (**itl != **itr)
		{
			return false;
		}
	}
		
	return true;
}