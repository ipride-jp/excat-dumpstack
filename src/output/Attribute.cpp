#pragma warning(disable : 4503)
#pragma warning(disable : 4786)

#include "Attribute.h"
#include <map>
#include <vector>
#include <string>
#include "../common/Define.h"
#include "../jniutility/ExceptionTableMan.h"
#include "../common/Config.h"
#include "../common/Global.h"
#include "../common/HelperFunc.h"
#include "TypedElement.h"
#include "../common/JniLocalRefAutoRelease.h"
#include "../common/JvmtiAutoRelease.h"
#include "../common/DumpObject.h"

USE_NS(NS_OUTPUT)
USE_NS(NS_COMMON)

#ifndef XERCESC20
XERCES_CPP_NAMESPACE_USE
#endif

const char *Attribute::TAG_ATTR = "Attribute";

Attribute::Attribute(AttributeParam *param)
{
	this->param = param;

	expandedOnce = false;

	Global::curAttrDepth++;

	if (Global::curAttrDepth <= 
		Global::getConfig()->getOutputSetting()->getAttributeNestDepth()) 
	{
		init();
		expandedOnce = true;
	}

	Global::curAttrDepth--;
}

Attribute::~Attribute()
{
}

void Attribute::init()
{
	jint count;
	jfieldID *fieldIds = NULL;
	jclass clazz = param->clazz;
	jobject obj = param->object;
	param->jvmti->GetClassFields(clazz, &count, &fieldIds);
	AUTO_REL_JVMTI_OBJECT(fieldIds);

	OutputSetting *setting = Global::getConfig()->getOutputSetting();
	bool _public = setting->getPublic();
	bool _protected = setting->getProtected();
	bool _package = setting->getPackage();
	bool _private = setting->getPrivate();

	//dump template object
	DumpObject* dumpObject = Global::getConfig()->getDumpObject(param->className);

	for (int index = 0; index < count; index++)
	{
		char *objName = NULL;
		char *objDefinedType = NULL;
		char *objValue = NULL;

		int objectRef = -1;
		bool isObject = false;
		jobject fieldObject = NULL;
		jfieldID fieldId = *(fieldIds + index);

		char* fieldName = NULL;
		char* fieldType = NULL;
		jint modifier;
		param->jvmti->GetFieldName(clazz, fieldId, &fieldName, &fieldType, NULL);

		AUTO_REL_JVMTI_OBJECT(fieldName);
		AUTO_REL_JVMTI_OBJECT(fieldType);

		objName = fieldName;
		objDefinedType = fieldType;

        if(dumpObject != NULL && !dumpObject->shouldDump(fieldName))
		{
			continue;
		}

		param->jvmti->GetFieldModifiers(clazz, fieldId, &modifier);
		if ((!_public && (modifier & ACC_PUBLIC))
			|| (!_private && modifier & ACC_PRIVATE)
			|| (!_protected && modifier & ACC_PROTECTED)
			|| (!_package && (modifier & (ACC_PUBLIC | ACC_PROTECTED | ACC_PRIVATE)) == 0))
			continue;

		if(Global::logger->isTraceEnabled())
		{
			char* logBuf = new char[16 + Global::curAttrDepth * 2 + 
				strlen(fieldName) + 1];
			strcpy(logBuf,"dump attribute ");
			for(int i = 0;i < Global::curAttrDepth;i++)
			{
				strcat(logBuf,"\t");
			}
			strcat(logBuf,fieldName);
			LOG4CXX_TRACE(Global::logger, logBuf);
			delete logBuf;
		}

		char* buf = new char[MAX_BUF_LEN];
		memset(buf, 0, MAX_BUF_LEN);
		bool expandedOnce = param->parent->getExpandedOnce();

		if (strcmp(fieldType, "Z") == 0) //boolean
		{
			if(!expandedOnce)
			{

				jboolean fieldValue = (modifier & ACC_STATIC) == 0 
					? param->jni->GetBooleanField(obj, fieldId)
					: param->jni->GetStaticBooleanField(clazz, fieldId);
				objValue = (char*)(fieldValue == 0 ? "false" : "true");
			}
		}
		else if (strcmp(fieldType, "B") == 0)  //byte
		{
			if(!expandedOnce)
			{
				jbyte fieldValue = (modifier & ACC_STATIC) == 0 
				? param->jni->GetByteField(obj, fieldId)
				: param->jni->GetStaticByteField(clazz, fieldId);
				sprintf(buf, "%d", fieldValue);
				objValue = buf;
			}

		}
		else if (strcmp(fieldType, "C") == 0) //char
		{
			if(!expandedOnce)
			{
				jchar fieldValue = (modifier & ACC_STATIC) == 0 
					? param->jni->GetCharField(obj, fieldId)
					: param->jni->GetStaticCharField(clazz, fieldId);

				sprintf(buf, "%04x", fieldValue);
				objValue = buf;
			}

		}
		else if (strcmp(fieldType, "D") == 0) //double
		{
			if(!expandedOnce)
			{
				jdouble fieldValue = (modifier & ACC_STATIC) == 0 
					? param->jni->GetDoubleField(obj, fieldId)
					: param->jni->GetStaticDoubleField(clazz, fieldId);
				sprintf(buf, "%e", fieldValue);
				objValue = buf;
			}

		}
		else if (strcmp(fieldType, "F") == 0) //float
		{
			if(!expandedOnce)
			{
				jfloat fieldValue = (modifier & ACC_STATIC) == 0 
					? param->jni->GetFloatField(obj, fieldId)
					: param->jni->GetStaticFloatField(clazz, fieldId);
				sprintf(buf, "%e", fieldValue);
				objValue = buf;
			}

		}
		else if (strcmp(fieldType, "I") == 0) //int
		{
			if(!expandedOnce)
			{
				jint fieldValue = (modifier & ACC_STATIC) == 0 
					? param->jni->GetIntField(obj, fieldId)
					: param->jni->GetStaticIntField(clazz, fieldId);
				sprintf(buf, "%d", fieldValue);
				objValue = buf;
			}

		}
		else if (strcmp(fieldType, "S") == 0) //short
		{
			if(!expandedOnce)
			{
				jint fieldValue = (modifier & ACC_STATIC) == 0 
					? param->jni->GetShortField(obj, fieldId)
					: param->jni->GetStaticShortField(clazz, fieldId);
				sprintf(buf, "%d", fieldValue);
				objValue = buf;
			}

		}
		else if (strcmp(fieldType, "J") == 0) //long
		{
			if(!expandedOnce)
			{
				jlong fieldValue = (modifier & ACC_STATIC) == 0 
					? param->jni->GetLongField(obj, fieldId) 
					: param->jni->GetStaticLongField(clazz, fieldId);
				HelperFunc::getLongTypeValue(fieldValue,buf);
				objValue = buf;
			}

		}
		else //object
		{
			fieldObject = (modifier & ACC_STATIC) == 0 
				? param->jni->GetObjectField(obj, fieldId)
				: param->jni->GetStaticObjectField(clazz, fieldId);

			if (fieldObject == NULL)
			{	objValue = NULL;
			}
			else
			{
				AUTO_REL_JNI_LOCAL_REF(param->jni, fieldObject);
				objectRef = param->pool->getObjectId(fieldObject);
				ObjectData* objData = NULL;
				if(objectRef < 0)
				{
					objectRef = param->pool->addObject(fieldObject);
					objData = param->ancestor->getObjectData(objectRef);
					//ダンプするデータ量を増えた
					param->ancestor->addDumpDataSize(objData->getDumpSize());
				}else
                {
					objData = param->ancestor->getObjectData(objectRef);
				}

				//展開可能の場合、オブジェクトを展開する
				if(!objData->getExpanded())
				{
				    param->pool->expandObject(fieldObject,objData,param->ancestor,
						Global::curAttrDepth);
				}
			}
		}
        
		if(!expandedOnce)
		{
			TypedElementParam* paramTyped = new TypedElementParam();
			paramTyped->jni = param->jni;
			paramTyped->jvmti = param->jvmti;
			paramTyped->ancestor = param->ancestor;
			paramTyped->objDefinedType = objDefinedType;
			paramTyped->object = fieldObject;
			paramTyped->objName = objName;
			paramTyped->objValue = objValue;
			paramTyped->parent = param->parent;
			paramTyped->pool = param->pool;
			paramTyped->objectRef = objectRef;
			paramTyped->arrIndex = -1;
			paramTyped->valid=true;
			paramTyped->modifiers = modifier;
			TypedElement typedElem(paramTyped, this->TAG_ATTR);
			delete paramTyped;
		}
		delete[] buf;
	}
}
