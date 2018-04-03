#ifndef _READCFGFILEEXCEPTION_H
#define _READCFGFILEEXCEPTION_H

#include <exception>
#include <cstdio>

BEGIN_NS(NS_COMMON)
class  ReadCfgFileException: public std::exception  {

public:
	ReadCfgFileException(const char *msg);

    virtual ~ReadCfgFileException() throw();
    char *getErrorMsg();
    const char* what() throw(){
		return errorMsg;
	};
private:
	char* errorMsg;
};

END_NS

#endif

