#pragma warning(disable : 4503)
#pragma warning(disable : 4786)

#include "CpInfo.h"
#include <map>
#include <vector>
#include <string>
#include "../common/Define.h"
#include "../jniutility/ExceptionTableMan.h"
#include "ClassFile.h"
#include "ClassFileConstant.h"


USE_NS(NS_CLASS_FILE_OPERATION)

CpInfo::CpInfo(U1 *pos)
{
	this->largeNumericType = false;
	this->tag = pos;
	pos += sizeof(*this->tag);
	this->info = pos;

	switch(*this->tag) 
	{
    case JVM_CONSTANT_Class: 
		pos += 2;
        break;
	case 15:
		pos += 3;
		break;
    case JVM_CONSTANT_String:  
	case 16:
		pos += 2;
        break;
    case JVM_CONSTANT_Fieldref: 
    case JVM_CONSTANT_Methodref: 
    case JVM_CONSTANT_InterfaceMethodref: 
    case JVM_CONSTANT_Integer:  
    case JVM_CONSTANT_Float:  
    case JVM_CONSTANT_NameAndType: 
	case 18:
		pos += 4;
        break;
    case JVM_CONSTANT_Long:  
    case JVM_CONSTANT_Double: 
		pos += 8;
		this->largeNumericType = true;
        break;
    case JVM_CONSTANT_Utf8:
		{
			U2 len = ClassFile::getU2ReverseValue((U2*)pos);
			pos += 2;
			pos += len;
			break;
		}
    default:
        break;
	}

	size = pos - this->tag;
}

void CpInfo::getUtf8Info(char *buf)
{
	int len = ClassFile::getU2ReverseValue((U2*)info);
	//add by Qiu Song on 20090918 for ’·‚·‚¬‚¾Utf8‚Ì‘Î‰ž
	if(buf == NULL)
	{
		buf = new char[len + 1];
		memset(buf, 0, len + 1);
	}
	//end of add by Qiu Song on 20090918 for ’·‚·‚¬‚¾Utf8‚Ì‘Î‰ž
	memcpy(buf, info + sizeof(U2), len);
	buf[len] = '\0';
}

void CpInfo::getUtf8InfoNeedMemory(char **buf)
{
	int len = ClassFile::getU2ReverseValue((U2*)info);
    *buf = new char[len + 1];
    memcpy(*buf, info + sizeof(U2), len);
    (*buf)[len] = '\0';
}