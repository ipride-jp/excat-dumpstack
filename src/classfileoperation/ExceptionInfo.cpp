#pragma warning(disable : 4503)
#pragma warning(disable : 4786)

#include <map>
#include <vector>
#include <string>

#include "../jniutility/ExceptionTableMan.h"
//#include "ExceptionInfo.h"
#include "ClassFile.h"
#include "../common/Define.h"


USE_NS(NS_CLASS_FILE_OPERATION)

ExceptionInfo::ExceptionInfo(U1* beginPos,ClassFile* pClassFile ):start_pc(0),
    end_pc(0),handler_pc(0),catch_type(NULL)
{
    //read start_pc
    start_pc = ClassFile::getU2ReverseValue((U2*)beginPos);
	beginPos += sizeof(U2);
	
	//read end_pc
	end_pc = ClassFile::getU2ReverseValue((U2*)beginPos);
	beginPos += sizeof(U2);

    //read handler_pc
	handler_pc =ClassFile::getU2ReverseValue((U2*)beginPos);
    beginPos += sizeof(U2);

	//read catch_type
	U2 typePtr = ClassFile::getU2ReverseValue((U2*)beginPos);;
	if(typePtr != 0)
	{
		char buf[MAX_BUF_LEN];
		pClassFile->getConstantClassName(typePtr,buf);
        catch_type = new char[strlen(buf) + 1];
		strcpy(catch_type,buf);
	}
}

ExceptionInfo::ExceptionInfo(ExceptionInfo* other)
{
     start_pc = other->start_pc;
	 end_pc = other->end_pc;
	 handler_pc = other->handler_pc;
	 if(other->catch_type == NULL){
         catch_type = NULL;
	 }else{
         catch_type = new char[strlen(other->catch_type) + 1];
		 strcpy(catch_type,other->catch_type);
	 }
}


ExceptionInfo::~ExceptionInfo()
{
	if(catch_type != NULL)
	{
		delete catch_type;
		catch_type = NULL;
	}
}


bool ExceptionInfo::isHandledException(const char*exceptionName,int location)
{
	if(catch_type == NULL){
		return false;
	}

	if(start_pc <= location && end_pc > location){
		if(strcmp(catch_type,exceptionName) == 0){
			return true;
		}else{
			return false;
		}
	}else{
		return false;
	}
}


