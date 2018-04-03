#include "ThisObject.h"
#include "../common/HelperFunc.h"
#include "../common/Define.h"

USE_NS(NS_OUTPUT)
USE_NS(NS_COMMON)

const char *ThisObject::TAG_THIS = "This";
const char *ThisObject::ATTR_DEFINED_TYPE = "DefinedType";
const char *ThisObject::ATTR_REAL_TYPE = "RealType";

ThisObject::ThisObject(DumpParam *param)
{
	this->param = param;

	init();
}

ThisObject::~ThisObject()
{

}

void ThisObject::init()
{
/*
	DOMDocument *doc = param->parent->getOwnerDocument();

	XMLCh tempStr[MAX_BUF_LEN];
	XMLString::transcode(TAG_THIS, tempStr, MAX_BUF_LEN - 1);
	DOMElement *thisNode = doc->createElement(tempStr);

	//add attribute defined type
	char *sig = param->table->signature;
	HelperFunc::addAttrToDOMElement(thisNode, ATTR_DEFINED_TYPE, sig);

	//add attribute real type
	jobject obj;
	param->jvmti->GetLocalObject(param->thread, param->depth, 
		param->table->slot, &obj);
	jclass clazz = param->jni->GetObjectClass(obj);
	char *realSig;
	param->jvmti->GetClassSignature(clazz, &realSig, NULL);
	HelperFunc::addAttrToDOMElement(thisNode, ATTR_REAL_TYPE, realSig);

	//append the node
	param->parent->appendChild(thisNode);

	jint fieldCount;
	jfieldID *fields;
	param->jvmti->GetClassFields(clazz, &fieldCount, &fields);
	for (int index = 0; index < fieldCount; index++) 
	{
		param->fieldId = *(fields + index);
		param->clazz = clazz;
		Attribute attribute(param);
	}
*/
}
