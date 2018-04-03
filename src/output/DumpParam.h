#ifndef DUMP_PARM_H
#define DUMP_PARM_H
#include <xercesc/dom/DOM.hpp>
#include <jni.h>
#include <jvmti.h>
#include "../common/Define.h"

#ifndef XERCESC20
XERCES_CPP_NAMESPACE_USE
#endif

BEGIN_NS(NS_OUTPUT)
typedef struct DumpParamTag
{
	JNIEnv *jni;
	jvmtiEnv *jvmti;
	jthread thread;
	DOMDocument *doc;
	DOMElement *parent;
	jint depth;
	jvmtiFrameInfo *frameInfo;
} DumpParam;
END_NS

#endif

