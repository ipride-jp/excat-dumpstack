#if !defined(_ATTRIBUTE_H)
#define _ATTRIBUTE_H

#include <jni.h>
#include <jvmti.h>
#include "DumpParam.h"
#include "../common/Define.h"
#include "ObjectData.h"
#include "StackTrace.h"
#include "ObjectPool.h"


BEGIN_NS(NS_OUTPUT)
typedef struct AttributeParamTag
{
	JNIEnv *jni;
	jvmtiEnv *jvmti;
	StackTrace* ancestor;
	jthread thread;
	jobject object;
	jclass clazz;
	ObjectPool *pool;
	ObjectData *parent;
	char* className;
} AttributeParam;

class Attribute
{
public:
	Attribute(AttributeParam *param);
	virtual ~Attribute();
    bool getExpandedOnce(){return expandedOnce;};
private:
	void init();

private:
	AttributeParam *param;
    bool expandedOnce;
	/*
	static const char *ATTR_DEFINED_TYPE;
	static const char *ATTR_REAL_TYPE;
	static const char *ATTR_NAME;
	static const char *ATTR_VALUE;
	*/
	static const char *TAG_ATTR;		
};
END_NS

#endif  //_ATTRIBUTE_H
