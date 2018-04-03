#include "TypedElement.h"
#include "../common/HelperFunc.h"
#include "../common/Define.h"
#include "../common/Global.h"
#include "../common/OutputSetting.h"
#include "../common/JvmtiAutoRelease.h"
#include "SuperClass.h"
#include "Attribute.h"

USE_NS(NS_OUTPUT)
USE_NS(NS_COMMON)

const char *TypedElement::ATTR_DEFINED_TYPE = "DefinedType";
const char *TypedElement::ATTR_NAME = "Name";
const char *TypedElement::ATTR_VALUE = "Value";
const char *TypedElement::ATTR_ARRINDEX = "Index";
const char *TypedElement::ATTR_VALID = "Valid";
const char *TypedElement::ATTR_OBJECT_REF = "ObjectRef";

const char *TypedElement::TAG_THIS = "This";
const char *TypedElement::TAG_VARIABLE = "Variable";
const char *TypedElement::TAG_ITEM = "Item";
const char *TypedElement::TAG_ATTR = "Attribute";
const char *TypedElement::TAG_ARGUMENT = "Argument";


TypedElement::TypedElement(TypedElementParam *param, const char *tagName)
{
	this->param = param;
	this->tagName = tagName;

    init();
}

TypedElement::~TypedElement()
{
}

void TypedElement::init()
{
	OutputSetting *setting = Global::getConfig()->getOutputSetting();
	if (strcmp(TAG_ARGUMENT, tagName) == 0)
	{
		//no need to output argument
		if (!setting->getArgument())
			return;
	}
	
	if (strcmp(TAG_ATTR, tagName) == 0)
	{
		//no need to output attribute
		if (!setting->getAttribute())
			return;
	}

	if (strcmp(TAG_VARIABLE, tagName) == 0)
	{
		//no need to output variable
		if (!setting->getVariable())
			return;
	}


	TypeElementData* typeElementData = new TypeElementData();
	//add this son to parent
	param->parent->addTypeElementData(typeElementData); 
	typeElementData->setUnsure(param->unsure);
	
	char *temp = NULL;
    
    typeElementData->setTagName(tagName); 

	//add attribute name
	typeElementData->setObjName(param->objName); 

	//add attribute defined type
	if(param->objDefinedType != NULL)
	{
		if(*(param->objDefinedType) == 'U')
		{
			typeElementData->setObjDefinedType("unknown");
		}else
		{
			temp = HelperFunc::convertClassSig(param->objDefinedType);
			typeElementData->setObjDefinedType(temp); 
			delete temp;
		}
	}

	//add attribute value
	if (param->objValue != NULL)
	{
		typeElementData->setValue(param->objValue);
	}else if (param->objectRef == -1 && param->valid )
    {
		typeElementData->setValue("null");
	}
		
	//add object ref
	if (param->objectRef != -1)
	{
		typeElementData->setObjectRef(param->objectRef);
	}

	//if it is item node, add index attribute
	if (strcmp(TAG_ITEM, tagName) == 0)
	{
		typeElementData->setIndex(param->arrIndex);
	}

	//add attribute valid
	if (strcmp(tagName, TAG_VARIABLE) == 0)
	{
		typeElementData->setValid(param->valid); 
	}

	//add access flag
    if (strcmp(tagName, TAG_ATTR) == 0)
	{
		typeElementData->setModifiers(param->modifiers); 
	}

    //check dump data size
    param->ancestor->addDumpDataSize(typeElementData->getDumpSize());
}






