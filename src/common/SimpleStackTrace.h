#if !defined(_SIMPLE_STACK_TRACE_H)
#define _SIMPLE_STACK_TRACE_H

#include "Define.h"
#include "jvmti.h"
#include "jni.h"
#include <vector>

using namespace std;
BEGIN_NS(NS_COMMON)

class SimpleMethod
{
public:
	SimpleMethod(char *methodName, char *methodSig, char *classSig, jlocation location);
	~SimpleMethod();
	int operator ==(SimpleMethod &rhs);
	int operator !=(SimpleMethod &rhs);

private:
	char *methodName;
	char *methodSig;
	char *classSig;
	jlocation location;
};

class SimpleStackTrace
{
public:
	SimpleStackTrace(JNIEnv *jni, jvmtiEnv * jvmti, jvmtiFrameInfo *frameInfos, int pos, 
		int count, jthread thread);
	~SimpleStackTrace();
	
	int operator ==(SimpleStackTrace &rhs);
	
	time_t time;
	jthread thread;
	
private:
	JNIEnv *jni;
	jvmtiEnv *jvmti;
	vector<SimpleMethod*> *methods;
};

END_NS

#endif 

