#ifndef CcatFormatException_h_
#define CcatFormatException_h_

#include <stdio.h>
#include <string>
#include "../common/Define.h"

using namespace std;

BEGIN_NS(NS_ANTLRPARSE)
class CcatFormatException
{
public:
	CcatFormatException(const char* msg, int line, int column);
	virtual ~CcatFormatException(){};

	const char* getErrorMsg(){return errorMsg.c_str();};

private:
	string errorMsg;
};
END_NS
#endif /*CcatFormatException_h_*/
