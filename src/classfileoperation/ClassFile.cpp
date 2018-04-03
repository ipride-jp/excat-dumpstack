#pragma warning(disable : 4503)
#pragma warning(disable : 4786)

#include "../common/Define.h"

#include "../common/HelperFunc.h"
#include <map>
#include <vector>
#include <string>
#include "../jniutility/ExceptionTableMan.h"

#include "ClassFile.h"
#include "ClassFileConstant.h"
#include <string.h>
#include "../common/Define.h"
#include "../common/Global.h"
#include "CpInfo.h"
#include "FieldInfo.h"
#include "MethodInfo.h"
#include "AttributeInfo.h"

USE_NS(NS_CLASS_FILE_OPERATION)
USE_NS(NS_COMMON)


ClassFile::ClassFile(jint classDataLen, const unsigned char *classData,ExceptionTableMan* param):className(NULL),pExceptionTableMan(NULL)
{
	pExceptionTableMan = param;
    initial(classDataLen,classData);
}

ClassFile::ClassFile(jint classDataLen, const unsigned char *classData):className(NULL),pExceptionTableMan(NULL)
{
    initial(classDataLen,classData);
}

void ClassFile::initial(jint classDataLen, const unsigned char *classData)
{
	//add by Qiu Song on 20090825 for メソッド監視
//	fieldTable = NULL;
//	ldcValueTable = NULL;
	bHasGetField = false;
	bHasLdc = false;
	bNeedAddLocal = false;
	nMaxLocal = 0;
	bLongOrDoubleReturn = false;
	addedConstantUtf8Info = NULL;
	//end of add by Qiu Song on 20090825 for メソッド監視
	
	this->size = classDataLen;
	this->classData = classData;

	U1 *pos = (U1 *)classData;
	
	this->magic = (U4 *)pos;
	pos += sizeof(*this->magic);
	
	this->minorVersion = (U2 *)pos;
	pos += sizeof(*this->minorVersion);

	this->majorVersion = (U2 *)pos;
	pos += sizeof(*this->majorVersion);
	
	this->constantPoolCount = (U2 *)pos;
	pos += sizeof(*this->constantPoolCount);

	U2 constPoolCount = ClassFile::getU2ReverseValue(this->constantPoolCount);
	if (constPoolCount > 0)
	{
		bool skip = false;
		this->constantPool = new CpInfoPtr[constPoolCount];
		memset(this->constantPool, 0, constPoolCount * sizeof(CpInfoPtr));
		int nFieldrefCount = 0;//add by Qiu Song on 20090825 for メソッド監視
		for (int index = 1; index < constPoolCount; index ++)
		{
			if (skip)
			{
				skip = false;
				continue;
			}

			CpInfo *cp = new CpInfo(pos);
			pos += cp->getSize();
			this->constantPool[index] = cp;
			//add by Qiu Song on 20090825 for メソッド監視
/*			if(unsigned(cp->tag[0]) == JVM_CONSTANT_Fieldref)
			{
				if(fieldTable == NULL)
				{
					fieldTable = new map<int,int>;
				}
				fieldTable->insert(pair<int,int>(index, nFieldrefCount));
				nFieldrefCount ++;
			}*/
			//end of add by Qiu Song on 20090825 for メソッド監視
			if (cp->isLargeNumericType()) 
				skip = true;
		}
	}

	this->accessFlags = (U2 *)pos;
	pos += sizeof(*this->accessFlags);

	this->thisClass = (U2 *)pos;
	pos += sizeof(*this->thisClass);

	//get class name
    CpInfo *thisref = *(this->constantPool + getU2ReverseValue(thisClass));
	int classNameIndex = getU2ReverseValue((U2*)thisref->info);
    getUtf8ConstantInfoNeedMemory(classNameIndex, &className);

	this->superClass = (U2 *)pos;
	pos += sizeof(*this->superClass);

	this->interfacesCount = (U2 *)pos;
	pos += sizeof(*this->interfacesCount);

	pos += ClassFile::getU2ReverseValue(this->interfacesCount) * sizeof(U2);

	this->fieldsCount = (U2 *)pos;
	pos += sizeof(*this->fieldsCount);

	U2 fieCount = ClassFile::getU2ReverseValue(this->fieldsCount);
	if (fieCount > 0)
	{
		this->fields = new FieldInfoPtr[fieCount];
		for (int index = 0; index < fieCount; index++)
		{
			FieldInfo *fi = new FieldInfo(pos);
			pos += fi->getSize();
			this->fields[index] = fi;
		}
	}
	else
		this->fields = NULL;

	this->methodsCount = (U2 *)pos;
	pos += sizeof(*this->methodsCount);


	U2 methCount = ClassFile::getU2ReverseValue(this->methodsCount);
	if (methCount > 0)
	{
		this->methods = new MethodInfoPtr[methCount];
		for (int index = 0; index < methCount; index++)
		{
			MethodInfo *mi = new MethodInfo(pos,this);
			pos += mi->getSize();
			this->methods[index] = mi;
		}
	}
	else
		this->methods = NULL;

	this->attributesCount = (U2 *)pos;
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
}

ClassFile::~ClassFile()
{
	if(className != NULL)
	{
		delete className;
		className = NULL;
	}

	//delete constantPool
	U2 constPoolCount = ClassFile::getU2ReverseValue(this->constantPoolCount);
	int index;
	for (index = 1; index < constPoolCount; index++) 
	{
		CpInfo *cpInfo = *(this->constantPool + index);
		if (cpInfo != NULL)
		{
			delete cpInfo;
			cpInfo = NULL;
		}
	}
	if (this->constantPool != NULL)
	{
		delete[] this->constantPool;
		this->constantPool = NULL;
	}

	//delete fields
	U2 fielCount = ClassFile::getU2ReverseValue(this->fieldsCount);
	for (index = 0; index < fielCount; index++)
	{
		delete *(this->fields + index);
	}
	if (this->fields != NULL)
	{
		delete[] this->fields;
		this->fields = NULL;
	}

	//delete methods
	U2 methCount = ClassFile::getU2ReverseValue(this->methodsCount);
	for (index = 0; index < methCount; index++)
	{
		delete *(this->methods + index);
	}
	if (this->methods != NULL)
	{
		delete[] this->methods;
		this->methods = NULL;
	}

	//delete attributes
	U2 attrCount = ClassFile::getU2ReverseValue(this->attributesCount);
	for (index = 0; index < attrCount; index++)
	{
		delete *(this->attributes + index);
	}
	if (this->attributes != NULL)
	{
		delete[] this->attributes;
		this->attributes = NULL;
	}

	//add by Qiu Song on 20090827 for メソッド監視
/*	if(bHasGetField == false && fieldTable != NULL)
	{
		fieldTable->clear();
		delete fieldTable;
		fieldTable = NULL;
	}
*/
/*	if(bHasLdc == false && ldcValueTable != NULL)
	{
		ldcValueTable->clear();
		delete ldcValueTable;
		ldcValueTable = NULL;
	}*/

	if(addedConstantUtf8Info != NULL)
	{
		addedConstantUtf8Info->clear();
		addedConstantUtf8Info = NULL;
	}
	//end of add by Qiu Song on 20090827 for メソッド監視
}

void ClassFile::writeU2(unsigned char *&pos, U2 val)
{
	writeU1(pos, (U1)(val >> 8));
	writeU1(pos, (U1)val);
}

void ClassFile::writeU1(unsigned char *&pos, U1 val)
{
	*pos = val & 0xFF;
	++pos;
}

void ClassFile::writeU4(unsigned char *&pos, U4 val)
{
	writeU2(pos, (U2)(val >> 16));
	writeU2(pos, (U2)val);
}

void ClassFile::addUtf8CpoolEntry(unsigned char *&pos, const char *str)
{
	*pos = JVM_CONSTANT_Utf8;
	++pos;

	U2 len = strlen(str);
	writeU2(pos, len);

	memcpy(pos, str, len);
	pos += len;
}

void ClassFile::addClassInfoCpoolEntry(unsigned char *&pos, int nameIndex)
{
	*pos = JVM_CONSTANT_Class;
	++pos;

	writeU2(pos, nameIndex);
}

void ClassFile::addStringInfoCpoolEntry(unsigned char *&pos, int nameIndex)
{
	*pos = JVM_CONSTANT_String;
	++pos;

	writeU2(pos, nameIndex);
}

void ClassFile::addNameAndTypeCpoolEntry(unsigned char *&pos, int nameIndex, int descrIndex)
{
	*pos = JVM_CONSTANT_NameAndType;
	++pos;

	writeU2(pos, nameIndex);
	writeU2(pos, descrIndex);
}

void ClassFile::addMethodrefCpoolEntry(unsigned char *&pos, int classIndex, int nameTypeIndex)
{
	*pos = JVM_CONSTANT_Methodref;
	++pos;

	writeU2(pos, classIndex);
	writeU2(pos, nameTypeIndex);
}

//廃棄しました(comment by Qiu Song)
void ClassFile::insertMethodIntoConstructor(const char *destClassName, const char *className, const char *methodName,
											const char*methodSig, unsigned char** newClassData, 
											long *newClassDataLen)
{
	*newClassData = new unsigned char[size * 2 + 512];
	memset(*newClassData, 0, size * 2 + 512);
	unsigned char* outputPos = *newClassData;
	const unsigned char* pos = classData;
	int len;

	//copy to position before constantPoolCount
	len = (U1*)this->constantPoolCount - pos;
	memcpy(outputPos, pos, len);
	pos += len;
	outputPos += len;

	//increase constant pool count by 6
	writeU2(outputPos, getU2ReverseValue(this->constantPoolCount) + 6);
	pos += 2;
	
	//copy from constant_pool to access flags
	len = (U1*)this->accessFlags - (U1*)this->constantPoolCount - 2;
	memcpy(outputPos, (U1*)this->constantPoolCount + 2, len);
	outputPos += len;
	pos += len;

	//add class name to constant pool
	addUtf8CpoolEntry(outputPos, className);
	int nameIndex = getU2ReverseValue(this->constantPoolCount);
	
	//add classinfo to constant pool
	addClassInfoCpoolEntry(outputPos, nameIndex);
	int classIndex = nameIndex + 1;

	//add method name to constant pool
	addUtf8CpoolEntry(outputPos, methodName);
	nameIndex = classIndex + 1;

	//add method signature to constant pool
	addUtf8CpoolEntry(outputPos, methodSig);
	int descrIndex = nameIndex + 1;

	//add nametype to constant pool
	addNameAndTypeCpoolEntry(outputPos, nameIndex, descrIndex);
	int nameTypeIndex = descrIndex + 1;

	//add methodref to constant pool
	addMethodrefCpoolEntry(outputPos, classIndex, nameTypeIndex);
	int methodIndex = nameTypeIndex + 1;

//	len = classDataLen - (pos - classData);
//	memcpy(outputPos, pos, len);
//	outputPos += len;
//	*newClassDataLen = outputPos - *newClassData;

	//copy from access flags to methods
	len = (U1*)this->methodsCount + sizeof(*this->methodsCount) - (U1*)this->accessFlags;
	memcpy(outputPos, this->accessFlags, len);
	outputPos += len;

	//modify init method
	int methCount = getU2ReverseValue(this->methodsCount);
	MethodInfo *methInfo = NULL;
	CpInfo *consPool = NULL;
	for (int index = 0; index < methCount; index++) 
	{
		methInfo = *(this->methods + index);

		char temp[MAX_BUF_LEN];
		this->getUtf8ConstantInfo(getU2ReverseValue(methInfo->nameIndex), temp);

		if ((strcmp(temp, "<init>") == 0) /*&& (index == 0)*/ )
		{
			AttributeInfo *attrInfo = methInfo->getCodeAttribute(this);

			//find invokespecial instruction
			U1 *invokeSpecialPos = findInvokeSpecialPos(attrInfo);

			//check class name, if it is 
			char invokeSpecialClassName[MAX_BUF_LEN];
			getInvokeSpecialClassName(invokeSpecialPos, invokeSpecialClassName);
			if (strcmp(invokeSpecialClassName, destClassName) == 0)
			{
				memcpy(outputPos, methInfo->accessFlags, methInfo->getSize());
				outputPos += methInfo->getSize();
				continue;
			}

			int size = 4;
			len = (U1*)attrInfo->attributeNameIndex - (U1*)methInfo->accessFlags;
			memcpy(outputPos, methInfo->accessFlags, len);
			outputPos += len;

			len = sizeof(*attrInfo->attributeNameIndex);
			memcpy(outputPos, attrInfo->attributeNameIndex, len);
			outputPos += len;

			//attribute length + 4
			writeU4(outputPos, getU4ReverseValue(attrInfo->attributeLength) + size);
			len = sizeof(*attrInfo->attributeLength);

			U1 *tempPos = attrInfo->info;

			//increment max stack size
			//max stack
			writeU2(outputPos, getU2ReverseValue((U2*)tempPos) + 1);
			tempPos += 2;
			
			//max locals
			memcpy(outputPos, tempPos, 2);
			outputPos += 2;
			tempPos += 2;

			//code length + 4
			long codeLen = getU4ReverseValue((U4*)tempPos);
			writeU4(outputPos, codeLen + size);
			tempPos += 4;

			int invokeSpecLen = 0;
			while (opc_invokespecial != *tempPos) 
			{
				writeU1(outputPos, *tempPos);
				tempPos++;
				invokeSpecLen++;
			}

			//copy invokespecial
			len = 3;
			memcpy(outputPos, tempPos, len);
			outputPos += len;
			tempPos += len;
			invokeSpecLen += 3;

			//add aload_0
			writeU1(outputPos, opc_aload_0);

			//add invokestatic instruction
			writeU1(outputPos, opc_invokestatic);
			writeU2(outputPos, methodIndex);
			
			//left code
			len = codeLen - invokeSpecLen;
			memcpy(outputPos, tempPos, len);
			outputPos += len;
			tempPos += len;

			//exception table
			len = getU2ReverseValue((U2*)tempPos);
			len = len * 8 + 2;
			memcpy(outputPos, tempPos, len);
			outputPos += len;
			tempPos += len;

			//attribute count
			int attrCount = getU2ReverseValue((U2*)tempPos);
			writeU2(outputPos, attrCount);
			tempPos += 2;
			for (int attrIndex = 0; attrIndex < attrCount; attrIndex++)
			{
				int nameIndex = getU2ReverseValue((U2*)tempPos);
				char nameBuf[MAX_BUF_LEN];
				this->getUtf8ConstantInfo(nameIndex, nameBuf);
				
				writeU2(outputPos, nameIndex);
				tempPos += 2;

				//change the line number info
				if (strcmp(nameBuf, "LineNumberTable") == 0)
				{
					len = 4;
					memcpy(outputPos, tempPos, len);
					outputPos += len;
					tempPos += len;

					int lineLen = getU2ReverseValue((U2*)tempPos);
					writeU2(outputPos, lineLen);
					tempPos += 2;

					for (int lineIndex = 0; lineIndex < lineLen; lineIndex++)
					{
						if (lineIndex == 0)
						{
							memcpy(outputPos, tempPos, 4);
							outputPos += 4;
							tempPos += 4;
							continue;
						}

						writeU2(outputPos, getU2ReverseValue((U2*)tempPos) + size);
						tempPos += 2;

						memcpy(outputPos, tempPos, 2);
						tempPos += 2;
						outputPos += 2;

					}
				}
				else if(strcmp(nameBuf, "LocalVariableTable") == 0)//local variable
				{
					memcpy(outputPos, tempPos, 4);
					outputPos += 4;
					tempPos += 4;

					int localLen = getU2ReverseValue((U2*)tempPos);
					writeU2(outputPos, localLen);
					tempPos += 2;

					for (int localIndex = 0; localIndex < localLen; localIndex++)
					{
						//start pc
						if ((U2*)tempPos == 0) //this object and parameters
						{
							memcpy(outputPos, tempPos, 2);
							outputPos += 2;
						}
						else //other local variables
						{
							writeU2(outputPos, getU2ReverseValue((U2*)tempPos) + size);
						}
						tempPos += 2;

						//length
						writeU2(outputPos, getU2ReverseValue((U2*)tempPos) + size);
						tempPos += 2;

						//left
						memcpy(outputPos, tempPos, 6);
						tempPos += 6;
						outputPos += 6;
					}
				}
				else
				{
					len = getU4ReverseValue((U4*)tempPos) + 4;
					memcpy(outputPos, tempPos, len);
					outputPos += len;
					tempPos += len;
				}
			}

			int attrLen = 2 + 4 + getU4ReverseValue(attrInfo->attributeLength);
			int beforeAttrLen = (U1*)attrInfo->attributeNameIndex - (U1*)methInfo->accessFlags;
			len = methInfo->getSize() - attrLen - beforeAttrLen;
			memcpy(outputPos, tempPos, len);
			outputPos += len;
		}
		else
		{
			memcpy(outputPos, methInfo->accessFlags, methInfo->getSize());
			outputPos += methInfo->getSize();
		}
	}

//	len = this->classDataLen - ((U1*)this->attributesCount - this->classData);
//	memcpy(outputPos, this->attributesCount, len);
//	outputPos += len;
	
	len = this->classData + this->size - (U1*)this->attributesCount;
	memcpy(outputPos, this->attributesCount, len);
	outputPos += len;

	*newClassDataLen = outputPos - *newClassData;
}

void ClassFile::getUtf8ConstantInfo(int cpIndex, char *buf)
{
	CpInfo *consPool = *(this->constantPool + cpIndex);
	consPool->getUtf8Info(buf);
}

void ClassFile::getUtf8ConstantInfoNeedMemory(int cpIndex, char **buf)
{
	CpInfo *consPool = *(this->constantPool + cpIndex);
	consPool->getUtf8InfoNeedMemory(buf);
}

U1 *ClassFile::findInvokeSpecialPos(AttributeInfo *attrInfo)
{
	U1 *tempPos = attrInfo->info;

	//max stack
	tempPos += 2;
	
	//max locals
	tempPos += 2;

	//code length + 4
	tempPos += 4;

	while (opc_invokespecial != *tempPos) 
	{
		tempPos++;
	}

	return tempPos;
}

void ClassFile::getInvokeSpecialClassName(U1 *invokeSpecialPos, char *className)
{
	//check invokespecial class
	int methodrefIndex = getU2ReverseValue((U2*)(invokeSpecialPos + 1));
	CpInfo *methodref = *(this->constantPool + methodrefIndex);
	int classIndex = getU2ReverseValue((U2*)methodref->info);
	CpInfo *classInfo = *(this->constantPool + classIndex);
	int classNameIndex = getU2ReverseValue((U2*)classInfo->info);
	getUtf8ConstantInfo(classNameIndex, className);

	if(Global::logger->isDebugEnabled())
	{
		string logBuf = "The class name of invokespecial instruction is: ";
		logBuf += className;
		LOG4CXX_DEBUG(Global::logger, logBuf.c_str());
	}

}

bool ClassFile::isInterface()
{
	U2 flags = getU2ReverseValue(this->accessFlags);
	return (flags & ACC_INTERFACE) == ACC_INTERFACE;
}

bool ClassFile::isMethodDefValid(const char* methodName, const char* methodSignature, 
								 char* errorMsg)
{
    U2 methCount = getU2ReverseValue(this->methodsCount);
	if (methCount == 0)
	{
        strcpy(errorMsg, "Method does not exist.");
		return false;
	}

    bool compareSignature = true;
	if (strlen(methodSignature) == 0)
         compareSignature = false;

	MethodInfo *methInfo = NULL;
	U2 count = 0, methodIndex = 0;
	for (U2 index = 0; index < methCount; index++) 
	{	     
		methInfo = *(this->methods + index);

		char method[MAX_BUF_LEN];
		getUtf8ConstantInfo(getU2ReverseValue(methInfo->nameIndex), method);
        
		//modified by Qiu Song on 20090915 for 監視関数曖昧指定
		if(HelperFunc::doesStringMatch(method, methodName) == false)
		{
			continue;
		}
        //end of modified by Qiu Song on 20090915 for 監視関数曖昧指定

		if (compareSignature)
		{
			char descriptor[MAX_BUF_LEN];
			getUtf8ConstantInfo(getU2ReverseValue(methInfo->descriptorIndex), descriptor);
			char* newDescriptor = HelperFunc::convertMethodSig(descriptor);
            if (strcmp(methodSignature, newDescriptor) != 0){
				delete[] newDescriptor;
				continue;
			}
			delete[] newDescriptor;
		}
		//add by Qiu Song on 20090924 for シグナル指定なしですべての同名のメソッドを監視する
		methodIndex = index;
		methInfo = *(this->methods + methodIndex);
		U2 flags = getU2ReverseValue(methInfo->accessFlags);
		if ((flags & ACC_NATIVE) == ACC_NATIVE)
		{
			strcpy(errorMsg, "Method cann't be native.");
			continue;
		}
		if ((flags & ACC_ABSTRACT) == ACC_ABSTRACT)
		{
			strcpy(errorMsg, "Method cann't be abstract.");
			continue;
		}
		count++;
		//end of add by Qiu Song on 20090924 for シグナル指定なしですべての同名のメソッドを監視する
	}
	
	if (count == 0)
	{
		 strcpy(errorMsg, "Method does not exist.");
		 return false;
	}
	return true;
}

int ClassFile::insertByteCodeToClass(int modifyType, int insertMethodCount, 
									 string *insertMethodName, string *insertMethodSig,
									 int *insertMethodPosition,string paramClassName,
									 string paramClassUrl, unsigned char **newClassData, 
									 long *newClassDataLen)
{
	*newClassData = new unsigned char[size * 2 + 512];
	memset(*newClassData, 0, size * 2 + 512);
	unsigned char* outputPos = *newClassData;
	const unsigned char* pos = classData;
	int len;

	//copy to position before constantPoolCount
	len = (U1*)this->constantPoolCount - pos;
	memcpy(outputPos, pos, len);
	pos += len;
	outputPos += len;

	//modified by Qiu Song on 20090916 for メソッド監視(-gの対応)
	int nConstantPoolCount;
	//increase constant pool count by 8
	switch(modifyType)
	{
	case 1:  //monitoringInstance
		nConstantPoolCount = getU2ReverseValue(this->constantPoolCount) + 10 + 1;
		break;
	case 2:  //monitoringMethod
		nConstantPoolCount = getU2ReverseValue(this->constantPoolCount) + 8 + 1;
		break;
	case 3:  //monitoringInstance + monitoringMethod
	default:
		nConstantPoolCount = getU2ReverseValue(this->constantPoolCount) + 14 + 1;
		break;
	}
	int nReturnValuePos = nConstantPoolCount -1;
    unsigned char* pConstantPoolCountPos = outputPos;
    writeU2(outputPos, nConstantPoolCount);
	//end of modified by Qiu Song on 20090916 for メソッド監視(-gの対応)

	pos += 2;

	//copy from constant_pool to access flags
	len = (U1*)this->accessFlags - (U1*)this->constantPoolCount - 2;
	memcpy(outputPos, (U1*)this->constantPoolCount + 2, len);
	outputPos += len;
	pos += len;

	int classNameIndex_forInstance = 0;
	int nameIndex = 0;
	if (modifyType != 2)
	{
		//add class name to constant pool
		addUtf8CpoolEntry(outputPos, paramClassName.c_str());
		int classNameIndex = getU2ReverseValue(this->constantPoolCount);

		//add this class name to constant pool
		addStringInfoCpoolEntry(outputPos, classNameIndex);
		classNameIndex_forInstance = classNameIndex + 1;

		//add class name to constant pool
		addUtf8CpoolEntry(outputPos, CALLBACK_CLASS_NAME);
		nameIndex = classNameIndex_forInstance + 1;
	}
	else
	{
		//add class name to constant pool
		addUtf8CpoolEntry(outputPos, CALLBACK_CLASS_NAME);
		nameIndex = getU2ReverseValue(this->constantPoolCount);
	}
	
	//add classinfo to constant pool
	addClassInfoCpoolEntry(outputPos, nameIndex);
	int classIndex = nameIndex + 1;

	//add param to constant pool
	addUtf8CpoolEntry(outputPos, paramClassUrl.c_str());
	int paramIndex_forMethod = classIndex + 1;
	
	//add stringref to constant pool
	addStringInfoCpoolEntry(outputPos, paramIndex_forMethod);
	paramIndex_forMethod += 1;

	int methodIndex_forInstance = paramIndex_forMethod; 
    if ((modifyType == 1) || (modifyType == 3))
	{
		//add Callback::callback_forInstance
		//add method name to constant pool
		addUtf8CpoolEntry(outputPos, CALLBACK_METHOD_FOR_INSTANCE);
		nameIndex = paramIndex_forMethod + 1;
		
		//add method signature to constant pool
		addUtf8CpoolEntry(outputPos, CALLBACK_METHOD_FOR_INSTANCE_SIG);
		int descrIndex = nameIndex + 1;
		
		//add nametype to constant pool
		addNameAndTypeCpoolEntry(outputPos, nameIndex, descrIndex);
		int nameTypeIndex = descrIndex + 1;
		
		//add methodref to constant pool
		addMethodrefCpoolEntry(outputPos, classIndex, nameTypeIndex);
		methodIndex_forInstance = nameTypeIndex + 1;
	}
	int methodIndex_forMethod = 0; 
    if ((modifyType == 2) || (modifyType == 3))
	{
		//add Callback::callback_for_method
		//add method name to constant pool
		addUtf8CpoolEntry(outputPos, CALLBACK_METHOD_FOR_METHOD);
		nameIndex = methodIndex_forInstance + 1;
		
		//add method signature to constant pool
		addUtf8CpoolEntry(outputPos, CALLBACK_METHOD_FOR_METHOD_SIG);
		int descrIndex = nameIndex + 1;
		
		//add nametype to constant pool
		addNameAndTypeCpoolEntry(outputPos, nameIndex, descrIndex);
		int nameTypeIndex = descrIndex + 1;
		
		//add methodref to constant pool
		addMethodrefCpoolEntry(outputPos, classIndex, nameTypeIndex);
		methodIndex_forMethod = nameTypeIndex + 1;
	}

	//add by Qiu Song on 20090916 for メソッド監視(-gの対応)
	addMethodReturnTypeToConstantPool(outputPos, pConstantPoolCountPos, nConstantPoolCount);
	//end of add by Qiu Song on 20090916 for メソッド監視(-gの対応)

	//copy from access flags to methods
	len = (U1*)this->methodsCount + sizeof(*this->methodsCount) - (U1*)this->accessFlags;
	memcpy(outputPos, this->accessFlags, len);
	outputPos += len;

	int methCount = getU2ReverseValue(this->methodsCount);
	MethodInfo *methInfo = NULL;
	CpInfo *consPool = NULL;

	for (int index = 0; index < methCount; index++) 
	{
		methInfo = *(this->methods + index);

		//add by Qiu Song on 20090812 for メソッド監視
		unsigned char *attributeLengthPos = NULL;
		long attributeLengthValue = 0;
		unsigned char *codeLengthPos = NULL;
		long codeLengthValue = 0;
		unsigned char *attributeMaxLocalPos = NULL;
	    nMaxLocal = 0;
		bLongOrDoubleReturn = false;
		bNeedAddLocal = false;
		//end of add by Qiu Song on 20090812 for メソッド監視

		char method[MAX_BUF_LEN];
		char descriptor[MAX_BUF_LEN];
		this->getUtf8ConstantInfo(getU2ReverseValue(methInfo->nameIndex), method);//メソッド名
		this->getUtf8ConstantInfo(getU2ReverseValue(methInfo->descriptorIndex), descriptor);//メソッド解析文

		//configファイルに指定された監視メソッドを探す
        int i = 0;
		for (i = 0; i < insertMethodCount; i++)
		{
			//modified by Qiu Song on 20090915 for 監視関数曖昧指定
			if(HelperFunc::doesStringMatch(method, insertMethodName[i].c_str()) == true)
			//end of modified by Qiu Song on 20090915 for 監視関数曖昧指定
			{
				char* newdescriptor = HelperFunc::convertMethodSig(descriptor);
				if (strlen(insertMethodSig[i].c_str()) == 0)
				{
					delete[] newdescriptor;
					break;
				}
				else if (strcmp(newdescriptor, insertMethodSig[i].c_str()) == 0)
				{
					delete[] newdescriptor;
					break;
				}
				delete[] newdescriptor;
			}
		}

		//add by Qiu Song on 20090916 for メソッド監視(-gの対応)
        int nReturnTypePos = findUtf8InfoConstantPos(HelperFunc::getReturnTypeFromMethodSig(descriptor));
		//end of add by Qiu Song on 20090916 for メソッド監視(-gの対応)
		int size = 0;
		//add by Qiu Song on 20090828 for メソッド監視
		//監視位置：0-both 1-start 2-end
		int nDumpPosition = 1;
		if(insertMethodPosition != NULL)
		{
			//監視位置：0-both 1-start 2-end
			nDumpPosition = insertMethodPosition[i];
		}
		//end of add by Qiu Song on 20090828 for メソッド監視
		bool insertForMethod = false;
		if (i < insertMethodCount)
		{
			insertForMethod = true;
			//増えたsize(メソッド監視の参照 comment by Qiu Song)
			size += 5;
			if (paramIndex_forMethod > 255)
			{
				size += 1;  //ldc-->ldc_w
			}
		}
		bool insertForInstance = false;
		if ((modifyType == 1 || modifyType == 3) 
			&& (strcmp(method, "<init>") == 0))
		{
			insertForInstance = true;
			size += 8;
			if (classNameIndex_forInstance > 255)
			{
				size += 1;  //ldc-->ldc_w
			}
			if (paramIndex_forMethod > 255)
			{
				size += 1;  //ldc-->ldc_w
			}
		}
		if (insertForMethod || insertForInstance)
		{
			AttributeInfo *attrInfo = methInfo->getCodeAttribute(this);

			len = (U1*)attrInfo->attributeNameIndex - (U1*)methInfo->accessFlags;
			memcpy(outputPos, methInfo->accessFlags, len);
			outputPos += len;

			len = sizeof(*attrInfo->attributeNameIndex);
			memcpy(outputPos, attrInfo->attributeNameIndex, len);
			outputPos += len;

			//attribute length + size
			//add by Qiu Song on 20090812 for メソッド監視
			attributeLengthPos = outputPos;
			attributeLengthValue = getU4ReverseValue(attrInfo->attributeLength) + size;
			//end of add by Qiu Song on 20090812 for メソッド監視
			writeU4(outputPos, getU4ReverseValue(attrInfo->attributeLength) + size);
			len = sizeof(*attrInfo->attributeLength);

			U1 *tempPos = attrInfo->info;

			//increment max stack size
			//max stack (>= 3)
			U2 stackSize = getU2ReverseValue((U2*)tempPos);
			if (stackSize >= 3)
			{
				writeU2(outputPos, stackSize);
			}
			else
			{
				writeU2(outputPos, 3);
			}
			tempPos += 2;
			
			//max locals
			//add by Qiu Song on 20090907 for バグ：戻り値は他の関数の戻り値の場合
			attributeMaxLocalPos = outputPos;
			nMaxLocal = getU2ReverseValue((U2*)tempPos);
			//end of add by Qiu Song on 20090907 for バグ：戻り値は他の関数の戻り値の場合
			memcpy(outputPos, tempPos, 2);
			outputPos += 2;
			tempPos += 2;

			//code length + size
			//add by Qiu Song on 20090812 for メソッド監視
			codeLengthPos = outputPos;
			codeLengthValue = getU4ReverseValue((U4*)tempPos) + size;
			//end of add by Qiu Song on 20090812 for メソッド監視
			long codeLen = getU4ReverseValue((U4*)tempPos);
			writeU4(outputPos, codeLen + size );
			tempPos += 4;

			int invokeSpecLen = 0;
			//初期化処理メソッド(構造関数)をスキップする
			if (strcmp(method, "<init>") == 0)
			{
				while (opc_invokespecial != *tempPos) 
				{
					writeU1(outputPos, *tempPos);
					tempPos++;
					invokeSpecLen++;
				}

				//copy invokespecial
				len = 3;
				memcpy(outputPos, tempPos, len);
				outputPos += len;
				tempPos += len;
				invokeSpecLen += 3;

				if (insertForInstance)
				{
					//add aload_0
					writeU1(outputPos, opc_aload_0);
					
					//add ldc/ldc_w
					if (classNameIndex_forInstance > 255)
					{
						writeU1(outputPos, opc_ldc_w);
						writeU2(outputPos, classNameIndex_forInstance); 
					}
					else
					{
						writeU1(outputPos, opc_ldc);
						writeU1(outputPos, (U1)classNameIndex_forInstance); 
					}

					//add ldc/ldc_w
					if (paramIndex_forMethod > 255)
					{
						writeU1(outputPos, opc_ldc_w);
						writeU2(outputPos, paramIndex_forMethod); 
					}
					else
					{
						writeU1(outputPos, opc_ldc);
						writeU1(outputPos, (U1)paramIndex_forMethod); 
					}

					//add invokestatic instruction
					writeU1(outputPos, opc_invokestatic);
					writeU2(outputPos, methodIndex_forInstance);
				}//end of 「if (insertForInstance)」 line：858
			}//end of 「if (strcmp(method, "<init>") == 0)」 line:842

			//add by Qiu Song on 20090812 for メソッド監視
			int nNewPos = 0;
			int nInsertLength = 5;
			if (paramIndex_forMethod > 255)
			{
				nInsertLength = 6;
			}
			map<int,int>* byteCodeLineChangeMap = new map<int,int>;
			if (insertForMethod && nDumpPosition != 2)
			{
				insertCallbacksFunc(&outputPos, paramIndex_forMethod, methodIndex_forMethod, nInsertLength);
				byteCodeLineChangeMap->insert(pair<int,int>(0,nInsertLength));
				nNewPos += nInsertLength;
			}

			//left code
			len = codeLen - invokeSpecLen;

			if(nDumpPosition == 1)
			{
				if(false == copyTheLeftCode(&outputPos, tempPos, len, nInsertLength, byteCodeLineChangeMap, nNewPos))
				{
					memcpy(outputPos, tempPos, len);
					outputPos += len;
				}
			}
			else
			{ 
				if(true == insertCallbackFuncBeforeReturn(&outputPos, paramIndex_forMethod, 
									methodIndex_forMethod,tempPos, len, byteCodeLineChangeMap, nNewPos))
				{
					//リターンコード前にgetfieldがある場合
/*					if(bHasGetField == true)
					{
						//フィールドテーブルに追加する
						if(Global::classFieldTable == NULL)
						{
							Global::classFieldTable = new map<string, map<int,int>*>;
						}
						Global::classFieldTable->insert(pair<string, map<int, int>*>(paramClassName, fieldTable));
					}

					if(bHasLdc == true)
					{
						if(Global::classConstantPool == NULL)
						{
							Global::classConstantPool = new map<string, map<int, string>*>;
						}
						Global::classConstantPool->insert(pair<string, map<int, string>*>(paramClassName, ldcValueTable));
					}
*/				}
				else
				{
					memcpy(outputPos, tempPos, len);
					outputPos += len;
				}
			}
			//attribute lengthとcode lengthをリセットする
			unsigned addedSize = getAddedSizeAfterInsertCallbacks(byteCodeLineChangeMap);
			//add by Qiu Song on 20091125 for バグ：534
			if(len == 1 && nDumpPosition == 0 && byteCodeLineChangeMap->size() == 1)
			{
				addedSize += addedSize;
			}
			//end of add by Qiu Song on 20091125 for バグ：534
			if(addedSize != 0)
			{
				attributeLengthValue = attributeLengthValue + addedSize - nInsertLength;
				unsigned char *attributeLengthPosBak = attributeLengthPos;
				writeU4(attributeLengthPosBak, attributeLengthValue);
				codeLengthValue = codeLengthValue + addedSize - nInsertLength;
                writeU4(codeLengthPos, codeLengthValue);
			}
			tempPos += len;

			if(bNeedAddLocal == true)
			{
			    //add by Qiu Song on 20090907 for バグ：戻り値は他の関数の戻り値の場合
				//一時変数の個数を+1する
				writeU2(attributeMaxLocalPos, nMaxLocal);
			}

			//end of add by Qiu Song on 20090812 for メソッド監視

			//exception table
			int exceptionLen = getU2ReverseValue((U2*)tempPos);
			
			writeU2(outputPos, exceptionLen);
			tempPos += 2;
			for (int exceptionIndex = 0; exceptionIndex < exceptionLen; exceptionIndex++)
			{
				//modified by Qiu Song on 20090814 for メソッド監視
				//start PC
				writeU2(outputPos, getTablePosAfterInsertCallbacks(getU2ReverseValue((U2*)tempPos),
																   byteCodeLineChangeMap, size, nInsertLength));
				tempPos += 2;


				//end PC
				writeU2(outputPos, getTablePosAfterInsertCallbacks(getU2ReverseValue((U2*)tempPos),
																   byteCodeLineChangeMap, size, nInsertLength));
				tempPos += 2;

				//handle PC
				writeU2(outputPos, getTablePosAfterInsertCallbacks(getU2ReverseValue((U2*)tempPos),
																   byteCodeLineChangeMap, size, nInsertLength));
				//end of modified by Qiu Song on 20090814 for メソッド監視

				tempPos += 2;

				//catch type
				memcpy(outputPos, tempPos, 2);
				tempPos += 2;
				outputPos += 2;
			}

			//attribute count
			int attrCount = getU2ReverseValue((U2*)tempPos);
			writeU2(outputPos, attrCount);
			tempPos += 2;
			for (int attrIndex = 0; attrIndex < attrCount; attrIndex++)
			{
				int nameIndex = getU2ReverseValue((U2*)tempPos);
				char nameBuf[MAX_BUF_LEN];
				this->getUtf8ConstantInfo(nameIndex, nameBuf);
				
				writeU2(outputPos, nameIndex);
				tempPos += 2;

				//change the line number info
				if (strcmp(nameBuf, "LineNumberTable") == 0)
				{
					len = 4;
					memcpy(outputPos, tempPos, len);
					outputPos += len;
					tempPos += len;

					int lineLen = getU2ReverseValue((U2*)tempPos);
					writeU2(outputPos, lineLen);
					tempPos += 2;

					for (int lineIndex = 0; lineIndex < lineLen ; lineIndex++)
					{
						if ((lineIndex == 0) && strcmp(method, "<init>") == 0)
						{
							memcpy(outputPos, tempPos, 4);
							outputPos += 4;
							tempPos += 4;
							continue;
						}

						//modified by Qiu Song on 20090814 for メソッド監視
						writeU2(outputPos, getTablePosAfterInsertCallbacks(getU2ReverseValue((U2*)tempPos),
																   byteCodeLineChangeMap, size, nInsertLength));
						//end of modified by Qiu Song on 20090814 for メソッド監視
						
						tempPos += 2;

						memcpy(outputPos, tempPos, 2);
						tempPos += 2;
						outputPos += 2;
					}
				}
				else if ((strcmp(nameBuf, "LocalVariableTable") == 0) //local variable
					|| (strcmp(nameBuf, "LocalVariableTypeTable") == 0)) //local variable type table
				{
					//add by Qiu Song on 20090916 for メソッド監視(-gの対応)
					unsigned char* localVariableAttrLenPos = outputPos;
					long nLocalVariableAttrLen = getU4ReverseValue((U4*)tempPos);
					//end of add by Qiu Song on 20090916 for メソッド監視(-gの対応)
					memcpy(outputPos, tempPos, 4);
					outputPos += 4;
					tempPos += 4;

					int localLen = getU2ReverseValue((U2*)tempPos);
					//add by Qiu Song on 20090916 for メソッド監視(-gの対応)
					if(bNeedAddLocal == true && (strcmp(nameBuf, "LocalVariableTable") == 0))
					{
						localLen++;
					}
					//end of add by Qiu Song on 20090916 for メソッド監視(-gの対応)
					writeU2(outputPos, localLen);
					tempPos += 2;

					for (int localIndex = 0; localIndex < localLen; localIndex++)
					{
						if (getU2ReverseValue((U2*)tempPos) == 0) //this object and parameters
						{
							//start pc
							memcpy(outputPos, tempPos, 2);
							outputPos += 2;
							tempPos += 2;

							//modified by Qiu Song on 20090814 for メソッド監視
							//length
						    writeU2(outputPos, getTablePosAfterInsertCallbacks(getU2ReverseValue((U2*)tempPos),
																   byteCodeLineChangeMap, size, nInsertLength));
							//end of modified by Qiu Song on 20090814 for メソッド監視

							tempPos += 2;
						}
						else //other local variables
						{
							//add by Qiu Song on 20090916 for メソッド監視(-gの対応)
    						if(bNeedAddLocal == true && localIndex == localLen -1
								&& (strcmp(nameBuf, "LocalVariableTable") == 0))
							{
								writeU2(outputPos, 0);
								writeU2(outputPos, codeLengthValue);
								writeU2(outputPos, nReturnValuePos);
								writeU2(outputPos, nReturnTypePos);
								//long或いはdoubleの場合、indexは最大一時変数個数-2
								if(bLongOrDoubleReturn)
								{
									writeU2(outputPos, nMaxLocal - 2);
								}
								else
								{
									writeU2(outputPos, nMaxLocal - 1);
								}

								//attributeの長さをリセットする
								attributeLengthValue += 10;
								writeU4(attributeLengthPos, attributeLengthValue);
								nLocalVariableAttrLen += 10;
                                writeU4(localVariableAttrLenPos, nLocalVariableAttrLen);
								break;
							}
							else
							//end of add by Qiu Song on 20090916 for メソッド監視(-gの対応)
							{
							//modified by Qiu Song on 20090814 for メソッド監視
							//start pc
							int nOldStartPc = getU2ReverseValue((U2*)tempPos);
							int nNewStartPc = getTablePosAfterInsertCallbacks(nOldStartPc - 1, byteCodeLineChangeMap,
								                  size, nInsertLength) + 1;//test
							writeU2(outputPos, nNewStartPc);
							//end of modified by Qiu Song on 20090814 for メソッド監視
							
							tempPos += 2;

							//length
							int nLength = getU2ReverseValue((U2*)tempPos);
							nLength = getTablePosAfterInsertCallbacks(nOldStartPc + nLength, byteCodeLineChangeMap, 
								          size, nInsertLength) - nNewStartPc;
						    writeU2(outputPos, nLength);
							tempPos += 2;
							}
						}

						//left
						memcpy(outputPos, tempPos, 6);
						tempPos += 6;
						outputPos += 6;
					}//end of for (int localIndex = 0; localIndex < localLen; localIndex++)
				}
				else
				{
					len = getU4ReverseValue((U4*)tempPos) + 4;
					memcpy(outputPos, tempPos, len);
					outputPos += len;
					tempPos += len;
				}
			}

			int attrLen = 2 + 4 + getU4ReverseValue(attrInfo->attributeLength);
			int beforeAttrLen = (U1*)attrInfo->attributeNameIndex - (U1*)methInfo->accessFlags;
			len = methInfo->getSize() - attrLen - beforeAttrLen;
			memcpy(outputPos, tempPos, len);
			outputPos += len;
			
			//add by Qiu Song on 20090814 for メソッド監視
			byteCodeLineChangeMap->clear();
			delete byteCodeLineChangeMap;
			//end of add by Qiu Song on 20090814 for メソッド監視
		}
		else
		{
			memcpy(outputPos, methInfo->accessFlags, methInfo->getSize());
			outputPos += methInfo->getSize();
		}

	}//end of 「for (int index = 0; index < methCount; index++)」

//	len = this->classDataLen - ((U1*)this->attributesCount - this->classData);
//	memcpy(outputPos, this->attributesCount, len);
//	outputPos += len;
	

	len = this->classData + this->size - (U1*)this->attributesCount;
	memcpy(outputPos, this->attributesCount, len);
	outputPos += len;
	*newClassDataLen = outputPos - *newClassData;    	
	return 0;
}

//begin of add

bool ClassFile::isClassNameEqual(const char* className)
{

     char temp[MAX_BUF_LEN];

     CpInfo *thisref = *(this->constantPool + getU2ReverseValue(thisClass));
     int classNameIndex = getU2ReverseValue((U2*)thisref->info);
     getUtf8ConstantInfo(classNameIndex, temp);
     char*  pTmp = HelperFunc::replaceChar(className, '.', '/');
     if (strcmp(pTmp, temp) == 0)

     {
		  delete pTmp;
          return true;
     }
     else
     {
		  delete pTmp;
          return false;
     }

}

//end of add


//add by Qiu Song on 20090812 for メソッド監視
//メソッドを終了する前にcallbacksメソッドを追加する
bool ClassFile::insertCallbackFuncBeforeReturn(unsigned char** outputPos,
											   int paramIndex_forMethod, 
											   int methodIndex_forMethod,
											   unsigned char * byteCodeData, 
											   int len,
											   map<int,int>* byteCodeLineChangeMap,
											   int& nNewPos)
{
	//一時変数初期化処理
	unsigned char * tempData = new unsigned char[len+1];
	memset(tempData, 0, len+1);
	memcpy(tempData, byteCodeData, len);
	int nInsertLength = 5;
	if (paramIndex_forMethod > 255)
	{
		nInsertLength = 6;
	}

	//バイトコードにreturnコードがあるの場合
	if(findReturnPos(tempData, len, nInsertLength, byteCodeLineChangeMap) == true)
	{
		unsigned preOpcode;
		int nPrePos;
		bool bHasMaxLocalAdded = false;
		int nLocalNum = nMaxLocal;
		for(int nPos = 0; nPos < len; nPos ++,nNewPos++)
		{

			unsigned opCode = readU1(tempData, nPos);
			
			//ジャンプコードの場合、ジャンプ先を再調整する
			if(isJumpOperand(opCode))
			{
				//ジャンプコードを書き込む
				writeU1(*outputPos, opCode);
				nPos++;
				nNewPos++;
				
				//ジャンプ先を修正する
				modifyJumpTarget(outputPos, opCode, tempData, nPos, nNewPos, byteCodeLineChangeMap,true);
			}

			//switchコードの場合、ジャンプ先を再調整する
			else if(isSwitchOperand(opCode))
			{
				writeU1(*outputPos, opCode);
				nPos++;
				nNewPos++;

				//ジャンプ先を修正する
				modifySwitchTarget(outputPos, opCode, tempData, nPos, nNewPos, byteCodeLineChangeMap,true);
			}

			//リターンコードの場合、callbacksメソッドを挿入する
			else if(isReturnOperand(opCode))
			{
				if((isInvokeCode(preOpcode) == true || isGetFieldCode(preOpcode) == true ||
				   isLdcCode(preOpcode) == true || isMethodOpCode(preOpcode) == true) 
				   && (opCode != opc_return  && opCode != opc_athrow))
				{
					//戻り値は他の関数の戻り値の場合、一つの一時変数を追加する
					insertLocalParamBeforeReturn(outputPos, opCode, nLocalNum, nNewPos);
					if(bHasMaxLocalAdded == false)
					{
						if(opCode == opc_lreturn || opCode == opc_dreturn)
						{
							nMaxLocal += 2;
							bLongOrDoubleReturn = true;
						}
						else
						{
							nMaxLocal ++;
							bLongOrDoubleReturn = false;
						}
						bHasMaxLocalAdded = true;
					}
				}
				insertCallbacksFunc(outputPos, paramIndex_forMethod, methodIndex_forMethod, nInsertLength);
				nNewPos += nInsertLength;
				writeU1(*outputPos, opCode);
			}
			else
			{
				if(false == isTypeChangeCode(opCode))
				{
					nPrePos = nPos;
					preOpcode = opCode;
				}

				writeU1(*outputPos, opCode);
				int nSkipLength = getOpCodeSkipLength(tempData, nPos, opCode);
				for(int i = 0; i < nSkipLength; i++)
				{
					nPos ++;
					nNewPos ++;
					opCode = readU1(tempData, nPos);
					writeU1(*outputPos, opCode);
				}
			}
		}
		delete[] tempData;
		return true;
	}
	delete[] tempData;
	return false;
}

//バイトコードにリターンコードを探す
bool ClassFile::findReturnPos(unsigned char * byteCodeData, 
							  int len,
							  int nInsertLength,
							  map<int,int>* byteCodeLineChangeMap)
{
	bool bHasReturn = false;

	int nPrePos;
    unsigned preOpcode;

	for(int nPos = 0; nPos < len; nPos ++)
	{
		unsigned opCode = readU1(byteCodeData,nPos);
		if(true == isReturnOperand(opCode))
		{
			bHasReturn = true;
			if((isInvokeCode(preOpcode) == true || isGetFieldCode(preOpcode) == true ||
			   isLdcCode(preOpcode) == true || isMethodOpCode(preOpcode) == true) 
			   && (opCode != opc_return  && opCode != opc_athrow))
			{
				bNeedAddLocal = true;
				if(nMaxLocal >= 4 && nMaxLocal < 256)
				{
					byteCodeLineChangeMap->insert(pair<int, int>(nPos, nInsertLength + 4));
				}
				else if(nMaxLocal < 4)
				{
					byteCodeLineChangeMap->insert(pair<int, int>(nPos, nInsertLength + 2));
				}
				else
				{
					byteCodeLineChangeMap->insert(pair<int, int>(nPos, nInsertLength + 8));
				}
			}
			else
			{
				byteCodeLineChangeMap->insert(pair<int, int>(nPos, nInsertLength));
			}
			
			if(opCode == opc_return || opCode == opc_athrow)
			{
				//returnと例外発生の場合、戻り値が必要がない
				continue;
			}
		}
		else if(true == isSwitchOperand(opCode))
		{
			getSwitchCodeAddedSize(nPos + 1,byteCodeLineChangeMap,nInsertLength);
		}
		
		else if(false == isTypeChangeCode(opCode))
		{
			nPrePos = nPos;
			preOpcode = opCode;
		}
		
		int nSkipLength = getOpCodeSkipLength(byteCodeData, nPos, opCode);
		nPos += nSkipLength;
	}
	return bHasReturn;
}

//Callbacksメソッド挿入したのバイトコードの位置
unsigned ClassFile::getCodePosAfterInsertCallbacks(unsigned oldPos, map<int, int>* byteCodeLineChangeMap, bool bBeforeEnd)
{
	unsigned newPos = oldPos;
	map<int, int>::iterator it = byteCodeLineChangeMap->begin();
	while(it != byteCodeLineChangeMap->end())
	{
		if(oldPos >= (*it).first)
		{
			//add by Qiu Song on 20091124 バグ：530
			if(bBeforeEnd == true && oldPos == (*it).first)
			{
				return newPos;
			}
			//end of add by Qiu Song on 20091124 for バグ：530
			newPos +=  (*it).second;
		}
		else
		{
			break;
		}
		++it;
	}
	return newPos;
}

//Callbacksメソッド挿入したのコード長さの増えた総数
unsigned ClassFile::getAddedSizeAfterInsertCallbacks(map<int, int>* byteCodeLineChangeMap)
{
	unsigned addSize = 0;
	map<int, int>::iterator it = byteCodeLineChangeMap->begin();
	while(it != byteCodeLineChangeMap->end())
	{
		addSize +=  (*it).second;
		++it;
	}
	return addSize;
}

//Callbacksを挿入した後、ジャンプ先を修正する
void ClassFile::modifyJumpTarget(unsigned char** outputPos,
								 unsigned opCode, 
								 unsigned char * byteCodeData,
								 int& nPos,
								 int& nNewPos,
								 map<int, int>* byteCodeLineChangeMap,
								 bool bBeforeEnd)
{
	//新データにジャンプコードの開始位置を取得する
	int nNewStartPos = getCodePosAfterInsertCallbacks(nPos-1,byteCodeLineChangeMap);;

	//opc_jsr_wとopc_goto_wの場合、ジャンプ先の桁数は4桁
	if(opCode == opc_jsr_w || opCode == opc_goto_w)
	{
		unsigned oldJumpTarget = readU4(byteCodeData, nPos);
		unsigned newJumpTarget = getCodePosAfterInsertCallbacks(oldJumpTarget+nPos-1,byteCodeLineChangeMap,bBeforeEnd);

		writeU4(*outputPos, newJumpTarget- nNewStartPos);
		nPos +=3;//外部loopも+1処理を行うため
		nNewPos +=3;
	}
	else
	{
		unsigned oldJumpTarget = readU2(byteCodeData, nPos);
		unsigned oldTargetPos = oldJumpTarget+nPos-1;
		if(oldJumpTarget > 32767)
		{
            oldTargetPos = oldTargetPos - 65536;
		}
		unsigned newJumpTarget = getCodePosAfterInsertCallbacks(oldTargetPos,byteCodeLineChangeMap,bBeforeEnd);
		writeU2(*outputPos, newJumpTarget- nNewStartPos);
		nPos +=1;//外部loopも+1処理を行うため
		nNewPos +=1;
	}
}

//Callbacksを挿入した後、switchコードのジャンプ先を修正する
void ClassFile::modifySwitchTarget(unsigned char** outputPos,
								 unsigned opCode, 
								 unsigned char * byteCodeData,
								 int& nPos,
								 int& nNewPos,
								 map<int, int>* byteCodeLineChangeMap,
								 bool bBeforeEnd)
{
	//新データにジャンプコードの開始位置を取得する
	int nStartPos = nPos - 1;
	int nNewStartPos = getCodePosAfterInsertCallbacks(nStartPos,byteCodeLineChangeMap);
	
	//共通部分の処理を行う
	modifySwitchCommon(outputPos, opCode, byteCodeData, nPos, nNewPos, nStartPos,
		               nNewStartPos, byteCodeLineChangeMap,bBeforeEnd);

	//lookupswitch文のジャンプ先の修正
	if(opCode == opc_lookupswitch)
	{
		modifyLookupSwitch(outputPos, opCode, byteCodeData, nPos, nNewPos, nStartPos,
		                   nNewStartPos, byteCodeLineChangeMap,bBeforeEnd);
	}

	//tableswitch文のジャンプ先の修正
	else if(opCode == opc_tableswitch)
	{
		modifyTableSwitch(outputPos, opCode, byteCodeData, nPos, nNewPos, nStartPos,
		                  nNewStartPos, byteCodeLineChangeMap,bBeforeEnd);
	}
}

//Callbacksを挿入した後、switchコード共通部分の修正関数
void ClassFile::modifySwitchCommon(unsigned char** outputPos,
								 unsigned opCode, 
								 unsigned char * byteCodeData,
								 int& nPos,
								 int& nNewPos,
								 int nStartPos,
								 int nNewStartPos,
								 map<int, int>* byteCodeLineChangeMap,
								 bool bBeforeEnd)
{

	//旧バイトコードの固定値0の桁数を計算する
	int nMode = nPos % 4;
	if(0 != nMode)
	{
		nPos += (4-nMode);
	}

	//新バイトコードの固定値0の桁数を計算する
    nMode = nNewPos % 4;
    if(0 != nMode)
	{
		int nCount = 4 - nMode;
		for(int i = 0; i < nCount; i++)
		{
			writeU1(*outputPos, 0);
			nNewPos++;
		}
	}

	//default文のジャンプ先を呼び出す
	unsigned oldJumpTarget = readU4(byteCodeData, nPos);
	unsigned newJumpTarget = getCodePosAfterInsertCallbacks(oldJumpTarget+ nStartPos,byteCodeLineChangeMap,bBeforeEnd);
	writeU4(*outputPos, newJumpTarget - nNewStartPos);
	nNewPos += 4;
	nPos +=4;
}

//Callbacksを挿入した後、lookupswitchコードのジャンプ先を修正する関数
/******************************************************************************
 lookupswitch文の構造：
	 固定値：     lookupswitch         1桁
	 固定値：     0                    0～3桁(default文ジャンプ先の開始位置は必ず4の倍数)
	 変数：       deafult文ジャンプ先  4桁
	 変数：       case個数             4桁
	 組み合わせ： case情報             8桁
		              caseデータ       4桁
					  ジャンプ先       4桁
 ******************************************************************************/
void ClassFile::modifyLookupSwitch(unsigned char** outputPos,
								 unsigned opCode, 
								 unsigned char * byteCodeData,
								 int& nPos,
								 int& nNewPos,
								 int nStartPos,
								 int nNewStartPos,
								 map<int, int>* byteCodeLineChangeMap,
								 bool bBeforeEnd)
{
	//case個数を取得する
	unsigned nCaseCount = readU4(byteCodeData, nPos);
	writeU4(*outputPos, nCaseCount);

	//ジャンプ先を修正する
	for(int i = 0; i < nCaseCount; i++)
	{	//caseデータを取得する
		nNewPos += 4;
		nPos +=4;
		unsigned oldData = readU4(byteCodeData, nPos);
		writeU4(*outputPos, oldData);
		
		//ジャンプ先を取得する
		nNewPos += 4;
		nPos +=4;
		unsigned oldJumpTarget = readU4(byteCodeData, nPos);
		unsigned newJumpTarget = getCodePosAfterInsertCallbacks(oldJumpTarget+nStartPos,byteCodeLineChangeMap,bBeforeEnd);
		writeU4(*outputPos, newJumpTarget - nNewStartPos);
	}

	//最後の位置の調整(外部のloopに+1操作を行うため)
	nNewPos += 3;
	nPos +=3;
}

//Callbacksを挿入した後、tableswitchコードのジャンプ先を修正する関数
/******************************************************************************
 lookupswitch文の構造：
	 固定値：     tableswitch         1桁
	 固定値：     0                    0～3桁(default文ジャンプ先の開始位置は必ず4の倍数)
	 変数：       deafult文ジャンプ先  4桁
	 開始値：     開始値               4桁
	 終了値：     終了値               4桁
	 組み合わせ： case情報             4桁
		              caseデータ       なし
					  ジャンプ先       4桁
 ******************************************************************************/
void ClassFile::modifyTableSwitch(unsigned char** outputPos,
								 unsigned opCode, 
								 unsigned char * byteCodeData,
								 int& nPos,
								 int& nNewPos,
								 int nStartPos,
								 int nNewStartPos,
								 map<int, int>* byteCodeLineChangeMap,
								 bool bBeforeEnd)
{
	//開始値を取得する
	unsigned nStartValue = readU4(byteCodeData, nPos);
	writeU4(*outputPos, nStartValue);

	//終了値を取得する
	nNewPos += 4;
	nPos +=4;
	unsigned nEndValue = readU4(byteCodeData, nPos);
	writeU4(*outputPos, nEndValue);

	int nCaseCount = nEndValue - nStartValue +1;
	
	//ジャンプ先を修正する
	for(int i = 0; i < nCaseCount; i++)
	{
		//ジャンプ先を取得する
		nNewPos += 4;
		nPos +=4;
		unsigned oldJumpTarget = readU4(byteCodeData, nPos);
		unsigned newJumpTarget = getCodePosAfterInsertCallbacks(oldJumpTarget+nStartPos,byteCodeLineChangeMap,bBeforeEnd);
		writeU4(*outputPos, newJumpTarget - nNewStartPos);
	}

	//最後の位置の調整(外部のloopに+1操作を行うため)
	nNewPos += 3;
	nPos +=3;
}

//修正したバイトコードにswitch文の追加した桁数を取得する関数(負数の可能性もあります)
void ClassFile::getSwitchCodeAddedSize(int nPos, map<int,int>* byteCodeLineChangeMap, int nInsertLength)
{
	int nSwitchCodeAddedSize = 0;

	//旧バイトコードの固定値0の桁数を計算する
	int nMode = nPos % 4;
	if(0 != nMode)
	{
		nSwitchCodeAddedSize -= (4-nMode);
	}

	//新バイトコードの固定値0の桁数を計算する
	int nNewPos = getCodePosAfterInsertCallbacks(nPos, byteCodeLineChangeMap);
	nMode = nNewPos % 4;
    if(0 != nMode)
	{
		nSwitchCodeAddedSize += (4-nMode);
	}

	byteCodeLineChangeMap->insert(pair<int, int>(nPos,nSwitchCodeAddedSize));
}

//リターンコードががどうか判断関数
bool ClassFile::isReturnOperand(unsigned opCode)
{
	//returnとathrowは対応対象外になりますので、判断しない
	if(opCode == opc_areturn || opCode == opc_dreturn || opCode == opc_freturn ||
	   opCode == opc_ireturn || opCode == opc_lreturn || opCode == opc_return  ||
	   opCode == opc_athrow)
	{
		return true;
	}
	return false;
}

//ジャンプコードがどうか判断関数
bool ClassFile::isJumpOperand(unsigned opCode)
{
	if(opCode == opc_jsr || opCode == opc_jsr_w || opCode == opc_goto ||
	   opCode == opc_goto_w || opCode == opc_ifeq || opCode == opc_ifne ||
	   opCode == opc_ifle || opCode == opc_iflt || opCode == opc_ifge ||
	   opCode == opc_ifgt || opCode == opc_ifnull || opCode == opc_ifnonnull ||
	   opCode == opc_if_icmpeq || opCode == opc_if_icmpne || opCode == opc_if_icmple ||
	   opCode == opc_if_icmplt || opCode == opc_if_icmpge || opCode == opc_if_icmpgt ||
	   opCode == opc_if_acmpeq || opCode == opc_if_acmpne)
	{
		return true;
	}
	return false;
}

//タイプ変更コードがどうか判断関数
bool ClassFile::isTypeChangeCode(unsigned opCode)
{
	if(opCode == opc_i2l || opCode == opc_i2f || opCode == opc_i2d || 
	   opCode == opc_l2i || opCode == opc_l2f || opCode == opc_l2d ||
	   opCode == opc_f2i || opCode == opc_f2l || opCode == opc_f2d ||
	   opCode == opc_d2i || opCode == opc_d2l || opCode == opc_d2f ||
	   opCode == opc_i2b || opCode == opc_i2c || opCode == opc_i2s ||
	   opCode == opc_checkcast || opCode == checkcast_quick)
	{
		return true;
	}
	return false;
}

//Invokeまたはxaloadコードかどうか判断関数
bool ClassFile::isInvokeCode(unsigned opCode)
{
	if(opCode == opc_invokevirtual    || opCode == opc_invokespecial   ||
	   opCode == opc_invokestatic     || opCode == opc_invokeinterface ||
	   opCode == opc_iaload           || opCode == opc_laload          ||
	   opCode == opc_faload           || opCode == opc_daload          ||
	   opCode == opc_aaload           || opCode == opc_baload          ||
	   opCode == opc_caload           || opCode == opc_saload          ||
	   opCode == opc_anewarray        || opCode == opc_multianewarray  ||
	   opCode == opc_newarray         || opCode == anewarray_quick     ||
	   opCode == multianewarray_quick || opCode == opc_arraylength  ||
	   opCode == opc_instanceof       ||  opCode == instanceof_quick)
	{
		return true;
	}
	return false;
}

//ldcコードがどうか判断関数
bool ClassFile::isLdcCode(unsigned opCode)
{
	if(opCode == opc_ldc     || opCode == ldc_quick  || 
	   opCode == opc_ldc_w   || opCode == opc_ldc2_w ||
	   opCode == ldc_w_quick || opCode == ldc2_w_quick)
	{
		return true;
	}
	return false;
}

//getfieldコードがどうか判断関数
bool ClassFile::isGetFieldCode(unsigned opCode)
{
	if(opCode == opc_getfield    || opCode == getfield_quick   ||
	   opCode == getfield2_quick || opCode == getfield_quick_w ||
	   opCode == opc_getstatic   || opCode == getstatic_quick  ||
	   opCode == getstatic2_quick)
	{
		return true;
	}
	return false;
}

//運算コード稼動か判断関数
bool ClassFile::isMethodOpCode(unsigned opCode)
{
  if( opCode >= opc_iadd && opCode <= opc_iinc )
  {
	  return true;
  }
  return false;
}

//switchコードがあるかどうか探す
bool ClassFile::isSwitchOperand(unsigned opCode)
{
	if(opCode == opc_lookupswitch || opCode == opc_tableswitch)
	{
		return true;
	}
	return false;
}

//1桁数のバイトコードを呼び出す
unsigned ClassFile::readU1(unsigned char * byteCodeData, int pos)
{
	return (((unsigned)byteCodeData[pos]) & 0xFF);
}

//2桁数のバイトコードを呼び出す
unsigned ClassFile::readU2(unsigned char * byteCodeData, int pos)
{
	 unsigned res;
	 res = readU1(byteCodeData, pos);
	 return ((res<<8) + readU1(byteCodeData, pos+1));
}

//4桁数のバイトコードを呼び出す
unsigned ClassFile::readU4(unsigned char * byteCodeData, int pos)
{
	unsigned res;
	res = readU2(byteCodeData, pos);
    return ((res <<16) + readU2(byteCodeData, pos+2));
}

//8桁数のバイトコードを呼び出す
long ClassFile::readLongInfo(unsigned char * byteCodeData, int pos)
{
	long highByte = readU4(byteCodeData,0);
	long lowByte  = readU4(byteCodeData,4);
    if(highByte != 0)
	{
		lowByte = (-1)*(~lowByte +1);
	}
	return lowByte;
}

//バイトコードにcallbacksメソッドを挿入する
void ClassFile::insertCallbacksFunc(unsigned char** outputPos, int paramIndex_forMethod, int methodIndex_forMethod, int& nInsertLength)
{
	//add ldc/ldc_w
	if (paramIndex_forMethod > 255)
	{
		writeU1(*outputPos, opc_ldc_w);
		writeU2(*outputPos, paramIndex_forMethod);
		nInsertLength = 6;
	}
	else
	{
		writeU1(*outputPos, opc_ldc);
		writeU1(*outputPos, (U1)paramIndex_forMethod); 
		nInsertLength = 5;
	}

	//add invokestatic instruction
	writeU1(*outputPos, opc_invokestatic);
	writeU2(*outputPos, methodIndex_forMethod);
}

//リターンコード前に一つの一時変数を追加する
void ClassFile::insertLocalParamBeforeReturn(unsigned char** outputPos, unsigned opCode,
											 int attributeMaxLocal, int &nNewPos)
{
	unsigned storeCode = -1;
	unsigned loadCode = -1;
	getStoreAndCode(opCode, attributeMaxLocal, storeCode, loadCode);
	if(attributeMaxLocal >= 4 && attributeMaxLocal <= 255)
	{
		writeU1(*outputPos, storeCode);
		writeU1(*outputPos, (U1)attributeMaxLocal);
		writeU1(*outputPos, loadCode);
		writeU1(*outputPos, (U1)attributeMaxLocal);
   	    nNewPos += 4;
	}
	else if(attributeMaxLocal < 4)
	{
		writeU1(*outputPos, storeCode);
		writeU1(*outputPos, loadCode);
    	nNewPos += 2;
	}
	else
	{
		writeU1(*outputPos, opc_wide);
		writeU1(*outputPos, storeCode);
		writeU2(*outputPos, (U2)attributeMaxLocal);
		writeU1(*outputPos, opc_wide);
		writeU1(*outputPos, loadCode);
		writeU2(*outputPos, (U2)attributeMaxLocal);
   	    nNewPos += 8;
	}
}

//リターンコード前に挿入したstoreコードを取得する
void ClassFile::getStoreAndCode(unsigned opCode, int attributeMaxLocal, unsigned& storeCode, unsigned& loadCode)
{
	if(attributeMaxLocal >= 4)
	{
		switch(opCode)
		{
		case opc_ireturn:
			storeCode = opc_istore;
			loadCode = opc_iload;
			return;
		case opc_lreturn:
			storeCode = opc_lstore;
			loadCode = opc_lload;
			return;
		case opc_freturn:
			storeCode = opc_fstore;
			loadCode = opc_fload;
			return;
		case opc_dreturn:
			storeCode = opc_dstore;
			loadCode = opc_dload;
			return;
		case opc_areturn:
			storeCode = opc_astore;
			loadCode = opc_aload;
			return;
		}
	}
	else
	{
		switch(opCode)
		{
		case opc_ireturn:
			storeCode = (opc_istore_0 + attributeMaxLocal);
			loadCode = (opc_iload_0 + attributeMaxLocal);
			return ;
		case opc_lreturn:
			storeCode = (opc_lstore_0 + attributeMaxLocal);
			loadCode = (opc_lload_0 + attributeMaxLocal);
			return;
		case opc_freturn:
			storeCode = (opc_fstore_0 + attributeMaxLocal);
			loadCode = (opc_fload_0 + attributeMaxLocal);
			return;
		case opc_dreturn:
			storeCode = (opc_dstore_0 + attributeMaxLocal);
			loadCode = (opc_dload_0 + attributeMaxLocal);
			return;
		case opc_areturn:
			storeCode = (opc_astore_0 + attributeMaxLocal);
			loadCode = (opc_aload_0 + attributeMaxLocal);
			return;
		}
	}
	return;
}

//次のopCodeへ桁数を取得する
int ClassFile::getOpCodeSkipLength(unsigned char * byteCodeData, int nPos,unsigned opcode)
{
	int nTempPos,high,low,nMode,nPairs;
	/* Process this opcode */
	switch (opcode)
	{
		//wide operator
		case opc_wide:
			opcode = readU1(byteCodeData, nPos + 1);  //get real operator
			if(opcode == opc_iinc)
			{  //if iinc skip the next two bytes
			   return 5;
			}
			return 3;
		case opc_tableswitch:
			nTempPos = nPos + 1;
			nMode = nTempPos % 4;
			if(0 != nMode)
			{
				nTempPos += (4-nMode);
			}
			//skip 4 bytes
			nTempPos += 4;
			low = readU4(byteCodeData, nTempPos);
			nTempPos += 4;
			high = readU4(byteCodeData, nTempPos);
			nTempPos +=4;
			nTempPos += (high+1-low) * 4;
			return (nTempPos - nPos - 1);
		case opc_lookupswitch:
			nTempPos = nPos + 1;
			nMode = nTempPos % 4;
			if(0 != nMode)
			{
				nTempPos += (4-nMode);
			}
			nTempPos +=4;
			nPairs = readU4(byteCodeData,nTempPos);
			nTempPos += 4;
			nTempPos +=  nPairs * 8;
			return (nTempPos - nPos - 1);
		default:
			return opcode_length(opcode) - 1;
	}
}

//opCodeの桁数を取得する
int ClassFile::opcode_length(unsigned opcode)
{
    /* Define array that holds length of an opcode */
    static unsigned char _opcode_length[opc_MAX+1] = 
			  JVM_OPCODE_LENGTH_INITIALIZER;
    if(opcode >  opc_MAX || _opcode_length[opcode]==0)
	{
		char logBuf[MAX_BUF_LEN];
		sprintf(logBuf,"unknown opcode:%d",opcode);
		LOG4CXX_DEBUG(Global::logger, logBuf);
		return -1;
	}
	else
	{
		return _opcode_length[opcode];
	}
}

//メソッド実行前の監視するの場合、callbacksを挿入した後に、残りバイトコードのコピー処理
bool ClassFile::copyTheLeftCode(unsigned char** outputPos,
								unsigned char * byteCodeData, 
								int len,
								int nInsertLength,
								map<int,int>* byteCodeLineChangeMap,
								int& nNewPos)
{
	//switchがあるの場合
	if(findSwitchPos(byteCodeData, len, nInsertLength, byteCodeLineChangeMap) == true)
	{
		copyByteCodeWithSwitch(outputPos, nInsertLength, byteCodeData,
							   len, byteCodeLineChangeMap, nNewPos);
		return true;
	}
	return false;
}

//バイトコードにswitchコードを探す(メソッド実行前の監視機能専用)
bool ClassFile::findSwitchPos(unsigned char * byteCodeData, 
							  int len,
							  int nInsertLength,
							  map<int,int>* byteCodeLineChangeMap)
{
	bool bHasSwitch = false;

	for(int nPos = 0; nPos < len; nPos ++)
	{
		unsigned opCode = readU1(byteCodeData,nPos);
		if(true == isSwitchOperand(opCode))
		{
			getSwitchCodeAddedSize(nPos + 1,byteCodeLineChangeMap,nInsertLength);
			bHasSwitch = true;
		}
		int nSkipLength = getOpCodeSkipLength(byteCodeData, nPos, opCode);
		nPos += nSkipLength;
	}
	return bHasSwitch;
}

//メソッドを終了する前にcallbacksメソッドを追加する
void ClassFile::copyByteCodeWithSwitch(unsigned char** outputPos,
									   int nInsertLength,
									   unsigned char * byteCodeData, 
									   int len,
									   map<int,int>* byteCodeLineChangeMap,
									   int& nNewPos)
{
	//一時変数初期化処理
	unsigned char * tempData = new unsigned char[len+1];
	memset(tempData, 0, len+1);
	memcpy(tempData, byteCodeData, len);


	//バイトコードにreturnコードがあるの場合
	for(int nPos = 0; nPos < len; nPos ++,nNewPos++)
	{
		unsigned opCode = readU1(tempData, nPos);
		
		//ジャンプコードの場合、ジャンプ先を再調整する
		if(isJumpOperand(opCode))
		{
			//ジャンプコードを書き込む
			writeU1(*outputPos, opCode);
			nPos++;
			nNewPos++;
			
			//ジャンプ先を修正する
			modifyJumpTarget(outputPos, opCode, tempData, nPos, nNewPos, byteCodeLineChangeMap);
		}

		//switchコードの場合、ジャンプ先を再調整する
		else if(isSwitchOperand(opCode))
		{
			writeU1(*outputPos, opCode);
			nPos++;
			nNewPos++;

			//ジャンプ先を修正する
			modifySwitchTarget(outputPos, opCode, tempData, nPos, nNewPos, byteCodeLineChangeMap);
		}
		else
		{
			writeU1(*outputPos, opCode);
			int nSkipLength = getOpCodeSkipLength(tempData, nPos, opCode);
			for(int i = 0; i < nSkipLength; i++)
			{
				nPos ++;
				nNewPos ++;
				opCode = readU1(tempData, nPos);
				writeU1(*outputPos, opCode);
			}
		}
	}
	delete[] tempData; 
}

//Callbacksメソッド挿入したの各テーブルの新位置を取得する
unsigned ClassFile::getTablePosAfterInsertCallbacks(unsigned oldPos, map<int, int>* byteCodeLineChangeMap, int size, int nInsertLength)
{
	unsigned newPos = oldPos + size;
	if( byteCodeLineChangeMap->size() != 0 )
	{
		newPos = getCodePosAfterInsertCallbacks(oldPos,	byteCodeLineChangeMap) + size - nInsertLength;
	}
	return newPos;
}
void ClassFile::getConstantClassName(int cpIndex, char *buf)
{
     CpInfo *classRef = *(this->constantPool + cpIndex);
     int classNameIndex = getU2ReverseValue((U2*)classRef->info);
     getUtf8ConstantInfo(classNameIndex, buf);
}

int ClassFile::findUtf8InfoConstantPos(char* searchValue)
{
	U2 constPoolCount = ClassFile::getU2ReverseValue(this->constantPoolCount);
    bool skip = false;
	for(int i=1 ; i < constPoolCount; i ++)
	{
		if(skip == true)
		{
			skip = false;
			continue;
		}
		CpInfo* cpInfo = constantPool[i];
		if(unsigned(cpInfo->tag[0]) != JVM_CONSTANT_Utf8)
		{
			if(cpInfo->isLargeNumericType())
			{
				skip = true;
			}
			continue;
		}
        char* constantValue = NULL;
		this->getUtf8ConstantInfo(i, constantValue);
		if(constantValue != NULL && strcmp(constantValue, searchValue) == 0)
		{
			delete constantValue;
			return i;
		}
		delete constantValue;
	}
	if(addedConstantUtf8Info != NULL)
	{
		map<int,string>::iterator it = addedConstantUtf8Info->begin();
		while(it != addedConstantUtf8Info->end())
		{
			string utf8Value = (*it).second;
			if(strcmp(searchValue, utf8Value.c_str()) == 0)
			{
				return  (*it).first;
			}
			++it;
		}
	}
	return -1;
}

void ClassFile::addMethodReturnTypeToConstantPool(unsigned char*& outputPos, unsigned char* pConstantPoolCountPos, int& nConstantPoolCount)
{
	this->addUtf8CpoolEntry(outputPos, METHOD_RETURN_VALUE);
	int methCount = getU2ReverseValue(this->methodsCount);
	for (int i = 0; i < methCount; i++) 
	{
		MethodInfo *methInfo = NULL;
		char descriptor[MAX_BUF_LEN];
		methInfo = *(this->methods + i);
		this->getUtf8ConstantInfo(getU2ReverseValue(methInfo->descriptorIndex), descriptor);//メソッド解析文
		char* returnType = HelperFunc::getReturnTypeFromMethodSig(descriptor);
		if(strcmp(returnType, "V") != 0)
		{
			if(findUtf8InfoConstantPos(returnType) == -1)
			{
				addUtf8CpoolEntry(outputPos, returnType);
				nConstantPoolCount ++;
				if(addedConstantUtf8Info == NULL)
				{
					addedConstantUtf8Info = new map<int, string>;
				}
				addedConstantUtf8Info->insert(pair<int, string>(nConstantPoolCount -  1, returnType));
			}
		}
	}

	//constant poolの個数をリセットする
    writeU2(pConstantPoolCountPos, nConstantPoolCount);
}
//end of add by Qiu Song on 20090812 for メソッド監視



