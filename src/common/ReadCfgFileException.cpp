#include <string.h>
#include "HelperFunc.h"
#include "ReadCfgFileException.h"

USE_NS(NS_COMMON)
ReadCfgFileException::ReadCfgFileException(const char *msg)
{
    if(msg == NULL)
	{
       errorMsg = NULL;
	}else
    {
		errorMsg = HelperFunc::strdup(msg);
    }

}

ReadCfgFileException::~ReadCfgFileException() throw()
{
	if(errorMsg != NULL)
	{
		delete errorMsg;
		errorMsg = NULL;
	}
}
char * ReadCfgFileException::getErrorMsg()
{
	return errorMsg;
}

