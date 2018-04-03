#if !defined(_SUPERCLASS_H)
#define _SUPERCLASS_H

#include <jni.h>
#include <jvmti.h>
#include "DumpParam.h"
#include "ObjectData.h"
#include "ObjectPool.h"
#include "StackTrace.h"


BEGIN_NS(NS_OUTPUT)
typedef struct SuperClassParamTag
{
	JNIEnv *jni;
	jvmtiEnv *jvmti;
	StackTrace* ancestor;
	jthread thread;
	jobject object;
	jclass clazz;
	ObjectData *parent;
	ObjectPool *pool;
} SuperClassParam;

class SuperClass
{
public:
	SuperClass(SuperClassParam *param);
	virtual ~SuperClass();

private:
	void init();

private:
	SuperClassParam *param;
	static const char *TAG_SUPER_CLASS;
	static const char *ATTR_SIG;
	static const char *OBJECT_CLASS_SIG;
};
END_NS

#endif  //_SUPERCLASS_H
