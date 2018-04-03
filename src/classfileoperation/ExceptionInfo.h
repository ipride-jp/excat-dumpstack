#ifndef _EXCEPTION_INFO_H_
#define _EXCEPTION_INFO_H_

#include "BaseInfo.h"

BEGIN_NS(NS_CLASS_FILE_OPERATION)

class ClassFile;

class ExceptionInfo:public BaseInfo
{
	public :
        ExceptionInfo(U1* beginPos,ClassFile* pClassFile);
        ExceptionInfo(ExceptionInfo* other);
		~ExceptionInfo();

	    bool isHandledException(const char*exceptionName,int location);
	private:
		int start_pc;
		int end_pc;
		int handler_pc;
		char* catch_type;
};

typedef ExceptionInfo* ExceptionInfoPtr;

END_NS

#endif //_EXCEPTION_INFO_H_

