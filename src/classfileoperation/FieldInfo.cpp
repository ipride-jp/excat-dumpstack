#pragma warning(disable : 4503)
#pragma warning(disable : 4786)

#include "FieldInfo.h"
#include <map>
#include <vector>
#include <string>
#include "../common/Define.h"
#include "../jniutility/ExceptionTableMan.h"
#include "ClassFile.h"
#include "AttributeInfo.h"

USE_NS(NS_CLASS_FILE_OPERATION)

FieldInfo::FieldInfo(U1 *pos)
{
	this->accessFlags = (U2*)pos;
	pos += sizeof(*this->accessFlags);
	
	this->nameIndex = (U2*)pos;
	pos += sizeof(*this->nameIndex);

	this->descriptorIndex = (U2*)pos;
	pos += sizeof(*this->descriptorIndex);

	this->attributesCount = (U2*)pos;
	pos += sizeof(*this->attributesCount);

	U2 attrCount = ClassFile::getU2ReverseValue(this->attributesCount);
	if (attrCount > 0)
	{
		this->attributes = new AttributeInfoPtr[attrCount];
		for (int index = 0; index < attrCount; index++)
		{
			AttributeInfo *ai = new AttributeInfo(pos);
			pos += ai->getSize();
			this->attributes[index] = ai;
		}
	}
	else
		this->attributes = NULL;

	this->size = pos - (U1*)this->accessFlags;
}

FieldInfo::~FieldInfo()
{
	//delete attributes
	U2 attrCount = ClassFile::getU2ReverseValue(this->attributesCount);
	for (int index = 0; index < attrCount; index++)
	{
		delete *(this->attributes + index);
	}
	if (this->attributes != NULL) 
	{
		delete[] this->attributes;
		this->attributes = NULL;
	}
}
