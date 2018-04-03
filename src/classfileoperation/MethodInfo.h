#if !defined(_METHODINFO_H)
#define _METHODINFO_H

#include "BaseInfo.h"

BEGIN_NS(NS_CLASS_FILE_OPERATION)
class ClassFile;
class AttributeInfo;

class MethodInfo : public BaseInfo
{
public:
	MethodInfo(U1 *pos,ClassFile* pClassFile);
	~MethodInfo();

	AttributeInfo *getCodeAttribute(ClassFile *classFile);
private:
	//Exception�e�[�u���̏��
    ExceptionInfo** exceptions;
	U2 exceptionTableNum;
	char* methodName;
	char* methodSignature;
	//Exception�e�[�u���̏����擾
	void getExceptionInfo(U1* beginPos,ClassFile* pClassFile);
public:
	U2 *accessFlags;
	U2 *nameIndex;
	U2 *descriptorIndex;
	U2 *attributesCount;
	AttributeInfo **attributes;
};

typedef MethodInfo* MethodInfoPtr;
END_NS

#endif

