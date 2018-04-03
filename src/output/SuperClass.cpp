#include "SuperClass.h"
#include "../common/Config.h"
#include "../common/Global.h"
#include "../common/HelperFunc.h"
#include "../common/JvmtiAutoRelease.h"
#include "../common/JniLocalRefAutoRelease.h"
#include "Attribute.h"

USE_NS(NS_OUTPUT)
USE_NS(NS_COMMON)


const char *SuperClass::ATTR_SIG = "Signature";
const char *SuperClass::TAG_SUPER_CLASS = "SuperClass";
const char *SuperClass::OBJECT_CLASS_SIG = "Ljava/lang/Object;";


SuperClass::SuperClass(SuperClassParam *param)
{
	this->param = param;
	init();
}

SuperClass::~SuperClass()
{
}

void SuperClass::init()
{
	char *sig = NULL;
	param->jvmti->GetClassSignature(param->clazz, &sig, NULL);
	AUTO_REL_JVMTI_OBJECT(sig);
	if (strcmp(sig, OBJECT_CLASS_SIG) == 0) 
		return;

	//get class name of super class
	char *javaClassName;
	javaClassName = HelperFunc::convertClassSig(sig);

	SuperClassData* superClassData = param->parent->getSuperClass();

	if(superClassData == NULL)
	{
		//create a new node
		superClassData = new SuperClassData();
		param->parent->setSuperClass(superClassData); 
		superClassData->setSignature(javaClassName);
		param->ancestor->addDumpDataSize(superClassData->getDumpSize());
	}

	if (param->clazz != NULL && param->object != NULL)
	{
		//add attribute
		AttributeParam paramAttr;
		paramAttr.jni = param->jni;
		paramAttr.jvmti = param->jvmti;
		paramAttr.ancestor = param->ancestor;
		paramAttr.thread = param->thread;
		paramAttr.parent = superClassData;
		paramAttr.clazz = param->clazz;
		paramAttr.object = param->object;
		paramAttr.pool = param->pool;
		paramAttr.className = javaClassName;
		Attribute attr(&paramAttr);
	    if(attr.getExpandedOnce())
		{
            superClassData->setExpandedOnce(true);
		}

		//add super class
		jclass superClazz = param->jni->GetSuperclass(param->clazz);
		AUTO_REL_JNI_LOCAL_REF(param->jni, superClazz);
		if (superClazz != NULL)
		{
			SuperClassParam paramSC;
			memcpy(&paramSC, param, sizeof(*param));
			paramSC.parent = superClassData;
			paramSC.clazz = superClazz;
			SuperClass superClass(&paramSC);
		}
		
	}

	delete javaClassName;
}
