#pragma warning(disable : 4503)
#pragma warning(disable : 4786)

#include <map>
#include <vector>
#include <string>
#include "../common/Define.h"
#include "../jniutility/ExceptionTableMan.h"
#include "MethodInfo.h"

#include "ClassFile.h"
#include "AttributeInfo.h"


USE_NS(NS_CLASS_FILE_OPERATION)

MethodInfo::MethodInfo(U1 *pos,ClassFile* pClassFile):exceptionTableNum(0),
   exceptions(NULL),methodName(NULL),methodSignature(NULL)
{
	this->accessFlags = (U2*)pos;
	pos += sizeof(*this->accessFlags);
	
	this->nameIndex = (U2*)pos;
    //get method name
	this->nameIndex = (U2*)pos;
	pClassFile->getUtf8ConstantInfoNeedMemory(ClassFile::getU2ReverseValue((U2*)pos),&methodName);

	pos += sizeof(*this->nameIndex);

	//get method signature
	this->descriptorIndex = (U2*)pos;
    pClassFile->getUtf8ConstantInfoNeedMemory(ClassFile::getU2ReverseValue((U2*)pos),&methodSignature);

	this->descriptorIndex = (U2*)pos;
	pos += sizeof(*this->descriptorIndex);

	this->attributesCount = (U2*)pos;
	pos += sizeof(*this->attributesCount);

	U2 attrCount = ClassFile::getU2ReverseValue(this->attributesCount);
	if (attrCount > 0)
	{
		this->attributes = new AttributeInfoPtr[attrCount];
		char buf[MAX_BUF_LEN];
		for (int index = 0; index < attrCount; index++)
		{
			U1* beginPos = pos;
			AttributeInfo *ai = new AttributeInfo(pos);

			pos += ai->getSize();
			this->attributes[index] = ai;

            //find the attribute with the name of "Code"
			pClassFile->getUtf8ConstantInfo(
			    ClassFile::getU2ReverseValue(ai->attributeNameIndex), buf);
			if (strcmp(buf, "Code") == 0)
			{
				//get exception table length
                getExceptionInfo(beginPos,pClassFile);

				//登録
				class ExceptionTableMan* pExceptionTableMan = pClassFile->getExceptionTableMan();
				//メソッド監視の時もここに入る
				if(pExceptionTableMan != NULL){
				    pExceptionTableMan->registerExceptionTable((int)exceptionTableNum,exceptions,
					    pClassFile->getClassNameOFMyself(),methodName,methodSignature);
				}
			}

		}
	}
	else
		this->attributes = NULL;

	this->size = pos - (U1*)this->accessFlags;
}

MethodInfo::~MethodInfo()
{
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

	if(exceptionTableNum > 0)
	{
		for (int index = 0; index < exceptionTableNum; index++)
		{
			delete *(this->exceptions + index);
		}
		if (this->exceptions != NULL)
		{
			delete[] this->exceptions;
			this->exceptions = NULL;
		}
	}

	if(methodSignature != NULL)
	{
		delete methodSignature;
		methodSignature = NULL;
	}

	if(methodName != NULL)
	{
		delete methodName;
		methodName = NULL;
	}
}

//Exceptionテーブルの情報を取得
//beginPosがAttributeの開始位置
void MethodInfo::getExceptionInfo(U1* beginPos,ClassFile* pClassFile )
{

	//skip attribute_name_index
	beginPos += sizeof(U2);
	//skip attribute_length
    beginPos += sizeof(U4);
	//skip max_stack
	beginPos += sizeof(U2);
    //skip max_locals
    beginPos += sizeof(U2);
    //read code_length
	U4 code_length = ClassFile::getU4ReverseValue((U4*)beginPos);
	beginPos += sizeof(U4);
	//skip code
    beginPos += code_length;
	//read exception_table length
	exceptionTableNum = ClassFile::getU2ReverseValue((U2*)beginPos);
    beginPos += sizeof(U2);
	if(exceptionTableNum == 0)
	{   //no exception table
		return;
	}

	exceptions = new ExceptionInfoPtr[exceptionTableNum];
	for (int index = 0; index < exceptionTableNum; index++)
	{
        exceptions[index] = new ExceptionInfo(beginPos,pClassFile);
		beginPos+= sizeof(U2) * 4;
	}
}


AttributeInfo *MethodInfo::getCodeAttribute(ClassFile *classFile)
{
	U2 count = ClassFile::getU2ReverseValue(this->attributesCount);
	char buf[MAX_BUF_LEN];
	for (int index = 0; index < count; index++)
	{
		AttributeInfo *info = *(this->attributes + index);
		classFile->getUtf8ConstantInfo(
			ClassFile::getU2ReverseValue(info->attributeNameIndex), buf);

		if (strcmp(buf, "Code") == 0)
			return info;
	}

	return NULL;
}
