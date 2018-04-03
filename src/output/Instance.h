#if !defined(_INSTANCE_H)
#define _INSTANCE_H

#include "jni.h"
#include "jvmti.h"
#include "ObjectPool.h"
#include "ObjectData.h"
#include "StackTrace.h"

BEGIN_NS(NS_OUTPUT)

typedef struct InstanceParamTag
{
	JNIEnv *jni;
	jvmtiEnv *jvmti;
	jthread thread;
	StackTrace *parent;
	ObjectPool *pool;
	string   className;
	string   classUrl;
	int     maxInstanceCount;
	vector<jweak>* objRefVec;
	InstanceData* instanceData;
} InstanceParam;

class Instance
{
	private:
		InstanceParam *param;

	public:
		Instance(InstanceParam *param);
		virtual ~Instance();

	private:
		void init();
};
END_NS

#endif  //_INSTANCE_H
