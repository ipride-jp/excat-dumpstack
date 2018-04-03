#include "JniLocalRefAutoRelease.h"

USE_NS(NS_COMMON)

JniLocalRefAutoRelease::JniLocalRefAutoRelease(JNIEnv *jni, jobject localRef)
{
	this->jni = jni;
	this->localRef = localRef;
}

JniLocalRefAutoRelease::~JniLocalRefAutoRelease()
{
	if (jni != NULL && localRef != NULL)
		jni->DeleteLocalRef(localRef);
}
