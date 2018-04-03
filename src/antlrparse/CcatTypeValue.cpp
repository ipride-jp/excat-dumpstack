#include "CcatTypeValue.hpp"
#include "../common/Global.h"
#include "../common/Define.h"
#include "../common/HelperFunc.h"
#include "../common/JvmUtilFunc.h"
#include "../common/JvmtiAutoRelease.h"
#include "../common/JniLocalRefAutoRelease.h"
#include "CcatParser.hpp"
#include "../jniutility/AgentCallbackHandler.h"

USE_NS(NS_ANTLRPARSE)
USE_NS(NS_COMMON)
USE_NS(NS_JNI_UTILITY)

#define GET_MIN(x,y)  (x>y)?y:x

#if defined(_WINDOWS) || defined(_EXCAT_HPUX)
#define ANTLR_LLONG_MIN       (-9223372036854775807-1)
#define ANTLR_LLONG_MAX       9223372036854775807

__int64 strtoll(const char *p,char **pend,int base) 
{ 
	__int64 number = 0; 
	int positive,c; 
	int error; 
	const char *pstart; 
	int anytrans = 0; 
	
	pstart = p; 
	
	switch (*p) 
	{   
	case '-': 
		positive = 0; 
		p++; 
		break; 
	case '+': 
		p++; 
		// FALL-THROUGH 
	default: 
		positive = 1; 
		break; 
	}
	
	switch (base) 
	{
	case 0: 
		base = 10;  // assume decimal base 
		if (*p == '0') 
		{
			base = 8; // could be octal 
			p++; 
			anytrans++; // count 0 as a translated char 
			switch (*p) 
			{
			case 'x': 
			case 'X': 
				base = 16; // hex 
				p++; 
				anytrans--; // oops, the previous 0 was part of a 0x, now look for valid chars 
				break; 
			} 
		} 
		break; 
	case 16:   // skip over '0x' and '0X' 
		if (*p == '0' && (p[1] == 'x' || p[1] == 'X')) 
			p += 2; 
		break; 
	} 
	error = 0; 
	while (1) 
	{   
		c = *p; 
		if (c >= '0' && c <= '9') 
			c -= '0'; 
		else if (c >= 'a' && c <= 'z') 
			c = c - ('a' - 10); 
		else if (c >= 'A' && c <= 'Z') 
			c = c - ('A' - 10); 
		else 
		{   // unrecognized character 
			// Go back to beginning of string if nothing 
			// was dealt with properly 
			if (anytrans == 0) 
				p = pstart; 
			break; 
		} 
		if (c >= base)  // not in number base 
		{ 
			// Go back to beginning of string if no characters 
			// were successfully dealt with 
			if (anytrans == 0) 
				p = pstart; 
			break; 
		} 
		if ((ANTLR_LLONG_MIN + c) / base > number) 
			error = 1; 
		number = number * base - c; 
		p++; 
		anytrans++; 
	} 
	if (pend) 
		*pend = (char *) p; 
	if (positive && number == ANTLR_LLONG_MIN || error) 
	{ 
	/*  a range error has occurred, set errno and return 
	LLONG_MAX or LLONG_MIN dependant upon positive. 
	I.e. errange on a negative, means it was under 
	LLONG_MIN, on a positive, was above LLONG_MAX 
		*/ 
		number = (positive) ? ANTLR_LLONG_MAX :ANTLR_LLONG_MIN; 
		return number; 
	} 
	return (positive) ? -number : number; 
}
#endif

CcatTypeValue::CcatTypeValue()
: type(CTYPE_NONE)
, boolValue(false)
, utf8CharValue(NULL)
, numberValue(0)
, floatValue(0)
, doubleValue(0)
, stringValue("")
, objectValue(NULL)
, hasError(false)
, errorMsg(NULL)
{
}

CcatTypeValue::CcatTypeValue(const CcatTypeValue& copy)
{
    type = copy.type;
    boolValue = copy.boolValue;
    numberValue = copy.numberValue;
    floatValue = copy.floatValue;
    doubleValue = copy.doubleValue;
    stringValue = copy.stringValue;
    //objectValue = copy.objectValue;
	if (copy.objectValue != NULL)
		objectValue = CcatParser::jvmEnv->jni->NewLocalRef(copy.objectValue);
	else
		objectValue = NULL;
	hasError = copy.hasError;
	utf8CharValue = NULL;
	if (copy.utf8CharValue != NULL)
	{
		utf8CharValue = new char[strlen(copy.utf8CharValue)+1];
		strcpy(utf8CharValue, copy.utf8CharValue);
	}
	errorMsg = NULL;
	if (copy.errorMsg != NULL)
	{
		errorMsg = new char[MAX_MSG_LEN+1];
		strncpy(errorMsg, copy.errorMsg, MAX_MSG_LEN);
	}
}

CcatTypeValue::~CcatTypeValue()
{
	if (utf8CharValue != NULL)
	{
		delete[] utf8CharValue;
	}
    if (errorMsg != NULL)
		delete[] errorMsg;

	if (objectValue != NULL)
	{
        AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objectValue);
		objectValue = NULL;
	}
}

CcatTypeValue& CcatTypeValue::operator=(const CcatTypeValue& copy)
{
    type = copy.type;
    boolValue = copy.boolValue;
    numberValue = copy.numberValue;
    floatValue = copy.floatValue;
    doubleValue = copy.doubleValue;
    stringValue = copy.stringValue;
    //objectValue = copy.objectValue;
	if (copy.objectValue != NULL)
		objectValue = CcatParser::jvmEnv->jni->NewLocalRef(copy.objectValue);
	else
		objectValue = NULL;
	hasError = copy.hasError;
	utf8CharValue = NULL;
	if (copy.utf8CharValue != NULL)
	{
		utf8CharValue = new char[strlen(copy.utf8CharValue)+1];
		strcpy(utf8CharValue, copy.utf8CharValue);
	}
	errorMsg = NULL;
	if (copy.errorMsg != NULL)
	{
		errorMsg = new char[MAX_MSG_LEN+1];
		strncpy(errorMsg, copy.errorMsg, MAX_MSG_LEN);
	}
	return *this;
}

void CcatTypeValue::setErrorMsg(const char* msg)
{
	hasError = true;
	if (msg == NULL)
	{
		if (errorMsg != NULL)
		{
			delete [] errorMsg;
			errorMsg = NULL;
		}
		return;
	}
    if (errorMsg == NULL)
	{
		errorMsg = new char[MAX_MSG_LEN+1];
	}
    memset(errorMsg, 0, MAX_MSG_LEN+1);
	if (strlen(msg) > 0)
	{
		strncpy(errorMsg, msg, GET_MIN(strlen(msg), MAX_MSG_LEN));
	}
	if (objectValue != NULL)
	{
        AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objectValue);
		objectValue = NULL;
	}
}


/*
 * 
 */
CcatTypeValue CcatTypeValue::relationValue(CcatTypeValue& op, bool justCompareEquale, 
										   bool canEqual, char* operatorStr) const
{
	bool isTypeMismatch = false;

	CcatTypeValue ret(*this);
    switch (ret.type)
	{
	case CTYPE_OBJECT_BYTE:
	case CTYPE_OBJECT_SHORT:
	case CTYPE_OBJECT_INTEGER:
	case CTYPE_OBJECT_LONG:
		if (justCompareEquale && (op.type == CTYPE_OBJECT_NULL))
		{
			ret.boolValue = false;
			break;
		}
        ret.getObjectValue();
		if (ret.hasError)
		{
			return ret;
		}
	case CTYPE_NUMBER:
		switch (op.type)
		{
		case CTYPE_NUMBER:
			if (justCompareEquale)
				ret.boolValue = ret.numberValue == op.numberValue;
			else if (canEqual)
				ret.boolValue = ret.numberValue >= op.numberValue;
			else
				ret.boolValue = ret.numberValue > op.numberValue;
			break;
		case CTYPE_FLOAT:
			if (justCompareEquale)
				ret.boolValue = ret.numberValue == op.floatValue;
			else if (canEqual)
				ret.boolValue = ret.numberValue >= op.floatValue;
			else
				ret.boolValue = ret.numberValue > op.floatValue;
			break;
		case CTYPE_DOUBLE:
			if (justCompareEquale)
				ret.boolValue = ret.numberValue == op.doubleValue;
			else if (canEqual)
				ret.boolValue = ret.numberValue >= op.doubleValue;
			else
				ret.boolValue = ret.numberValue > op.doubleValue;
			break;
		case CTYPE_OBJECT_BYTE:
		case CTYPE_OBJECT_SHORT:
		case CTYPE_OBJECT_INTEGER:
		case CTYPE_OBJECT_LONG:
			op.getObjectValue();
			if (op.hasError)
			{
				ret.setErrorMsg(op.getErrorMsg());
				return ret;
			}
			if (justCompareEquale)
				ret.boolValue = ret.numberValue == op.numberValue;
			else if (canEqual)
				ret.boolValue = ret.numberValue >= op.numberValue;
			else
				ret.boolValue = ret.numberValue > op.numberValue;
			break;
        case CTYPE_OBJECT_FLOAT:
			op.getObjectValue();
			if (op.hasError)
			{
				ret.setErrorMsg(op.getErrorMsg());
				return ret;
			}
			if (justCompareEquale)
				ret.boolValue = ret.numberValue == op.floatValue;
			else if (canEqual)
				ret.boolValue = ret.numberValue >= op.floatValue;
			else
				ret.boolValue = ret.numberValue > op.floatValue;
			break;
		case CTYPE_OBJECT_DOUBLE:
			op.getObjectValue();
			if (op.hasError)
			{
				ret.setErrorMsg(op.getErrorMsg());
				return ret;
			}
			if (justCompareEquale)
				ret.boolValue = ret.numberValue == op.doubleValue;
			else if (canEqual)
				ret.boolValue = ret.numberValue >= op.doubleValue;
			else
				ret.boolValue = ret.numberValue > op.doubleValue;
			break;
		default:
			isTypeMismatch = true;
			break;
		}
		break;
    case CTYPE_OBJECT_FLOAT:
		if (justCompareEquale && (op.type == CTYPE_OBJECT_NULL))
		{
			ret.boolValue = false;
			break;
		}
        ret.getObjectValue();
		if (ret.hasError)
		{
			return ret;
		}
	case CTYPE_FLOAT:
		switch (op.type)
		{
		case CTYPE_NUMBER:
			if (justCompareEquale)
				ret.boolValue = ret.floatValue == op.numberValue;
			else if (canEqual)
				ret.boolValue = ret.floatValue >= op.numberValue;
			else
				ret.boolValue = ret.floatValue > op.numberValue;
			break;
		case CTYPE_FLOAT:
			if (justCompareEquale)
				ret.boolValue = ret.floatValue == op.floatValue;
			else if (canEqual)
				ret.boolValue = ret.floatValue >= op.floatValue;
			else
				ret.boolValue = ret.floatValue > op.floatValue;
			break;
		case CTYPE_DOUBLE:
			if (justCompareEquale)
				ret.boolValue = ret.floatValue == op.doubleValue;
			else if (canEqual)
				ret.boolValue = ret.floatValue >= op.doubleValue;
			else
				ret.boolValue = ret.floatValue > op.doubleValue;
			break;
		case CTYPE_OBJECT_BYTE:
		case CTYPE_OBJECT_SHORT:
		case CTYPE_OBJECT_INTEGER:
		case CTYPE_OBJECT_LONG:
			op.getObjectValue();
			if (op.hasError)
			{
				ret.setErrorMsg(op.getErrorMsg());
				return ret;
			}
			if (justCompareEquale)
				ret.boolValue = ret.floatValue == op.numberValue;
			else if (canEqual)
				ret.boolValue = ret.floatValue >= op.numberValue;
			else
				ret.boolValue = ret.floatValue > op.numberValue;
			break;
        case CTYPE_OBJECT_FLOAT:
			op.getObjectValue();
			if (op.hasError)
			{
				ret.setErrorMsg(op.errorMsg);
				return ret;
			}
			if (justCompareEquale)
				ret.boolValue = ret.floatValue == op.floatValue;
			else if (canEqual)
				ret.boolValue = ret.floatValue >= op.floatValue;
			else
				ret.boolValue = ret.floatValue > op.floatValue;
			break;
		case CTYPE_OBJECT_DOUBLE:
			op.getObjectValue();
			if (op.hasError)
			{
				ret.setErrorMsg(op.errorMsg);
				return ret;
			}
			if (justCompareEquale)
				ret.boolValue = ret.floatValue == op.doubleValue;
			else if (canEqual)
				ret.boolValue = ret.floatValue >= op.doubleValue;
			else
				ret.boolValue = ret.floatValue > op.doubleValue;
			break;
		default:
			isTypeMismatch = true;
			break;
		}
		break;
	case CTYPE_OBJECT_DOUBLE:
		if (justCompareEquale && (op.type == CTYPE_OBJECT_NULL))
		{
			ret.boolValue = false;
			break;
		}
        ret.getObjectValue();
		if (ret.hasError)
		{
			return ret;
		}
	case CTYPE_DOUBLE:
		switch (op.type)
		{
		case CTYPE_NUMBER:
			if (justCompareEquale)
				ret.boolValue = ret.doubleValue == op.numberValue;
			else if (canEqual)
				ret.boolValue = ret.doubleValue >= op.numberValue;
			else
				ret.boolValue = ret.doubleValue > op.numberValue;
			break;
		case CTYPE_FLOAT:
			if (justCompareEquale)
				ret.boolValue = ret.doubleValue == op.floatValue;
			else if (canEqual)
				ret.boolValue = ret.doubleValue >= op.floatValue;
			else
				ret.boolValue = ret.doubleValue > op.floatValue;
			break;
		case CTYPE_DOUBLE:
			if (justCompareEquale)
				ret.boolValue = ret.doubleValue == op.doubleValue;
			else if (canEqual)
				ret.boolValue = ret.doubleValue >= op.doubleValue;
			else
				ret.boolValue = ret.doubleValue > op.doubleValue;
			break;
		case CTYPE_OBJECT_BYTE:
		case CTYPE_OBJECT_SHORT:
		case CTYPE_OBJECT_INTEGER:
		case CTYPE_OBJECT_LONG:
			op.getObjectValue();
			if (op.hasError)
			{
				ret.setErrorMsg(op.getErrorMsg());
				return ret;
			}
			if (justCompareEquale)
				ret.boolValue = ret.doubleValue == op.numberValue;
			else if (canEqual)
				ret.boolValue = ret.doubleValue >= op.numberValue;
			else
				ret.boolValue = ret.doubleValue > op.numberValue;
			break;
        case CTYPE_OBJECT_FLOAT:
			op.getObjectValue();
			if (op.hasError)
			{
				ret.setErrorMsg(op.errorMsg);
				return ret;
			}
			if (justCompareEquale)
				ret.boolValue = ret.doubleValue == op.floatValue;
			else if (canEqual)
				ret.boolValue = ret.doubleValue >= op.floatValue;
			else
				ret.boolValue = ret.doubleValue > op.floatValue;
			break;
		case CTYPE_OBJECT_DOUBLE:
			op.getObjectValue();
			if (op.hasError)
			{
				ret.setErrorMsg(op.errorMsg);
				return ret;
			}
			if (justCompareEquale)
				ret.boolValue = ret.doubleValue == op.doubleValue;
			else if (canEqual)
				ret.boolValue = ret.doubleValue >= op.doubleValue;
			else
				ret.boolValue = ret.doubleValue > op.doubleValue;
			break;
		default:
			isTypeMismatch = true;
			break;
		}
		break;
    case CTYPE_OBJECT_CHARACTER:
		if (justCompareEquale && (op.type == CTYPE_OBJECT_NULL))
		{
			ret.boolValue = false;
			break;
		}
        ret.getObjectValue();
		if (ret.hasError)
		{
			return ret;
		}
	case CTYPE_CHAR:
		if (op.type == CTYPE_OBJECT_CHARACTER)
		{
			op.getObjectValue();
			if (op.hasError)
			{
				ret.setErrorMsg(op.getErrorMsg());
				return ret;
			}
		}
		if (op.type == CTYPE_CHAR)
		{
			if (justCompareEquale)
				ret.boolValue = (strcmp(ret.utf8CharValue, op.utf8CharValue) == 0)? true : false;
			else if (canEqual)
				ret.boolValue = (strcmp(ret.utf8CharValue, op.utf8CharValue) >= 0)? true : false;
			else
				ret.boolValue = (strcmp(ret.utf8CharValue, op.utf8CharValue) > 0)? true : false;
		}
		else
		{
			isTypeMismatch = true;
		}
		break;
	case CTYPE_OBJECT_STRING:
		if (justCompareEquale && (op.type == CTYPE_OBJECT_NULL))
		{
			ret.boolValue = false;
			break;
		}
        ret.getObjectValue();
		if (ret.hasError)
		{
			return ret;
		}
    case CTYPE_STRING:
		if (op.type == CTYPE_OBJECT_STRING)
		{
			op.getObjectValue();
			if (op.hasError)
			{
				ret.setErrorMsg(op.errorMsg);
				return ret;
			}
		}
		if (op.type == CTYPE_STRING)
		{
			int nRetV = strcmp(ret.stringValue.c_str(), op.stringValue.c_str());
			//modified by Qiu Song on 20090916 for ðŒŽ®‚ÉžB–†Žw’è
			if (justCompareEquale)
			{
				ret.boolValue = HelperFunc::doesStringMatch(ret.stringValue.c_str(), op.stringValue.c_str());
			}
			//end of modified by Qiu Song on 20090916 for ðŒŽ®‚ÉžB–†Žw’è
			else if (canEqual)
				ret.boolValue = ( nRetV >= 0);
			else
				ret.boolValue = ( nRetV > 0);
		}
        else
		{
			isTypeMismatch = true;
		}
		break;
    case CTYPE_OBJECT_DATE:
		if (justCompareEquale && (op.type == CTYPE_OBJECT_NULL))
		{
			ret.boolValue = false;
			break;
		}
		if (op.type == CTYPE_OBJECT_DATE)
		{
			char szFormat[20] = {"yyyy/MM/dd HH:mm:ss"};
			ret.getDateObjectValue(szFormat);
			if (ret.hasError)
			{
				return ret;
			}
			op.getDateObjectValue(szFormat);
			if (op.hasError)
			{
				ret.setErrorMsg(op.getErrorMsg());
				return ret;
			}

			int nRetV = ret.compareDateString(ret.stringValue, op.stringValue, szFormat);
			if (justCompareEquale)
			{
				ret.boolValue = (nRetV == 0);
			}
			else if (canEqual)
			{
				ret.boolValue = (nRetV >= 0);
			}
			else
			{
				ret.boolValue = (nRetV > 0);
			}
		}
		else if (op.type == CTYPE_DATE_STRING)
		{
			char szFormat[20] = {0};
			op.getDateStringFormat(szFormat);
			ret.getDateObjectValue(szFormat);
			if (ret.hasError)
			{
				return ret;
			}

			int nRetV = ret.compareDateString(ret.stringValue, op.stringValue, szFormat);
			if (justCompareEquale)
			{
				ret.boolValue = (nRetV == 0);
			}
			else if (canEqual)
			{
				ret.boolValue = (nRetV >= 0);
			}
			else
			{
				ret.boolValue = (nRetV > 0);
			}
		}
		else
		{
			isTypeMismatch = true;
		}
		break;
    case CTYPE_DATE_STRING:
		if (op.type == CTYPE_DATE_STRING)
		{
			char szFormat1[20] = {0};
			char szFormat2[20] = {0};
			ret.getDateStringFormat(szFormat1);
			op.getDateStringFormat(szFormat2);
			if (strcmp(szFormat1, szFormat2) != 0)
			{
				isTypeMismatch = true;
			}
			int nRetV = ret.compareDateString(ret.stringValue, op.stringValue, szFormat1);
			if (justCompareEquale)
			{
				ret.boolValue = (nRetV == 0);
			}
			else if (canEqual)
			{
				ret.boolValue = (nRetV >= 0);
			}
			else
			{
				ret.boolValue = (nRetV > 0);
			}
		}
		else if (op.type == CTYPE_OBJECT_DATE)
		{
			char szFormat[20] = {0};
			ret.getDateStringFormat(szFormat);
			op.getDateObjectValue(szFormat);
			if (op.hasError)
			{
				ret.setErrorMsg(op.getErrorMsg());
				return ret;
			}
			int nRetV = ret.compareDateString(ret.stringValue, op.stringValue, szFormat);
			if (justCompareEquale)
			{
				ret.boolValue = (nRetV == 0);
			}
			else if (canEqual)
			{
				ret.boolValue = (nRetV >= 0);
			}
			else
			{
				ret.boolValue = (nRetV > 0);
			}
		}
        else
		{
			isTypeMismatch = true;
		}
		break;
	case CTYPE_OBJECT_BOOLEAN:
		if (justCompareEquale && (op.type == CTYPE_OBJECT_NULL))
		{
			ret.boolValue = false;
			break;
		}
        ret.getObjectValue();
		if (ret.hasError)
		{
			return ret;
		}
	case CTYPE_BOOL:
		if (op.type == CTYPE_OBJECT_BOOLEAN)
		{
			op.getObjectValue();
			if (op.hasError)
			{
				ret.setErrorMsg(op.getErrorMsg());
				return ret;
			}
		}
		if (op.type == CTYPE_BOOL)
		{
			if (justCompareEquale)
				ret.boolValue = (ret.boolValue == op.boolValue);
			else
				isTypeMismatch = true;
		}
		else
		{
			isTypeMismatch = true;
		}
		break;
	case CTYPE_OBJECT_NULL:
		if (justCompareEquale)
		{
			if (op.type == CTYPE_OBJECT_NULL)
			{
				ret.boolValue = true;
			}
			else if (op.type > CTYPE_OBJECT_NULL)
			{
				ret.boolValue = false;
			}
			else
			{
				isTypeMismatch = true;
			}
		}
		else
		{
			isTypeMismatch = true;
		}
		break;
    default:
		if (ret.type > CTYPE_OBJECT_NULL)
		{
			if (justCompareEquale)
			{
				if (op.type == CTYPE_OBJECT_NULL)
				{
					ret.boolValue = false;
				}
				else if (ret.type == op.type)
				{
					jboolean bRet = CcatParser::jvmEnv->jni->IsSameObject(ret.objectValue, 
																		  op.objectValue);
					ret.boolValue = (bRet == 0)?false:true;
				}
				else
				{
					isTypeMismatch = true;
				}
			}
			else
			{
				isTypeMismatch = true;
			}
		}
		else
		{
			isTypeMismatch = true;
		}
		break;
	}

	if (isTypeMismatch)
	{
		char szMsg[MAX_MSG_LEN + 1];
		sprintf(szMsg, "The operator [%s]'s operands are not match.", operatorStr);
		ret.setErrorMsg(szMsg);
	}
	else
	{
		if (ret.objectValue != NULL)
		{
			jobject objBak = ret.objectValue;
			AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);
			ret.objectValue = NULL;
		}
		ret.type = CTYPE_BOOL;
	}

	return ret;
}

void CcatTypeValue::checkSansuOperands(CcatTypeValue& op, char* operatorStr)
{
	if ((this->type == CTYPE_OBJECT_BYTE) || (this->type == CTYPE_OBJECT_SHORT)
		|| (this->type == CTYPE_OBJECT_INTEGER) || (this->type == CTYPE_OBJECT_LONG))
	{
		this->getObjectValue();
		if (this->hasError)
			return;
	}
	else if ((this->type == CTYPE_OBJECT_FLOAT) || (this->type == CTYPE_OBJECT_DOUBLE))
	{
		this->getObjectValue();
		if (this->hasError)
			return;
	}

	if ((op.type == CTYPE_OBJECT_BYTE) || (op.type == CTYPE_OBJECT_SHORT)
		|| (op.type == CTYPE_OBJECT_INTEGER) || (op.type == CTYPE_OBJECT_LONG))
	{
		op.getObjectValue();
		if (op.hasError)
		{
			this->setErrorMsg(op.getErrorMsg());
			return;
		}
	}
	else if ((op.type == CTYPE_OBJECT_FLOAT) || (op.type == CTYPE_OBJECT_DOUBLE))
	{
		op.getObjectValue();
		if (op.hasError)
		{
			this->setErrorMsg(op.getErrorMsg());
			return;
		}
	}

	if ((this->type != CTYPE_NUMBER) 
		&& (this->type != CTYPE_FLOAT) 
		&& (this->type != CTYPE_DOUBLE))
	{
		char szMsg[MAX_MSG_LEN + 1];
		sprintf(szMsg, "The operator [%s]'s operands are not match.", operatorStr);
		this->setErrorMsg(szMsg);
		return;
	}

	if ((op.type != CTYPE_NUMBER) 
		&& (op.type != CTYPE_FLOAT)
		&& (op.type != CTYPE_DOUBLE))
	{
		char szMsg[MAX_MSG_LEN + 1];
		sprintf(szMsg, "The operator [%s]'s operands are not match.", operatorStr);
		this->setErrorMsg(szMsg);
		return;
	}
}

CcatTypeValue CcatTypeValue::operator||(const CcatTypeValue& op) const
{
	CcatTypeValue ret1(*this);
	CcatTypeValue ret2(op);
	if (ret1.type == CTYPE_OBJECT_BOOLEAN)
	{
		ret1.getObjectValue();
		if (ret1.hasError)
			return ret1;
	}
	if (ret2.type == CTYPE_OBJECT_BOOLEAN)
	{
		ret2.getObjectValue();
		if (ret2.hasError)
		{
			ret1.setErrorMsg(ret2.getErrorMsg());
			return ret1;
		}
	}
	if ((ret1.type != CTYPE_BOOL) || (ret2.type != CTYPE_BOOL))
	{
		ret1.setErrorMsg("The operator [||]'s operands are not match.");
		return ret1;
	}
    ret1.boolValue = (ret1.boolValue || ret2.boolValue);
	return ret1;
}

CcatTypeValue CcatTypeValue::operator&&(const CcatTypeValue& op) const
{
	CcatTypeValue ret1(*this);
	CcatTypeValue ret2(op);
	if (ret1.type == CTYPE_OBJECT_BOOLEAN)
	{
		ret1.getObjectValue();
		if (ret1.hasError)
			return ret1;
	}
	if (ret2.type == CTYPE_OBJECT_BOOLEAN)
	{
		ret2.getObjectValue();
		if (ret2.hasError)
		{
			ret1.setErrorMsg(ret2.getErrorMsg());
			return ret1;
		}
	}
	if ((ret1.type != CTYPE_BOOL) || (ret2.type != CTYPE_BOOL))
	{
		ret1.setErrorMsg("The operator [&&]'s operands are not match.");
		return ret1;
	}
    ret1.boolValue = (ret1.boolValue && ret2.boolValue);
	return ret1;
}

CcatTypeValue CcatTypeValue::operator!() const
{
	CcatTypeValue ret(*this);
	if (ret.type == CTYPE_BOOL)
	{
		ret.boolValue = !(ret.boolValue);
	}
	else if (ret.type == CTYPE_OBJECT_BOOLEAN)
	{
		ret.getObjectValue();
		ret.boolValue = !(ret.boolValue);
	}
	else
	{
		ret.setErrorMsg("The operator [!]'s operands are not match.");
	}
	return ret;
}


CcatTypeValue CcatTypeValue::operator==(const CcatTypeValue& op) const
{
    CcatTypeValue tmp(op);
	if ((this->type == CTYPE_OBJECT_NULL)
		|| (op.type == CTYPE_OBJECT_NULL))
	{
		if (this->type == op.type)
		{
			tmp.boolValue = true;
			tmp.type = CTYPE_BOOL;
		}
		else
		{
			if ((this->type < CTYPE_OBJECT_NULL)
				|| (op.type < CTYPE_OBJECT_NULL))
			{
				tmp.setErrorMsg("The operator [==]'s operands are not match.");
			}
			else
			{
				tmp.boolValue = false;
				tmp.type = CTYPE_BOOL;
			}
		}
		return tmp;
	}
	else
	{
		return relationValue(tmp, true, false, "==");
	}
}

CcatTypeValue CcatTypeValue::operator!=(const CcatTypeValue& op) const
{
    CcatTypeValue tmp(op);
	if ((this->type == CTYPE_OBJECT_NULL)
		|| (op.type == CTYPE_OBJECT_NULL))
	{
		if (this->type == op.type)
		{
			tmp.boolValue = false;
			tmp.type = CTYPE_BOOL;
		}
		else
		{
			if ((this->type < CTYPE_OBJECT_NULL)
				|| (op.type < CTYPE_OBJECT_NULL))
			{
				tmp.setErrorMsg("The operator [!=]'s operands are not match.");
			}
			else
			{
				tmp.boolValue = true;
				tmp.type = CTYPE_BOOL;
			}
		}
		return tmp;
	}
	else
	{
		CcatTypeValue ret = relationValue(tmp, true, false, "!=");
		if (ret.hasError)
			return ret;
		else 
			return  !ret;
	}
}

CcatTypeValue CcatTypeValue::operator>(const CcatTypeValue& op) const
{
    CcatTypeValue tmp(op);
    return relationValue(tmp, false, false, ">");
}

CcatTypeValue CcatTypeValue::operator>=(const CcatTypeValue& op) const
{
    CcatTypeValue tmp(op);
    return relationValue(tmp, false, true, ">=");
}

CcatTypeValue CcatTypeValue::operator<(const CcatTypeValue& op) const
{
    CcatTypeValue tmp1(op);
    CcatTypeValue tmp2(*this);
    CcatTypeValue ret = tmp1.relationValue(tmp2, false, false, "<");
	return ret;
}

CcatTypeValue CcatTypeValue::operator<=(const CcatTypeValue& op) const
{
    CcatTypeValue tmp1(op);
    CcatTypeValue tmp2(*this);
    CcatTypeValue ret = tmp1.relationValue(tmp2, false, true, "<=");
	return ret;
}

CcatTypeValue CcatTypeValue::operator+() const
{
	CcatTypeValue ret(*this);
    if ((ret.type >= CTYPE_OBJECT_BYTE)
		&& (ret.type <= CTYPE_OBJECT_DOUBLE))
	{
		ret.getObjectValue();
		if (ret.hasError)
			return ret;
	}
	if ((ret.type != CTYPE_NUMBER) 
		&& (ret.type != CTYPE_FLOAT)
		&& (ret.type != CTYPE_DOUBLE))
	{
		ret.setErrorMsg("The operator [+]'s operand is not match.");
		return ret;
	}
	else
	{
		return ret;
	}
}

CcatTypeValue CcatTypeValue::operator-() const
{
    CcatTypeValue ret(*this);
    if ((ret.type >= CTYPE_OBJECT_BYTE)
		&& (ret.type <= CTYPE_OBJECT_DOUBLE))
	{
		ret.getObjectValue();
		if (ret.hasError)
			return ret;
	}
	if (ret.type == CTYPE_NUMBER)
	{
        ret.numberValue = -this->numberValue;
	}
	else if (ret.type == CTYPE_FLOAT)
	{
        ret.floatValue = -this->floatValue;
	}
	else if (ret.type == CTYPE_DOUBLE)
	{
        ret.doubleValue = -this->doubleValue;
	}
	else
	{
		ret.setErrorMsg("The operator [-]'s operand is not match.");
	}
	return ret;
}


CcatTypeValue CcatTypeValue::operator+(const CcatTypeValue& op) const
{ 
    CcatTypeValue op1(*this);
    CcatTypeValue op2(op);
    op1.checkSansuOperands(op2, "+");

	if (op1.hasError)
		return op1;

    if (op1.type == CTYPE_NUMBER)
	{
		if (op2.type == CTYPE_NUMBER)
		{
			op1.numberValue += op2.numberValue;
		}
		else if (op2.type == CTYPE_FLOAT)
		{
			op1.floatValue = op1.numberValue + op2.floatValue;
			op1.type = CTYPE_FLOAT;
		}
		else if (op2.type == CTYPE_DOUBLE)
		{
			op1.doubleValue = op1.numberValue + op2.doubleValue;
			op1.type = CTYPE_DOUBLE;
		}
		else
		{
		    op1.setErrorMsg("The operator [+]'s operands are not match.");
		}
	}
	else if (op1.type == CTYPE_FLOAT)
	{
		if (op2.type == CTYPE_NUMBER)
		{
			op1.floatValue += op2.numberValue;
		}
		else if (op2.type == CTYPE_FLOAT)
		{
			op1.floatValue += op2.floatValue;
		}
		else if (op2.type == CTYPE_DOUBLE)
		{
			op1.doubleValue = op1.floatValue + op2.doubleValue;
			op1.type = CTYPE_DOUBLE;
		}
		else
		{
			op1.setErrorMsg("The operator [+]'s operands are not match.");
		}
	}
	else if (op1.type == CTYPE_DOUBLE)
	{
		if (op2.type == CTYPE_NUMBER)
		{
			op1.doubleValue += op2.numberValue;
		}
		else if (op2.type == CTYPE_FLOAT)
		{
			op1.doubleValue += op2.floatValue;
		}
		else if (op2.type == CTYPE_DOUBLE)
		{
			op1.doubleValue += op2.doubleValue;
		}
		else
		{
		    op1.setErrorMsg("The operator [+]'s operands are not match.");
		}
	}
    else
	{
		op1.setErrorMsg("The operator [+]'s operands are not match.");
	}
	return op1;
}


CcatTypeValue CcatTypeValue::operator-(const CcatTypeValue& op) const
{
    CcatTypeValue op1(*this);
    CcatTypeValue op2(op);
    op1.checkSansuOperands(op2, "-");

	if (op1.hasError)
		return op1;

    if (op1.type == CTYPE_NUMBER)
	{
		if (op2.type == CTYPE_NUMBER)
		{
			op1.numberValue -= op2.numberValue;
		}
		else if (op2.type == CTYPE_FLOAT)
		{
			op1.floatValue = op1.numberValue - op2.floatValue;
			op1.type = CTYPE_FLOAT;
		}
		else if (op2.type == CTYPE_DOUBLE)
		{
			op1.doubleValue = op1.numberValue - op2.doubleValue;
			op1.type = CTYPE_DOUBLE;
		}
		else
		{
		    op1.setErrorMsg("The operator [-]'s operands are not match.");
		}
	}
	else if (op1.type == CTYPE_FLOAT)
	{
		if (op2.type == CTYPE_NUMBER)
		{
			op1.floatValue -= op2.numberValue;
		}
		else if (op2.type == CTYPE_FLOAT)
		{
			op1.floatValue -= op2.floatValue;
		}
		else if (op2.type == CTYPE_DOUBLE)
		{
			op1.doubleValue = op1.floatValue - op2.doubleValue;
			op1.type = CTYPE_DOUBLE;
		}
		else
		{
			op1.setErrorMsg("The operator [-]'s operands are not match.");
		}
	}
	else if (op1.type == CTYPE_DOUBLE)
	{
		if (op2.type == CTYPE_NUMBER)
		{
			op1.doubleValue -= op2.numberValue;
		}
		else if (op2.type == CTYPE_FLOAT)
		{
			op1.doubleValue -= op2.floatValue;
		}
		else if (op2.type == CTYPE_DOUBLE)
		{
			op1.doubleValue -= op2.doubleValue;
		}
		else
		{
		    op1.setErrorMsg("The operator [-]'s operands are not match.");
		}
	}
    else
	{
		op1.setErrorMsg("The operator [-]'s operands are not match.");
	}
	return op1;
}

CcatTypeValue CcatTypeValue::operator*(const CcatTypeValue& op) const
{
    CcatTypeValue op1(*this);
    CcatTypeValue op2(op);
    op1.checkSansuOperands(op2, "*");

	if (op1.hasError)
		return op1;

    if (op1.type == CTYPE_NUMBER)
	{
		if (op2.type == CTYPE_NUMBER)
		{
			op1.numberValue *= op2.numberValue;
		}
		else if (op2.type == CTYPE_FLOAT)
		{
			op1.floatValue = op1.numberValue * op2.floatValue;
			op1.type = CTYPE_FLOAT;
		}
		else if (op2.type == CTYPE_DOUBLE)
		{
			op1.doubleValue = op1.numberValue * op2.doubleValue;
			op1.type = CTYPE_DOUBLE;
		}
		else
		{
		    op1.setErrorMsg("The operator [*]'s operands are not match.");
		}
	}
	else if (op1.type == CTYPE_FLOAT)
	{
		if (op2.type == CTYPE_NUMBER)
		{
			op1.floatValue *= op2.numberValue;
		}
		else if (op2.type == CTYPE_FLOAT)
		{
			op1.floatValue *= op2.floatValue;
		}
		else if (op2.type == CTYPE_DOUBLE)
		{
			op1.doubleValue = op1.floatValue * op2.doubleValue;
			op1.type = CTYPE_DOUBLE;
		}
		else
		{
			op1.setErrorMsg("The operator [*]'s operands are not match.");
		}
	}
	else if (op1.type == CTYPE_DOUBLE)
	{
		if (op2.type == CTYPE_NUMBER)
		{
			op1.doubleValue *= op2.numberValue;
		}
		else if (op2.type == CTYPE_FLOAT)
		{
			op1.doubleValue *= op2.floatValue;
		}
		else if (op2.type == CTYPE_DOUBLE)
		{
			op1.doubleValue *= op2.doubleValue;
		}
		else
		{
		    op1.setErrorMsg("The operator [*]'s operands are not match.");
		}
	}
    else
	{
		op1.setErrorMsg("The operator [*]'s operands are not match.");
	}
	return op1;
}


CcatTypeValue CcatTypeValue::operator/(const CcatTypeValue& op) const
{
    CcatTypeValue op1(*this);
    CcatTypeValue op2(op);
	op1.checkSansuOperands(op2, "/");

	if (op1.hasError)
		return op1;

    if (op1.type == CTYPE_NUMBER)
	{
		if (op2.type == CTYPE_NUMBER)
		{
			if (op2.numberValue != 0)
				op1.numberValue /= op2.numberValue;
			else
			{
				op1.setErrorMsg("The operator [/]'s second operand is 0.");
			}
		}
		else if (op2.type == CTYPE_FLOAT)
		{
			if (op2.floatValue != 0)
				op1.floatValue = op1.numberValue / op2.floatValue;
			else
			{
				op1.setErrorMsg("The operator [/]'s second operand is 0.");
			}
			op1.type = CTYPE_FLOAT;
		}
		else if (op2.type == CTYPE_DOUBLE)
		{
			if (op2.doubleValue != 0)
				op1.doubleValue = op1.numberValue / op2.doubleValue;
			else
			{
				op1.setErrorMsg("The operator [/]'s second operand is 0.");
			}
			op1.type = CTYPE_DOUBLE;
		}
		else
		{
		    op1.setErrorMsg("The operator [/]'s operands are not match.");
		}
	}
	else if (op1.type == CTYPE_FLOAT)
	{
		if (op2.type == CTYPE_NUMBER)
		{
			if (op2.numberValue != 0)
				op1.floatValue /= op2.numberValue;
			else
			{
				op1.setErrorMsg("The operator [/]'s second operand is 0.");
			}
		}
		else if (op2.type == CTYPE_FLOAT)
		{
			if (op2.floatValue != 0)
				op1.floatValue = op1.floatValue / op2.floatValue;
			else
			{
				op1.setErrorMsg("The operator [/]'s second operand is 0.");
			}
		}
		else if (op2.type == CTYPE_DOUBLE)
		{
			if (op2.doubleValue != 0)
			{
				op1.doubleValue = op1.floatValue / op2.doubleValue;
				op1.type = CTYPE_DOUBLE;
			}	
			else
			{
				op1.setErrorMsg("The operator [/]'s second operand is 0.");
			}
		}
		else
		{
			op1.setErrorMsg("The operator [/]'s operands are not match.");
		}
	}
	else if (op1.type == CTYPE_DOUBLE)
	{
		if (op2.type == CTYPE_NUMBER)
		{
			if (op2.numberValue != 0)
				op1.doubleValue /= op2.numberValue;
			else
			{
				op1.setErrorMsg("The operator [/]'s second operand is 0.");
			}
		}
		else if (op2.type == CTYPE_FLOAT)
		{
			if (op2.floatValue != 0)
				op1.doubleValue = op1.doubleValue / op2.floatValue;
			else
			{
				op1.setErrorMsg("The operator [/]'s second operand is 0.");
			}
		}
		else if (op2.type == CTYPE_DOUBLE)
		{
			if (op2.doubleValue != 0)
				op1.doubleValue = op1.doubleValue / op2.doubleValue;
			else
			{
				op1.setErrorMsg("The operator [/]'s second operand is 0.");
			}
		}
		else
		{
		    op1.setErrorMsg("The operator [/]'s operands are not match.");
		}
	}
    else
	{
		op1.setErrorMsg("The operator [/]'s operands are not match.");
	}

	return op1;
}


CcatTypeValue CcatTypeValue::operator%(const CcatTypeValue& op) const
{
    CcatTypeValue op1(*this);
    CcatTypeValue op2(op);
	op1.checkSansuOperands(op2, "%");

	if (op1.hasError)
		return op1;

    if (op1.type == CTYPE_NUMBER)
	{
		if (op2.type == CTYPE_NUMBER)
		{
			if (op2.numberValue != 0)
				op1.numberValue %= op2.numberValue;
			else
			{
				op1.setErrorMsg("The operator [%]'s second operand is 0.");
			}
		}
		else if (op2.type == CTYPE_FLOAT)
		{
			if (op2.floatValue != 0)
			{
				float dTmp =  op1.numberValue / op2.floatValue;
				op1.floatValue = op1.numberValue - dTmp * op2.floatValue;
				op1.type = CTYPE_FLOAT;
			}
			else
			{
				op1.setErrorMsg("The operator [%]'s second operand is 0.");
			}
		}
		else if (op2.type == CTYPE_DOUBLE)
		{
			if (op2.doubleValue != 0)
			{
				double dTmp =  op1.numberValue / op2.doubleValue;
				op1.doubleValue = op1.numberValue - dTmp * op2.doubleValue;
				op1.type = CTYPE_DOUBLE;
			}
			else
			{
				op1.setErrorMsg("The operator [%]'s second operand is 0.");
			}
		}
		else
		{
		    op1.setErrorMsg("The operator [%]'s operands are not match.");
		}
	}
	else if (op1.type == CTYPE_FLOAT)
	{
		if (op2.type == CTYPE_NUMBER)
		{
			if (op2.numberValue != 0)
			{
				float dTmp =  op1.floatValue / op2.numberValue;
				op1.floatValue -= dTmp * op2.numberValue;
			}
			else
			{
				op1.setErrorMsg("The operator [%]'s second operand is 0.");
			}
		}
		else if (op2.type == CTYPE_FLOAT)
		{
			if (op2.floatValue != 0)
			{
				float dTmp =  op1.floatValue / op2.floatValue;
				op1.floatValue -= dTmp * op2.floatValue;
			}
			else
			{
				op1.setErrorMsg("The operator [%]'s second operand is 0.");
			}
		}
		else if (op2.type == CTYPE_DOUBLE)
		{
			if (op2.doubleValue != 0)
			{
				double dTmp =  op1.floatValue / op2.doubleValue;
				op1.doubleValue = op1.floatValue - dTmp * op2.doubleValue;
				op1.type = CTYPE_DOUBLE;
			}
			else
			{
				op1.setErrorMsg("The operator [%]'s second operand is 0.");
			}
		}
		else
		{
			op1.setErrorMsg("The operator [%]'s operands are not match.");
		}
	}
	else if (op1.type == CTYPE_DOUBLE)
	{
		if (op2.type == CTYPE_NUMBER)
		{
			if (op2.numberValue != 0)
			{
				double dTmp =  op1.doubleValue / op2.numberValue;
				op1.doubleValue -= dTmp * op2.numberValue;
			}
			else
			{
				op1.setErrorMsg("The operator [%]'s second operand is 0.");
			}
		}
		else if (op2.type == CTYPE_FLOAT)
		{
			if (op2.floatValue != 0)
			{
				double dTmp =  op1.doubleValue / op2.floatValue;
				op1.doubleValue -= dTmp * op2.floatValue;
			}
			else
			{
				op1.setErrorMsg("The operator [%]'s second operand is 0.");
			}
		}
		else if (op2.type == CTYPE_DOUBLE)
		{
			if (op2.doubleValue != 0)
			{
				double dTmp =  op1.doubleValue / op2.doubleValue;
				op1.doubleValue -= dTmp * op2.doubleValue;
			}
			else
			{
				op1.setErrorMsg("The operator [%]'s second operand is 0.");
			}
		}
		else
		{
		    op1.setErrorMsg("The operator[%]'s operands are not match.");
		}
	}
    else
	{
		op1.setErrorMsg("The operator [%]'s operands are not match.");
	}
	return op1;
}


void CcatTypeValue::getDateStringFormat(char* dateFormat)
{
	strcpy(dateFormat, "");
    const char* szTmp = this->stringValue.c_str();
	if (strchr(szTmp, '/') != NULL)
	{
        strcat(dateFormat, "yyyy/MM/dd");
		if (strchr(szTmp, ':') != NULL)
		{
			strcat(dateFormat, " ");
		}
	}
	const char* p = strchr(szTmp, ':');
	if (p != NULL)
	{
		if (strlen(p) == 6)
		{
			strcat(dateFormat, "HH:mm:ss");
		}
		else
		{
			strcat(dateFormat, "HH:mm");
		}
	}
	return;
}

int  CcatTypeValue::compareDateString(string date1, string date2, char* dateFormat)
{
    int nRet = 0;

	const char* p1 = date1.c_str();
	const char* p2 = date2.c_str();
	if (strncmp(dateFormat, "yyyy", 4) == 0)  //compare "yyyy/mm/dd"
	{
		nRet = strncmp(p1, p2, 10);
		if (nRet != 0)
			return nRet;
	}
    
	char* p = strstr(dateFormat, "HH:mm");
	if (p == NULL)
		return nRet;

	int nTmp = p - dateFormat;    //compare hh:mm:ss
	nRet = strcmp(p1 + nTmp, p2 + nTmp);
	return nRet;
}

void CcatTypeValue::getObjectValue()
{
	switch (this->type)
	{
	case CTYPE_OBJECT_BYTE:
		getByteObjectValue();
		break;
	case CTYPE_OBJECT_SHORT:
		getShortObjectValue();
		break;
	case CTYPE_OBJECT_INTEGER:
		getIntObjectValue();
		break;
	case CTYPE_OBJECT_LONG:
		getLongObjectValue();
		break;
	case CTYPE_OBJECT_FLOAT:
		getFloatObjectValue();
		break;
	case CTYPE_OBJECT_DOUBLE:
		getDoubleObjectValue();
		break;
	case CTYPE_OBJECT_BOOLEAN:
		getBooleanObjectValue();
		break;
	case CTYPE_OBJECT_CHARACTER:
		getCharObjectValue();
		break;
	case CTYPE_OBJECT_STRING:
		getStringObjectValue();
		break;
	default:
		break;
	}
}


void CcatTypeValue::getByteObjectValue()
{
	jclass clazz = CcatParser::jvmEnv->jni->FindClass("java/lang/Byte");
	if (clazz == NULL)
	{
		Global::logger->logError("Cann't find class java.lang.Byte in CcatTypeValue::getByteObjectValue.");
		this->setErrorMsg("Cann't find class java.lang.Byte.");
		return;
	}
	jfieldID fieldId = CcatParser::jvmEnv->jni->GetFieldID(clazz, "value", "B");
	if (fieldId == NULL)
	{
		Global::logger->logError("Cann't get fieldId of java.lang.Byte.value in CcatTypeValue::getByteObjectValue.");
		this->setErrorMsg("Cann't get fieldId of java.lang.Byte.value.");
		return;
	}

	this->numberValue = CcatParser::jvmEnv->jni->GetByteField(this->objectValue, fieldId);
	this->type = CTYPE_NUMBER;
	jobject objBak = this->objectValue;
	AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);	
	this->objectValue = NULL;
}


void CcatTypeValue::getShortObjectValue()
{
	jclass clazz = CcatParser::jvmEnv->jni->FindClass("java/lang/Short");
	if (clazz == NULL)
	{
		Global::logger->logError("Cann't find class java.lang.Short in CcatTypeValue::getShortObjectValue.");
		this->setErrorMsg("Cann't find class java.lang.Short.");
		return;
	}
	jfieldID fieldId = CcatParser::jvmEnv->jni->GetFieldID(clazz, "value", "S");
	if (fieldId == NULL)
	{
		Global::logger->logError("Cann't get fieldId of java.lang.Short.value in CcatTypeValue::getShortObjectValue.");
		this->setErrorMsg("Cann't get fieldId of java.lang.Short.value.");
		return;
	}

	this->numberValue = CcatParser::jvmEnv->jni->GetShortField(this->objectValue, fieldId);
	this->type = CTYPE_NUMBER;
	jobject objBak = this->objectValue;
	AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);	
	this->objectValue = NULL;
}


void CcatTypeValue::getIntObjectValue()
{
	jclass clazz = CcatParser::jvmEnv->jni->FindClass("java/lang/Integer");
	if (clazz == NULL)
	{
		Global::logger->logError("Cann't find class java.lang.Integer in CcatTypeValue::getIntObjectValue.");
		this->setErrorMsg("Cann't find class java.lang.Integer.");
		return;
	}
	jfieldID fieldId = CcatParser::jvmEnv->jni->GetFieldID(clazz, "value", "I");
	if (fieldId == NULL)
	{
		Global::logger->logError("Cann't get fieldId of java.lang.Integer.value in CcatTypeValue::getIntObjectValue.");
		this->setErrorMsg("Cann't get fieldId of java.lang.Integer.value.");
		return;
	}

	this->numberValue = CcatParser::jvmEnv->jni->GetIntField(this->objectValue, fieldId);
	this->type = CTYPE_NUMBER;
	jobject objBak = this->objectValue;
	AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);	
	this->objectValue = NULL;
}


void CcatTypeValue::getLongObjectValue()
{
	jclass clazz = CcatParser::jvmEnv->jni->FindClass("java/lang/Long");
	if (clazz == NULL)
	{
		Global::logger->logError("Cann't find class java.lang.Long in CcatTypeValue::getLongObjectValue.");
		this->setErrorMsg("Cann't find class java.lang.Long.");
		return;
	}
	jfieldID fieldId = CcatParser::jvmEnv->jni->GetFieldID(clazz, "value", "J");
	if (fieldId == NULL)
	{
		Global::logger->logError("Cann't get fieldId of java.lang.Long.value in CcatTypeValue::getLongObjectValue.");
		this->setErrorMsg("Cann't get fieldId of java.lang.Long.value.");
		return;
	}

	this->numberValue = CcatParser::jvmEnv->jni->GetIntField(this->objectValue, fieldId);
	this->type = CTYPE_NUMBER;
	jobject objBak = this->objectValue;
	AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);	
	this->objectValue = NULL;
}


void CcatTypeValue::getFloatObjectValue()
{
	jclass clazz = CcatParser::jvmEnv->jni->FindClass("java/lang/Float");
	if (clazz == NULL)
	{
		Global::logger->logError("Cann't find class java.lang.Float in CcatTypeValue::getFloatObjectValue.");
		this->setErrorMsg("Cann't find class java.lang.Float.");
		return;
	}
	jfieldID fieldId = CcatParser::jvmEnv->jni->GetFieldID(clazz, "value", "F");
	if (fieldId == NULL)
	{
		Global::logger->logError("Cann't get fieldId of java.lang.Float.value in CcatTypeValue::getFloatObjectValue.");
		this->setErrorMsg("Cann't get fieldId of java.lang.Float.value.");
		return;
	}

	this->floatValue = CcatParser::jvmEnv->jni->GetFloatField(this->objectValue, fieldId);
	this->type = CTYPE_FLOAT;
	jobject objBak = this->objectValue;
	AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);	
	this->objectValue = NULL;
}


void CcatTypeValue::getDoubleObjectValue()
{
	jclass clazz = CcatParser::jvmEnv->jni->FindClass("java/lang/Double");
	if (clazz == NULL)
	{
		Global::logger->logError("Cann't find class java.lang.Double in CcatTypeValue::getDoubleObjectValue.");
		this->setErrorMsg("Cann't find class java.lang.Double.");
		return;
	}
	jfieldID fieldId = CcatParser::jvmEnv->jni->GetFieldID(clazz, "value", "D");
	if (fieldId == NULL)
	{
		Global::logger->logError("Cann't get fieldId of java.lang.Double.value in CcatTypeValue::getDoubleObjectValue.");
		this->setErrorMsg("Cann't get fieldId of java.lang.Double.value.");
		return;
	}

	this->doubleValue = CcatParser::jvmEnv->jni->GetDoubleField(this->objectValue, fieldId);
	this->type = CTYPE_DOUBLE;
	jobject objBak = this->objectValue;
	AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);	
	this->objectValue = NULL;
}


void CcatTypeValue::getBooleanObjectValue()
{
	jclass clazz = CcatParser::jvmEnv->jni->FindClass("java/lang/Boolean");
	if (clazz == NULL)
	{
		Global::logger->logError("Cann't find class java.lang.Boolean in CcatTypeValue::getBooleanObjectValue.");
		this->setErrorMsg("Cann't find class java.lang.Boolean.");
		return;
	}
	jfieldID fieldId = CcatParser::jvmEnv->jni->GetFieldID(clazz, "value", "Z");
	if (fieldId == NULL)
	{
		Global::logger->logError("Cann't get fieldId of java.lang.Boolean.value in CcatTypeValue::getBooleanObjectValue.");
		this->setErrorMsg("Cann't get fieldId of java.lang.Boolean.value.");
		return;
	}

	jboolean value = CcatParser::jvmEnv->jni->GetBooleanField(this->objectValue, fieldId);
	this->boolValue = (value == 0) ? false : true;
	this->type = CTYPE_BOOL;
	jobject objBak = this->objectValue;
	AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);	
	this->objectValue = NULL;
}


void CcatTypeValue::getCharObjectValue()
{
	jclass clazz = CcatParser::jvmEnv->jni->FindClass("java/lang/Character");
	if (clazz == NULL)
	{
		Global::logger->logError("Cann't find class java.lang.Character in CcatTypeValue::getCharObjectValue.");
		this->setErrorMsg("Cann't find class java.lang.Character.");
		return;
	}
	jfieldID fieldId = CcatParser::jvmEnv->jni->GetFieldID(clazz, "value", "C");
	if (fieldId == NULL)
	{
		Global::logger->logError("Cann't get fieldId of java.lang.Character.value in CcatTypeValue::getCharObjectValue.");
		this->setErrorMsg("Cann't get fieldId of java.lang.Character.value.");
		return;
	}

	jchar value = CcatParser::jvmEnv->jni->GetCharField(this->objectValue, fieldId);
	if (value > 127)
	{
		int charSize = 0;
		this->utf8CharValue = HelperFunc::utf16be_to_utf8(value,&charSize);
		if (this->utf8CharValue == NULL)
		{
			this->setErrorMsg("Cann't convert char value from unicode to utf8.");
			return;
		}
	}
	else
	{
		this->utf8CharValue = new char[2];
		this->utf8CharValue[0] = value;
		this->utf8CharValue[1] = 0;
	}
	this->type = CTYPE_CHAR;
	jobject objBak = this->objectValue;
	AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);	
	this->objectValue = NULL;
}

void CcatTypeValue::getStringObjectValue()
{
	Logger::getLogger()->logDebug("get String object value.");
    const char* value = CcatParser::jvmEnv->jni->GetStringUTFChars((jstring)(this->objectValue), NULL);
	this->stringValue = value;
    CcatParser::jvmEnv->jni->ReleaseStringUTFChars((jstring)(this->objectValue),value);
	this->type = CTYPE_STRING;
	jobject objBak = this->objectValue;
	AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);	
	this->objectValue = NULL;
	Logger::getLogger()->logDebug("value of String object is \"%s\"", this->stringValue.c_str());
}

void CcatTypeValue::getDateObjectValue(char* dateFormat)
{
	Logger::getLogger()->logDebug("get Date object value.");

	jmethodID callbackMethod = CcatParser::jvmEnv->jni->GetStaticMethodID(
		AgentCallbackHandler::callbacksclass,"formatDateObject",
		"(Ljava/util/Date;Ljava/lang/String;)Ljava/lang/String;");
	if(callbackMethod == NULL)
	{
		LOG4CXX_FATAL(Global::logger, "Cann't find method Callbacks::formatDateObject");
		this->setErrorMsg("Failed to get methodid of Callbacks::formatDateObject.");
		return;
	}

	jobject value = CcatParser::jvmEnv->jni->CallStaticObjectMethod(AgentCallbackHandler::callbacksclass,
		callbackMethod,
		this->objectValue,CcatParser::jvmEnv->jni->NewStringUTF(dateFormat));
    if (value == NULL)
	{
        LOG4CXX_FATAL(Global::logger, "Failed to format date object.");
		this->setErrorMsg("Failed to format date object.");
		return;
	}

	char dateString[20];
	memset(dateString,0,20);
	HelperFunc::getJStringValue(CcatParser::jvmEnv->jni, (jstring)value, dateString, 20);
	this->stringValue = dateString;
	this->type = CTYPE_DATE_STRING;
	jobject objBak = this->objectValue;
	AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);	
	this->objectValue = NULL;
	Logger::getLogger()->logDebug("value of Date object is %s", this->stringValue.c_str());
	return;
}


void CcatTypeValue::getArrayElement(CcatTypeValue& index)
{
    if (this->type < CTYPE_ARRAY_CHARACTER)
	{
		this->setErrorMsg("Cann't access non array object by index.");
		return;
	}

	if (index.type != CcatTypeValue::CTYPE_NUMBER)
	{
		this->setErrorMsg("Array Index should be number type.");
		return;
	}

	if (Logger::getLogger()->isDebugEnabled())
	{
		string value = "";
		printValue(value);
	    Logger::getLogger()->logDebug("get element value of array: %s by index:%ld", value.c_str(), index.numberValue);
	}

    jsize length = CcatParser::jvmEnv->jni->GetArrayLength((jarray)(this->objectValue));
	if (index.numberValue+1 > length)
	{
		this->setErrorMsg("array index is outof range.");
		return;
	}

	jobject objBak = this->objectValue;
	switch(this->type)
	{
	case CTYPE_ARRAY_CHARACTER:
		{
		jchar* elems = CcatParser::jvmEnv->jni->GetCharArrayElements((jcharArray)this->objectValue, 0);
		if(elems == NULL)
		{
			Global::logger->logError("failed to call GetCharArrayElements in CcatTypeValue::getArrayElement.");
			this->setErrorMsg("Failed to get char array elements.");
			return;
		}
		jchar elem = elems[index.numberValue];
		if (elem > 127)
		{
			int charSize = 0;
			this->utf8CharValue = HelperFunc::utf16be_to_utf8(elem,&charSize);
			CcatParser::jvmEnv->jni->ReleaseCharArrayElements((jcharArray)(this->objectValue), elems, 0);
			if (this->utf8CharValue == NULL)
			{
				this->setErrorMsg("Cann't convert char value from unicode to utf8.");
				return;
			}
		}
		else
		{
			this->utf8CharValue = new char[2];
			this->utf8CharValue[0] = elem;
			this->utf8CharValue[1] = 0;
			CcatParser::jvmEnv->jni->ReleaseCharArrayElements((jcharArray)(this->objectValue), elems, 0);
		}
		this->type = CTYPE_CHAR;
		AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);	
		this->objectValue = NULL;
		}
		break;
	case CTYPE_ARRAY_BYTE:
		{
		jbyte* elems = CcatParser::jvmEnv->jni->GetByteArrayElements((jbyteArray)this->objectValue, 0);
		if(elems == NULL)
		{
			Global::logger->logError("failed to call GetByteArrayElements in CcatTypeValue::getArrayElement.");
			this->setErrorMsg("Failed to get byte array elements.");
			return;
		}
		this->numberValue = elems[index.numberValue];
		this->type = CTYPE_NUMBER;
		CcatParser::jvmEnv->jni->ReleaseByteArrayElements((jbyteArray)(this->objectValue), elems, 0);
		AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);	
		this->objectValue = NULL;
		}
		break;
	case CTYPE_ARRAY_SHORT:
		{
		jshort* elems = CcatParser::jvmEnv->jni->GetShortArrayElements((jshortArray)this->objectValue, 0);
		if(elems == NULL)
		{
			Global::logger->logError("failed to call GetShortArrayElements in CcatTypeValue::getArrayElement.");
			this->setErrorMsg("Failed to get short array elements.");
			return;
		}
		this->numberValue = elems[index.numberValue];
		this->type = CTYPE_NUMBER;
		CcatParser::jvmEnv->jni->ReleaseShortArrayElements((jshortArray)(this->objectValue), elems, 0);
		AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);	
		this->objectValue = NULL;
		}
		break;
	case CTYPE_ARRAY_INTEGER:
		{
		jint* elems = CcatParser::jvmEnv->jni->GetIntArrayElements((jintArray)this->objectValue, 0);
		if(elems == NULL)
		{
			Global::logger->logError("failed to call GetIntArrayElements in CcatTypeValue::getArrayElement.");
			this->setErrorMsg("Failed to get int array elements.");
			return;
		}
		this->numberValue = elems[index.numberValue];
		this->type = CTYPE_NUMBER;
		CcatParser::jvmEnv->jni->ReleaseIntArrayElements((jintArray)(this->objectValue), elems, 0);
		AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);	
		this->objectValue = NULL;
		}
		break;
	case CTYPE_ARRAY_LONG:
		{
		jlong* elems = CcatParser::jvmEnv->jni->GetLongArrayElements((jlongArray)this->objectValue, 0);
		if(elems == NULL)
		{
			Global::logger->logError("failed to call GetLongArrayElements in CcatTypeValue::getArrayElement.");
			this->setErrorMsg("Failed to get long array elements.");
			return;
		}
		this->numberValue = elems[index.numberValue];
		this->type = CTYPE_NUMBER;
		CcatParser::jvmEnv->jni->ReleaseLongArrayElements((jlongArray)(this->objectValue), elems, 0);
		AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);	
		this->objectValue = NULL;
		}
		break;
	case CTYPE_ARRAY_FLOAT:
		{
		jfloat* elems = CcatParser::jvmEnv->jni->GetFloatArrayElements((jfloatArray)this->objectValue, 0);
		if(elems == NULL)
		{
			Global::logger->logError("failed to call GetFloatArrayElements in CcatTypeValue::getArrayElement.");
			this->setErrorMsg("Failed to get float array elements.");
			return;
		}
		this->floatValue = elems[index.numberValue];
		this->type = CTYPE_FLOAT;
		CcatParser::jvmEnv->jni->ReleaseFloatArrayElements((jfloatArray)(this->objectValue), elems, 0);
		AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);	
		this->objectValue = NULL;
		}
		break;
	case CTYPE_ARRAY_DOUBLE:
		{
		jdouble* elems = CcatParser::jvmEnv->jni->GetDoubleArrayElements((jdoubleArray)this->objectValue, 0);
		if(elems == NULL)
		{
			Global::logger->logError("failed to call GetDoubleArrayElements in CcatTypeValue::getArrayElement.");
			this->setErrorMsg("Failed to get double array elements.");
			return;
		}
		this->doubleValue = elems[index.numberValue];
		this->type = CTYPE_DOUBLE;
		CcatParser::jvmEnv->jni->ReleaseDoubleArrayElements((jdoubleArray)(this->objectValue), elems, 0);
		AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);	
		this->objectValue = NULL;
		}
		break;
	case CTYPE_ARRAY_BOOL:
		{
		jboolean* elems = CcatParser::jvmEnv->jni->GetBooleanArrayElements((jbooleanArray)this->objectValue, 0);
		if(elems == NULL)
		{
			Global::logger->logError("failed to call GetBooleanArrayElements in CcatTypeValue::getArrayElement.");
			this->setErrorMsg("Failed to get boolean array elements.");
			return;
		}
		this->boolValue = (elems[index.numberValue] == 0) ? false : true;
		this->type = CTYPE_BOOL;
		CcatParser::jvmEnv->jni->ReleaseBooleanArrayElements((jbooleanArray)(this->objectValue), elems, 0);
	    AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);	
		this->objectValue = NULL;
		}
		break;
	case CTYPE_ARRAY_OBJECT:
		{
		this->objectValue = CcatParser::jvmEnv->jni->GetObjectArrayElement((jobjectArray)objBak, index.numberValue);
		AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);
        setObjectType();
		}
		break;
	default:
		break;
	}
	if (false == this->hasError)
	{
		if (Logger::getLogger()->isDebugEnabled())
		{
			string value = "";
			printValue(value);
			Logger::getLogger()->logDebug("element value is %s ", value.c_str());
		}
	}
	return;
}

void CcatTypeValue::getObjectAttribute(jclass clazz, const char* p, bool isStaticMethod)
{
	Logger::getLogger()->logDebug("get value of object's attribute %s", p);
	jint count;
	jfieldID *fieldIds = NULL;
	CcatParser::jvmEnv->jvmti->GetClassFields(clazz, &count, &fieldIds);
	AUTO_REL_JVMTI_OBJECT(fieldIds);
	int i = 0;
	for (i = 0; i < count; i++)
	{
		char* fieldName = NULL;
		char* fieldType = NULL;
		jfieldID fieldId = fieldIds[i];
		CcatParser::jvmEnv->jvmti->GetFieldName(clazz, fieldId, &fieldName, &fieldType, NULL);
		AUTO_REL_JVMTI_OBJECT(fieldName);
		AUTO_REL_JVMTI_OBJECT(fieldType);

		if (strcmp(fieldName, p) != 0)
		{
			continue;
		}

		jint modifier;
		CcatParser::jvmEnv->jvmti->GetFieldModifiers(clazz, fieldIds[i], &modifier);

		if (isStaticMethod && ((modifier & ACC_STATIC) == 0))
		{
			this->setErrorMsg("Cann't access non class field in class method.");
			return;
		}
    
		jobject objBak = this->objectValue;
		if (strcmp(fieldType, "Z") == 0) //boolean
		{
			jboolean fieldValue = (modifier & ACC_STATIC) == 0 
				? CcatParser::jvmEnv->jni->GetBooleanField(this->objectValue, fieldId)
				: CcatParser::jvmEnv->jni->GetStaticBooleanField(clazz, fieldId);
            this->boolValue = (fieldValue == 0)?false:true;
			this->type = CTYPE_BOOL;
			AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);
			this->objectValue = NULL;
		}
		else if (strcmp(fieldType, "B") == 0)  //byte
		{
			jbyte fieldValue = (modifier & ACC_STATIC) == 0 
				? CcatParser::jvmEnv->jni->GetByteField(this->objectValue, fieldId)
				: CcatParser::jvmEnv->jni->GetStaticByteField(clazz, fieldId);
			this->numberValue = fieldValue;
			this->type = CTYPE_NUMBER;
			AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);
			this->objectValue = NULL;
		}
		else if (strcmp(fieldType, "C") == 0) //char
		{
			jchar fieldValue = (modifier & ACC_STATIC) == 0 
				? CcatParser::jvmEnv->jni->GetCharField(this->objectValue, fieldId)
				: CcatParser::jvmEnv->jni->GetStaticCharField(clazz, fieldId);

			if (fieldValue < 127)
			{
				this->utf8CharValue = new char[2];
				this->utf8CharValue[0] = fieldValue;
				this->utf8CharValue[1] = 0;
			}
			else
			{
				int charSize = 0;
				this->utf8CharValue = HelperFunc::utf16be_to_utf8(fieldValue,&charSize);
				if (charSize == -1)
				{
					string errMsg = "cann't get attribute value of [";
					errMsg += p;
					errMsg += "].";
					this->setErrorMsg(errMsg.c_str());
					break;
				}
			}
			this->type = CTYPE_CHAR;
			AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);
			this->objectValue = NULL;
		}
		else if (strcmp(fieldType, "D") == 0) //double
		{
			jdouble fieldValue = (modifier & ACC_STATIC) == 0 
				? CcatParser::jvmEnv->jni->GetDoubleField(this->objectValue, fieldId)
				: CcatParser::jvmEnv->jni->GetStaticDoubleField(clazz, fieldId);
			this->doubleValue = fieldValue;
			this->type = CTYPE_DOUBLE;
			AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);
			this->objectValue = NULL;			
		}
		else if (strcmp(fieldType, "F") == 0) //float
		{
			jfloat fieldValue = (modifier & ACC_STATIC) == 0 
				? CcatParser::jvmEnv->jni->GetFloatField(this->objectValue, fieldId)
				: CcatParser::jvmEnv->jni->GetStaticFloatField(clazz, fieldId);
			this->floatValue = fieldValue;
			this->type = CTYPE_FLOAT;
			AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);
			this->objectValue = NULL;
		}
		else if (strcmp(fieldType, "I") == 0) //int
		{
			jint fieldValue = (modifier & ACC_STATIC) == 0 
				? CcatParser::jvmEnv->jni->GetIntField(this->objectValue, fieldId)
				: CcatParser::jvmEnv->jni->GetStaticIntField(clazz, fieldId);
			this->numberValue = fieldValue;
			this->type = CTYPE_NUMBER;
			AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);
			this->objectValue = NULL;
		}
		else if (strcmp(fieldType, "S") == 0) //short
		{
			jint fieldValue = (modifier & ACC_STATIC) == 0 
				? CcatParser::jvmEnv->jni->GetShortField(this->objectValue, fieldId)
				: CcatParser::jvmEnv->jni->GetStaticShortField(clazz, fieldId);
			this->numberValue = fieldValue;
			this->type = CTYPE_NUMBER;
			AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);
			this->objectValue = NULL;
		}
		else if (strcmp(fieldType, "J") == 0) //long
		{
			jlong fieldValue = (modifier & ACC_STATIC) == 0 
				? CcatParser::jvmEnv->jni->GetLongField(this->objectValue, fieldId) 
				: CcatParser::jvmEnv->jni->GetStaticLongField(clazz, fieldId);
			this->numberValue = fieldValue;
			this->type = CTYPE_NUMBER;
			AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);
			this->objectValue = NULL;
		}
		else //object
		{
			this->objectValue = (modifier & ACC_STATIC) == 0 
				? CcatParser::jvmEnv->jni->GetObjectField(objBak, fieldId)
				: CcatParser::jvmEnv->jni->GetStaticObjectField(clazz, fieldId);
			setObjectType();
			AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, objBak);
		}
		break;
	}
	if ( i == count)
	{
		string errMsg = "attribute name [";
		errMsg += p;
		errMsg += "] does not exist.";
		this->setErrorMsg(errMsg.c_str());
	}
	if (false == this->hasError)
	{
        string value="";
		printValue(value);
		Logger::getLogger()->logDebug("value of object's attribute %s: %s", p, value.c_str());
	}
	return;

}

void CcatTypeValue::getObjectAttribute(const char* p)
{
    if (this->type < CTYPE_OBJECT_NULL)
	{
		this->setErrorMsg("Cann't get arrtibete of non object.");
		return;
	}
	else if (this->type == CTYPE_OBJECT_NULL)
	{
		this->setErrorMsg("Cann't get arrtibete of null object.");
		return;
	}
	//get object class
	jclass clazz = CcatParser::jvmEnv->jni->GetObjectClass(this->objectValue);
	if(clazz == NULL)
	{
		Global::logger->logError("Failed to call GetObjectClass in CcatTypeValue::getObjectAttribute.");
		this->setErrorMsg("Get object class failed.");
		return;
	}
    AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, clazz);
	
	//get class status
	jint status;
	jvmtiError err = CcatParser::jvmEnv->jvmti->GetClassStatus(clazz,&status);
	if(err != JVMTI_ERROR_NONE)
	{
		Global::logger->logError("Failed to call GetClassStatus in CcatTypeValue::getObjectAttribute,error cd =%d.",err);
		this->setErrorMsg("Get class status failed.");
		return;
	}

	//IBM JDK‚Ìê‡Astatus‚ª”»’f•s—v
	if(!Global::getIsIBMJvm())
	{	
		if(status == 0)
		{
			this->setErrorMsg("Class is unsafe.");
			return;
		}
	}

	getObjectAttribute(clazz, p, false);
	return;

}

void CcatTypeValue::getAttribute(const char* p)
{
	Logger::getLogger()->logDebug("get value of attribute \"%s\"", p);

	char *methodName = NULL, *methodSig = NULL;

	jmethodID methodId = CcatParser::jvmEnv->frameInfo[CcatParser::jvmEnv->depth].method;
	if (!HelperFunc::validateJvmtiError(CcatParser::jvmEnv->jvmti, 
		CcatParser::jvmEnv->jvmti->GetMethodName(methodId, &methodName, &methodSig, NULL), ""))
	{
		this->setErrorMsg("Cann't get method signature.");
		return;
	}
	AUTO_REL_JVMTI_OBJECT(methodName);
	AUTO_REL_JVMTI_OBJECT(methodSig);
	
	jint modifiers;
	bool isStaticMethod = false;
	if (!HelperFunc::validateJvmtiError(CcatParser::jvmEnv->jvmti,
			CcatParser::jvmEnv->jvmti->GetMethodModifiers(methodId, &modifiers), "GetMethodModifiers"))
	{
		this->setErrorMsg("Cann't get method modifiers.");
		return;
	}
	if(modifiers & 0x0008)
	{
		isStaticMethod = true;
		if (strcmp(p, "this") == 0)
		{
			this->setErrorMsg("The this attribute should not be access in class method.");
			return;
		}
	}
	else
	{
		getParameter("0");
		if (this->hasError)
		{
			return;
		}
		if (strcmp(p, "this") == 0)
		{
			return;
		}
	}

	jclass clazz;
	jvmtiError err = CcatParser::jvmEnv->jvmti->GetMethodDeclaringClass(methodId, &clazz);
	if (err != JVMTI_ERROR_NONE)
	{
		char szMsg[128];
		sprintf(szMsg, "Cann't get method's declaring class. errcode=[%d]", err);
		this->setErrorMsg(szMsg);
		return;
	}
	AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, clazz);

	getObjectAttribute(clazz, p, isStaticMethod);
	if (false == this->hasError)
	{
        string value="";
		printValue(value);
		Logger::getLogger()->logDebug("attribute value of \"%s\" : %s", p, value.c_str());
	}
	return;
}

void CcatTypeValue::getParameter(const char* p)
{
	Logger::getLogger()->logDebug("get value of parameter #%s", p);

	char *methodName = NULL, *methodSig = NULL;

	jmethodID methodId = CcatParser::jvmEnv->frameInfo[CcatParser::jvmEnv->depth].method;
	if (!HelperFunc::validateJvmtiError(CcatParser::jvmEnv->jvmti, 
		CcatParser::jvmEnv->jvmti->GetMethodName(methodId, &methodName, &methodSig, NULL), ""))
	{
		this->setErrorMsg("Cann't get method signature.");
		return;
	}
	AUTO_REL_JVMTI_OBJECT(methodName);
	AUTO_REL_JVMTI_OBJECT(methodSig);
	
	//get the number of method's parameters
	jint paramSlotNum = 0;
	paramSlotNum = HelperFunc::getMethodParamNum(methodSig);

	jint modifiers;
	bool isStaticMethod = false;
	if (!HelperFunc::validateJvmtiError(CcatParser::jvmEnv->jvmti,
			CcatParser::jvmEnv->jvmti->GetMethodModifiers(methodId, &modifiers), "GetMethodModifiers"))
	{
		this->setErrorMsg("Cann't get method modifiers.");
		return;
	}
	if(modifiers & 0x0008)
	{
		isStaticMethod = true;
	}
	else
	{
		paramSlotNum += 1;
	}

    int paraIndex = atoi(p);
	if (isStaticMethod)
	{
		paraIndex -= 1;
	}
	if (paraIndex > paramSlotNum-1)
	{
		this->setErrorMsg("Parameter index is out of range.");
		return;
	}

	char* varTypes = new char[paramSlotNum + 1];
	for (int i = 0; i < paramSlotNum; i++)
	{
		varTypes[i] = 'U';
	}
	if (!isStaticMethod)
	{
		varTypes[0] = 'L';   //this object
	}
	varTypes[paramSlotNum] = 0;
    HelperFunc::getMethodParamType(methodSig,paramSlotNum,isStaticMethod,varTypes);

	int nSlotIndex = getParamSlotIndex(paraIndex, varTypes, paramSlotNum);
	if (nSlotIndex == -1)
	{
		this->setErrorMsg("Parameter index is out of range.");
		return;
	}

	char szTmp[2];
	szTmp[0] = varTypes[nSlotIndex];
	szTmp[1] = 0;
	if (JvmUtilFunc::isPrimitiveType(szTmp))
	{
		char* valueStr = NULL;
		if (varTypes[nSlotIndex] == 'C')
		{
			jint value;
			CcatParser::jvmEnv->jvmti->GetLocalInt(CcatParser::jvmEnv->thread, 
								CcatParser::jvmEnv->depth,
								nSlotIndex, &value);
			
			int charSize = 0;
			char* utf8Char = HelperFunc::utf16be_to_utf8(value,&charSize);
			if (charSize == -1)
			{
				valueStr = new char[5];
				sprintf(valueStr, "\\%x", value);
			}
			else
			{
				valueStr = utf8Char;
			}
		}
		else
		{
			jvmtiError err = JVMTI_ERROR_NONE;
			valueStr = JvmUtilFunc::getPrimitiveLocalVariableValue(CcatParser::jvmEnv->jvmti,
				CcatParser::jvmEnv->thread, CcatParser::jvmEnv->depth, 
				nSlotIndex, varTypes[nSlotIndex], &err);
		}
		switch(varTypes[nSlotIndex])
		{
		case 'B':
		case 'S':
		case 'I':
		case 'J':
            this->numberValue = atol(valueStr);
			this->type = CTYPE_NUMBER;
			break;
		case 'F':
            this->floatValue = atof(valueStr);
			this->type = CTYPE_FLOAT;
			break;
		case 'D':
            this->doubleValue = atof(valueStr);
			this->type = CTYPE_DOUBLE;
			break;
		case 'C':
            this->utf8CharValue = HelperFunc::strdup(valueStr);
			this->type = CTYPE_CHAR;
			break;
		case 'Z':
			if (strcmp(valueStr, "true") == 0)
				this->boolValue = true;
			else
				this->boolValue = false;
			this->type = CTYPE_BOOL;
			break;
		default:
			break;
		}
		delete valueStr;
    }
	else
	{
		jvmtiError err = CcatParser::jvmEnv->jvmti->GetLocalObject(CcatParser::jvmEnv->thread, 
														CcatParser::jvmEnv->depth, nSlotIndex, 
													    &(this->objectValue));
		if (err != JVMTI_ERROR_NONE)
		{
			char szMsg[128];
			sprintf(szMsg, "Cann't get object type's parameter. jvm_errcode=[%d]", err);
			this->setErrorMsg(szMsg);
			return;
		}
		setObjectType();
	}
	if (false == this->hasError)
	{
        string value="";
		printValue(value);
		Logger::getLogger()->logDebug("parameter value of #%s : %s", p, value.c_str());
	}
	return;
}

void CcatTypeValue::setObjectType()
{
	if (this->objectValue == NULL)
	{
		this->type = CTYPE_OBJECT_NULL;
		return;
	}
    string a;
	//get objectclass
	jclass clazz = CcatParser::jvmEnv->jni->GetObjectClass(this->objectValue);
	if(clazz == NULL)
	{
		Global::logger->logError("failed to call GetObjectClass in CcatTypeValue::getParameter.");
		this->setErrorMsg("Get object class failed.");
		return;
	}
	AUTO_REL_JNI_LOCAL_REF(CcatParser::jvmEnv->jni, clazz);
	
	//get class status
	jint status;
	jvmtiError err = CcatParser::jvmEnv->jvmti->GetClassStatus(clazz,&status);
	if(err != JVMTI_ERROR_NONE)
	{
		Global::logger->logError("failed to call GetClassStatus in CcatTypeValue::getParameter,error cd =%d.",err);
		this->setErrorMsg("Get class status failed.");
		return;
	}

	//IBM JDK‚Ìê‡Astatus‚ª”»’f•s—v
	if(!Global::getIsIBMJvm())
	{	
		if(status == 0)
		{
			this->setErrorMsg("Class is unsafe.");
			return;
		}
	}

	char *sig = NULL;
	err = CcatParser::jvmEnv->jvmti->GetClassSignature(clazz, &sig, NULL);
	if(err != JVMTI_ERROR_NONE)
	{
		Global::logger->logError("failed to call GetClassSignature in CcatTypeValue::getParameter,error cd =%d.",err);
		this->setErrorMsg("Get class signature failed.");
		return;
	}
	AUTO_REL_JVMTI_OBJECT(sig);

	if (strncmp(sig, "Ljava/lang/", 11) == 0)
	{
		if (strcmp(sig, "Ljava/lang/String;") == 0)
		{
			this->type = CTYPE_OBJECT_STRING;
		}
		else if (strcmp(sig, "Ljava/lang/Character;") == 0)
		{
			this->type = CTYPE_OBJECT_CHARACTER;
		}
		else if (strcmp(sig, "Ljava/lang/Byte;") == 0)
		{
			this->type = CTYPE_OBJECT_BYTE;
		}
		else if (strcmp(sig, "Ljava/lang/Short;") == 0)
		{
			this->type = CTYPE_OBJECT_SHORT;
		}
		else if (strcmp(sig, "Ljava/lang/Integer;") == 0)
		{
			this->type = CTYPE_OBJECT_INTEGER;
		}
		else if (strcmp(sig, "Ljava/lang/Long;") == 0)
		{
			this->type = CTYPE_OBJECT_LONG;
		}
		else if (strcmp(sig, "Ljava/lang/Float;") == 0)
		{
			this->type = CTYPE_OBJECT_FLOAT;
		}
		else if (strcmp(sig, "Ljava/lang/Double;") == 0)
		{
			this->type = CTYPE_OBJECT_DOUBLE;
		}
		else if (strcmp(sig, "Ljava/lang/Boolean;") == 0)
		{
			this->type = CTYPE_OBJECT_BOOLEAN;
		}
		else
		{
			this->type = CTYPE_OBJECT_OTHER;
		}
	}else if (strcmp(sig, "Ljava/util/Date;") == 0)
	{
		this->type = CTYPE_OBJECT_DATE;
	}else if (strcmp(sig, "Ljava/sql/Date;") == 0)
	{
		this->type = CTYPE_OBJECT_DATE;
	}else if (strcmp(sig, "Ljava/sql/Time;") == 0)
	{
		this->type = CTYPE_OBJECT_DATE;
	}else if (sig[0] == '[')
	{
		switch (sig[1])
		{
		case 'B':
			this->type = CTYPE_ARRAY_BYTE;
			break;
		case 'S':
			this->type = CTYPE_ARRAY_SHORT;
			break;
		case 'I':
			this->type = CTYPE_ARRAY_INTEGER;
			break;
		case 'J':
			this->type = CTYPE_ARRAY_LONG;
			break;
		case 'F':
			this->type = CTYPE_ARRAY_FLOAT;
			break;
		case 'D':
			this->type = CTYPE_ARRAY_DOUBLE;
			break;
		case 'C':
			this->type = CTYPE_ARRAY_CHARACTER;
			break;
		case 'Z':
			this->type = CTYPE_ARRAY_BOOL;
			break;
		default:
			this->type = CTYPE_ARRAY_OBJECT;
			break;
		}
	}
	else 
	{
		this->type = CTYPE_OBJECT_OTHER;
	}
}

int CcatTypeValue::getParamSlotIndex(int paraIndex, char* varTypes, int paramSlotNum)
{
	int nRet = -1;
	int cnt = -1;
	for (int i = 0; i < paramSlotNum; i++)
	{
		cnt++;
		if (cnt == paraIndex)
		{
			nRet = i;
		}
		if ((varTypes[i] == 'J') || (varTypes[i] == 'D'))
		{
			i++;
		}
	}
    return nRet;
}

void CcatTypeValue::printValue(string& valueStr)
{
	if (this->hasError)
	{
		valueStr = "";
		return;
	}
    char  szTmp[MAX_MSG_LEN+1];
	switch (this->type)
	{
	   case CTYPE_NONE :
		   valueStr = "";
		   break;
	   case CTYPE_BOOL:
		   if (this->boolValue)
		   {
			   valueStr = "true";
		   }
		   else
		   {
			   valueStr = "false";
		   }
		   break;
	   case CTYPE_NUMBER:
#ifdef _LINUX
		   sprintf(szTmp, "%lld", this->numberValue);
#else
		   sprintf(szTmp, "%I64d", this->numberValue);
#endif
		   valueStr = szTmp;
		   break;
	   case CTYPE_FLOAT:
		   sprintf(szTmp, "%G", this->floatValue);
		   valueStr = szTmp;
		   break;
	   case CTYPE_DOUBLE:
		   sprintf(szTmp, "%G", this->doubleValue);
		   valueStr = szTmp;
		   break;
	   case CTYPE_CHAR:
		   sprintf(szTmp, "%s", this->utf8CharValue);
		   valueStr = szTmp;
		   break;
	   case CTYPE_STRING:
	   case CTYPE_DATE_STRING:
		   valueStr = this->stringValue;
		   break;
	   case CTYPE_OBJECT_NULL:
		   valueStr = "null object";
		   break;
	   case CTYPE_OBJECT_STRING:
		   valueStr = "String object";
		   break;
	   case CTYPE_OBJECT_CHARACTER:
		   valueStr = "Character object";
		   break;
	   case CTYPE_OBJECT_BYTE:
		   valueStr = "Byte object";
		   break;
	   case CTYPE_OBJECT_SHORT:
		   valueStr = "Short object";
		   break;
	   case CTYPE_OBJECT_INTEGER:
		   valueStr = "Integer object";
		   break;
	   case CTYPE_OBJECT_LONG:
		   valueStr = "Long object";
		   break;
	   case CTYPE_OBJECT_FLOAT:
		   valueStr = "Float object";
		   break;
	   case CTYPE_OBJECT_DOUBLE:
		   valueStr = "Double object";
		   break;
	   case CTYPE_OBJECT_BOOLEAN:
		   valueStr = "Boolean object";
		   break;
	   case CTYPE_OBJECT_DATE:
		   valueStr = "Date object";
		   break;
	   case CTYPE_OBJECT_OTHER:
		   valueStr = "Other object";
		   break;
	   case CTYPE_ARRAY_CHARACTER:
		   valueStr = "char[] object";
		   break;
	   case CTYPE_ARRAY_BYTE:
		   valueStr = "byte[] object";
		   break;
	   case CTYPE_ARRAY_SHORT:
		   valueStr = "short[] object";
		   break;
	   case CTYPE_ARRAY_INTEGER:
		   valueStr = "int[] object";
		   break;
	   case CTYPE_ARRAY_LONG:
		   valueStr = "long[] object";
		   break;
	   case CTYPE_ARRAY_FLOAT:
		   valueStr = "float[] object";
		   break;
       case CTYPE_ARRAY_DOUBLE:
		   valueStr = "double[] object";
		   break;
	   case CTYPE_ARRAY_BOOL:
		   valueStr = "bool[] object";
		   break;
	   case CTYPE_ARRAY_OBJECT:
		   valueStr = "other array object";
		   break;
	}
}
