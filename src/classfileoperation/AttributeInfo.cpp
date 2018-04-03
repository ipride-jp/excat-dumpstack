#pragma warning(disable : 4503)
#pragma warning(disable : 4786)

#include "AttributeInfo.h"
#include <map>
#include <vector>
#include <string>
#include "../common/Define.h"
#include "../jniutility/ExceptionTableMan.h"
#include "ClassFile.h"


USE_NS(NS_CLASS_FILE_OPERATION)

AttributeInfo::AttributeInfo(U1 *pos) 
{      
	this->attributeNameIndex = (U2*)pos;
	pos += sizeof(*this->attributeNameIndex);
	
        this->attributeLength = (U4*)pos;
	pos += sizeof(*this->attributeLength);

	this->info = pos;

	pos += ClassFile::getU4ReverseValue(this->attributeLength) * sizeof(U1);
	size = pos - (U1*)this->attributeNameIndex;
}

