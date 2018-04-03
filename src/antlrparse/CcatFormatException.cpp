#include "CcatFormatException.hpp"
#include "../common/Define.h"

USE_NS(NS_ANTLRPARSE)

CcatFormatException::CcatFormatException(const char* msg, int lineNum, int columnNum)
{
	char szTmp[128+ 1];
	SNPRINTF(szTmp, 128, "line%d:%d: %s", lineNum, columnNum, msg);
	errorMsg = szTmp;
}
