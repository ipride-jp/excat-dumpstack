#if !defined(_JNILOCALREFAUTORELEASE_H)
#define _JNILOCALREFAUTORELEASE_H

#include <jni.h>
#include "Define.h"

BEGIN_NS(NS_COMMON)
class JniLocalRefAutoRelease
{
public:
	JniLocalRefAutoRelease(JNIEnv *jni, jobject localRef);
public:
	virtual ~JniLocalRefAutoRelease();

private:
	jobject localRef;
	JNIEnv *jni;
};
END_NS

#endif

