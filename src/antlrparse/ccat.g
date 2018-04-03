header{
#include <stdlib.h>
#include <stdio.h>
#include "CcatTypeValue.hpp"
#include "CcatFormatException.hpp"
#include "UnicodeCharBuffer.hpp"
#include "UnicodeCharScanner.hpp"
BEGIN_NS(NS_ANTLRPARSE)
}

options
{
language="Cpp";
}

{
#include "../common/HelperFunc.h"

USE_NS(NS_ANTLRPARSE)
USE_NS(NS_COMMON)

ParserParam* CcatParser::jvmEnv = NULL;
bool         CcatParser::isMinus = false;

extern __int64 strtoll(char const *p,char **pend,int base);

char* CcatParser::convertStringLiteral(char* pStringLiteral, string& errMsg)
{
	errMsg = "";
	int size = strlen(pStringLiteral);
	if (size == 0)
	{
	    char* pResult = new char[1];
	    pResult[0] = 0;
	    return pResult;
	}
	
	char*  pResult = new char[size + 1];
	memset(pResult, 0, size + 1);

	int nPos = 0;
	int nResultPos = 0;
	while(nPos < size)
	{
		if (pStringLiteral[nPos] != '\\')   //copy utf-8 char
		{
			unsigned char ch = (unsigned char)(pStringLiteral[nPos]);
			if( (ch & 0x80) == 0 )
			{
				pResult[nResultPos++] = ch;
				nPos++;
				continue;
			}

			short need = 0;
			if( (ch & 0xF0) == 0xF0 )
			{
				need = 4;
			}
			else if( (ch & 0xE0) == 0xE0 )
			{
				need = 3;
			}
			else if( (ch & 0xC0) == 0xC0 )
			{
				need = 2;
			}
			else
			{
				//Invalid utf-8
				need = 1;
			}
			
			strncpy(pResult + nResultPos, pStringLiteral+nPos, need);
			nResultPos += need;
			nPos += need;
			continue;
		}
		
		switch(pStringLiteral[nPos+1])
		{
		case 'b':
			pResult[nResultPos++] = '\b';
			nPos += 2;
			break;
		case 't':
			pResult[nResultPos++] = '\t';
			nPos += 2;
			break;
		case 'n':
			pResult[nResultPos++] = '\n';
			nPos += 2;
			break;
		case 'f':
			pResult[nResultPos++] = '\f';
			nPos += 2;
			break;
		case 'r':
			pResult[nResultPos++] = '\r';
			nPos += 2;
			break;
		case '\\':
			pResult[nResultPos++] = '\\';
			nPos += 2;
			break;
		case '"':
			pResult[nResultPos++] = '"';
			nPos += 2;
			break;
		case '\'':
			pResult[nResultPos++] = '\'';
			nPos += 2;
			break;
		case 'u':
		{
			char szTmp[7];
			strcpy(szTmp, "0x");
			strncpy(szTmp+2, pStringLiteral+nPos+2, 4);
			szTmp[6] = 0;
			unsigned int nTmp = strtoll(szTmp, (char**)NULL, 16);
			
			if (nTmp > 127)
			{
				int nBytesCount = 0;
				char* pszTmp = HelperFunc::utf16be_to_utf8(nTmp, &nBytesCount);
				if (pszTmp == NULL)
				{
					 errMsg = "cann't convert unicode char: \\u";
					 errMsg += (szTmp + 2);
					 errMsg += ".";
					 return NULL;
				}
				else
				{
					strcpy(pResult+nResultPos, pszTmp);
					nResultPos+=nBytesCount;
					delete[] pszTmp;
				}
			}
			else
			{
				pResult[nResultPos++] = nTmp;
			}
			nPos += 6;
			break;
		}
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		{
			char szTmp[5];
			strcpy(szTmp, "0");
			short len = 1;
			if ((pStringLiteral[nPos+2] >= '0') && (pStringLiteral[nPos+2] <= '7'))
			{
				len++;
				if ((pStringLiteral[nPos+3] >= '0') && (pStringLiteral[nPos+3] <= '7'))
				{
					len++;
				}
			}
			
			strncpy(szTmp+1, pStringLiteral+nPos+1, len);
			szTmp[len+1] = 0;
			unsigned int nTmp = strtoll(szTmp, (char**)NULL, 8);
			
			if (nTmp > 127)
			{
				int nBytesCount = 0;
				char* pszTmp = HelperFunc::utf16be_to_utf8(nTmp, &nBytesCount);
				if (pszTmp == NULL)
				{
					 errMsg = "cann't convert octal char: \\";
					 errMsg += (szTmp + 1);
					 errMsg += ".";
					 return NULL;
				}
				else
				{
					strcpy(pResult+nResultPos, pszTmp);
					nResultPos+=nBytesCount;
					delete[] pszTmp;
				}
			}
			else
			{
				pResult[nResultPos++] = nTmp;
			}
			nPos += (len+1);
			break;
		}
		default:
			errMsg = "character literal is not valid:";
			errMsg += pStringLiteral;
			errMsg += ".";
			return NULL;
		} //end of switch
	} //end of while
	
	return pResult;
}

}

class CcatParser extends Parser;
options {
	genHashLines = false;		// include line number information
}

{
private:
	char* convertStringLiteral(char* pStringLiteral, string& errMsg);

public:
    static ParserParam* jvmEnv;
    static bool isMinus;
}

expr [char* errMsg, bool isCompile, ParserParam* parserParam] returns [bool ret = false]
{
jvmEnv = parserParam;
CcatTypeValue tmp;
isMinus = false;
}
:
(tmp = expression[isCompile]
{
 if (tmp.hasError)
 {
   strcpy(errMsg, tmp.errorMsg);
   ret = false;
   return ret;
 }
}
)
?
COMMA
{
if (!isCompile)
{
	if (tmp.hasError)
	{
	   strcpy(errMsg, tmp.errorMsg);
	} 
	else if (tmp.type == CcatTypeValue::CTYPE_BOOL)
	{
	   ret = tmp.boolValue;
	}
	else if (tmp.type == CcatTypeValue::CTYPE_OBJECT_BOOLEAN)
	{
		tmp.getObjectValue();
		if (tmp.hasError)
		{
			strcpy(errMsg, tmp.errorMsg);
		}
		else
		{
			ret = tmp.boolValue;
		}
	}
	else
	{
	   strcpy(errMsg, "Not condition expresion.");
	}
}
else
{
	if ((tmp.type != CcatTypeValue::CTYPE_NONE)
		&& (tmp.type != CcatTypeValue::CTYPE_BOOL))
	{
		strcpy(errMsg, "Not condition expresion.");
		ret = false;
	}
	else
	{
		ret = true;
	}
}
}
;
exception
catch [ANTLR_USE_NAMESPACE(antlr)ANTLRException & ex1] {
// catch all exceptions and report it
//reportError(ex.toString());
strcpy(errMsg, ex1.toString().c_str());
}
catch [CcatFormatException & ex2] {
strcpy(errMsg, ex2.getErrorMsg());
}

expression[bool onlyCompile] returns [CcatTypeValue ret]
{bool bTmp = onlyCompile;}
:
ret = logical_and_expression[onlyCompile]
{
if (!onlyCompile)
{
	if (ret.hasError)
	{
	    return ret;
	}
}
}
(m:OR 
{CcatTypeValue tmp;
 if (!bTmp)
 {
	if (ret.hasError)
	{
		return ret;
	}
	else if (ret.type == CcatTypeValue::CTYPE_BOOL)
	{
		
	}
	else if (ret.type == CcatTypeValue::CTYPE_OBJECT_BOOLEAN)
	{
		ret.getObjectValue();
		if (ret.hasError)
		{
		    return ret;
		}
	}
	else
	{
		ret.setErrorMsg("The operator [||]'s operands are not match.");
		return ret;
	}
	bTmp = ret.boolValue;
 }
}
tmp=logical_and_expression[bTmp]
{
if (!bTmp)
{
	if (tmp.hasError)
	{
	   ret.setErrorMsg(tmp.errorMsg);
	   return ret;
	}
	ret = ret || tmp; 
}
else
{
	if (((ret.type != CcatTypeValue::CTYPE_NONE)
		&& (ret.type != CcatTypeValue::CTYPE_BOOL))
		||
	  ((tmp.type != CcatTypeValue::CTYPE_NONE)
		&& (tmp.type != CcatTypeValue::CTYPE_BOOL))
		)
	{
		throw CcatFormatException("The operator [||]'s operands are not match. ", m->getLine(), m->getColumn());
	}
	ret.type = CcatTypeValue::CTYPE_BOOL;
}
}
)*
;
exception
catch [ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex] {
throw ex;
}

logical_and_expression[bool onlyCompile] returns [CcatTypeValue ret]
{bool bTmp = onlyCompile; }
:
ret = equality_expression[onlyCompile]
{
if (!onlyCompile)
{
	if (ret.hasError)
	{
	    return ret;
	}
}
}
(m:AND 
{CcatTypeValue tmp;
 if (!bTmp)
 {
	if (ret.hasError)
	{
		return ret;
	}
	else if (ret.type == CcatTypeValue::CTYPE_BOOL)
	{
		
	}
	else if (ret.type == CcatTypeValue::CTYPE_OBJECT_BOOLEAN)
	{
		ret.getObjectValue();
		if (ret.hasError)
		{
		    return ret;
		}
	}
	else
	{
		ret.setErrorMsg("The operator [||]'s operands are not match.");
		return ret;
	}
	bTmp = !(ret.boolValue);
 }
}
tmp = equality_expression[bTmp]
{
if (!bTmp)
{
	if (tmp.hasError)
	{
	   ret.setErrorMsg(tmp.errorMsg);
	   return ret;
	}
	ret = ret && tmp; 
}
else
{
	if (((ret.type != CcatTypeValue::CTYPE_NONE)
		&& (ret.type != CcatTypeValue::CTYPE_BOOL))
		||
	  ((tmp.type != CcatTypeValue::CTYPE_NONE)
		&& (tmp.type != CcatTypeValue::CTYPE_BOOL))
		)
	{
		throw CcatFormatException("The operator [&&]'s operands are not match. ", m->getLine(), m->getColumn());
	}
	ret.type = CcatTypeValue::CTYPE_BOOL;
}
}
)*
;
exception
catch [ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex] {
throw ex;
}

equality_expression[bool onlyCompile] returns [CcatTypeValue ret]
{bool isEqual = false;}
:
ret = relational_expression[onlyCompile]
{
if (!onlyCompile)
{
	if (ret.hasError)
	{
		return ret;
	}
}
}
((
  m:NOTEQUAL {isEqual = false;}
  |
  n:EQUAL    {isEqual = true;}
 ) 
{CcatTypeValue tmp;}
tmp = relational_expression[onlyCompile]
{
if (!onlyCompile)
{
	if (tmp.hasError)
	{
		ret.setErrorMsg(tmp.errorMsg);
		return ret;
	}
	if (isEqual)
	   ret = (ret == tmp);
	else
	   ret = (ret != tmp);
}
else
{
	if ((ret.type == CcatTypeValue::CTYPE_NONE)
		|| (tmp.type == CcatTypeValue::CTYPE_NONE))
	{
		ret.type = CcatTypeValue::CTYPE_BOOL;
	}
	else if (((ret.type == CcatTypeValue::CTYPE_NUMBER)
			|| (ret.type == CcatTypeValue::CTYPE_FLOAT)
			|| (ret.type == CcatTypeValue::CTYPE_DOUBLE))
		&&
	  		((tmp.type == CcatTypeValue::CTYPE_NUMBER) 
			|| (tmp.type == CcatTypeValue::CTYPE_FLOAT)
			|| (tmp.type == CcatTypeValue::CTYPE_DOUBLE)))
	{
		ret.type = CcatTypeValue::CTYPE_BOOL;
	}
	else if (ret.type == tmp.type)
	{
		ret.type = CcatTypeValue::CTYPE_BOOL;
	}
	else
	{
		if (isEqual)
		{
			throw CcatFormatException("The operator [==]'s operands are not match. ", n->getLine(), n->getColumn());
		}
		else
		{
			throw CcatFormatException("The operator [!=]'s operands are not match. ", m->getLine(), m->getColumn());
		}
	}
}
}
)*
;
exception
catch [ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex] {
throw ex;
}

relational_expression[bool onlyCompile]  returns [CcatTypeValue ret]
{short op_kind = 0;}
:
ret = additive_expression[onlyCompile] 
{
if (!onlyCompile)
{
	if (ret.hasError)
	{
		return ret;
	}
}
}
(options {warnWhenFollowAmbig = false;}:
	(	m:LESSTHAN               {op_kind = 1;}
	|	n:GREATERTHAN            {op_kind = 2;}
	|	k:LESSTHANOREQUALTO      {op_kind = 3;}
	|	g:GREATERTHANOREQUALTO   {op_kind = 4;}
    )
{CcatTypeValue tmp;}
tmp = additive_expression[onlyCompile]
{
if (!onlyCompile)
{
	if (tmp.hasError)
	{
		ret.setErrorMsg(tmp.errorMsg);
		return ret;
	}
	switch(op_kind)
	{
	case 1:
		ret = (ret < tmp); 
		break;
	case 2:
		ret = (ret > tmp); 
		break;
	case 3:
		ret = (ret <= tmp); 
		break;
	case 4:
		ret = (ret >= tmp); 
		break;
	default:
		break;
	}
}
else
{
	bool isTypeMismatch = false;
	if ((ret.type == CcatTypeValue::CTYPE_OBJECT_NULL)
		|| (tmp.type == CcatTypeValue::CTYPE_OBJECT_NULL))
	{
		isTypeMismatch = true;
	}
	else if ((ret.type == CcatTypeValue::CTYPE_BOOL)
		|| (tmp.type == CcatTypeValue::CTYPE_BOOL))
	{
		isTypeMismatch = true;
	}
	else if ((ret.type == CcatTypeValue::CTYPE_NONE)
		|| (tmp.type == CcatTypeValue::CTYPE_NONE))
	{
		ret.type = CcatTypeValue::CTYPE_BOOL;
	}
	else if (((ret.type == CcatTypeValue::CTYPE_NUMBER)
			|| (ret.type == CcatTypeValue::CTYPE_FLOAT)
			|| (ret.type == CcatTypeValue::CTYPE_DOUBLE))
		&&
	  		((tmp.type == CcatTypeValue::CTYPE_NUMBER) 
			|| (tmp.type == CcatTypeValue::CTYPE_FLOAT)
			|| (tmp.type == CcatTypeValue::CTYPE_DOUBLE)))
	{
		ret.type = CcatTypeValue::CTYPE_BOOL;
	}
	else if (ret.type == tmp.type)
	{
		ret.type = CcatTypeValue::CTYPE_BOOL;
	}
	else
	{
		isTypeMismatch = true;
	}
	if (isTypeMismatch)
	{
		switch(op_kind)
		{
		case 1:
			throw CcatFormatException("The operator [<]'s operands are not match. ", m->getLine(), m->getColumn());
			break;
		case 2:
			throw CcatFormatException("The operator [>]'s operands are not match. ", n->getLine(), n->getColumn());
			break;
		case 3:
			throw CcatFormatException("The operator [<=]'s operands are not match. ", k->getLine(), k->getColumn());
			break;
		case 4:
			throw CcatFormatException("The operator [>=]'s operands are not match. ", g->getLine(), g->getColumn());
			break;
		default:
			break;
		}
	}
}
}
)*
;
exception
catch [ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex] {
throw ex;
}

additive_expression[bool onlyCompile] returns [CcatTypeValue ret]
{bool isPlus = false;}
:
ret = multiplicative_expression[onlyCompile]
{
if (!onlyCompile)
{
	if (ret.hasError)
	{
		return ret;
	}
}
}
(options{warnWhenFollowAmbig = false;}:
 (
  m:PLUS   {isPlus = true;}
| 
  n:MINUS  {isPlus = false;}
 ) 
{CcatTypeValue tmp;}
tmp = multiplicative_expression[onlyCompile]
{
if (!onlyCompile)
{
	if (tmp.hasError)
	{
		ret.setErrorMsg(tmp.errorMsg);
		return ret;
	}
	if (isPlus)
		ret = (ret + tmp); 
	else
		ret = (ret - tmp);
}
else
{
	if (((ret.type != CcatTypeValue::CTYPE_NONE)
		&& (ret.type != CcatTypeValue::CTYPE_NUMBER) 
		&& (ret.type == CcatTypeValue::CTYPE_FLOAT)
		&& (ret.type != CcatTypeValue::CTYPE_DOUBLE))
		||
	  ((tmp.type != CcatTypeValue::CTYPE_NONE)
		&& (tmp.type != CcatTypeValue::CTYPE_NUMBER) 
		&& (tmp.type == CcatTypeValue::CTYPE_FLOAT)
		&& (tmp.type != CcatTypeValue::CTYPE_DOUBLE))
		)
	{
		if (isPlus)
		{
			throw CcatFormatException("The operator [+]'s operands are not match. ", m->getLine(), m->getColumn());
		}
		else
		{
			throw CcatFormatException("The operator [-]'s operands are not match. ", n->getLine(), n->getColumn());
		}
	}
	ret.type = CcatTypeValue::CTYPE_DOUBLE;
}
}
)*
;
exception
catch [ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex] {
throw ex;
}

multiplicative_expression[bool onlyCompile] returns [CcatTypeValue ret]
{short op_kind = 0;}
:
ret = unary_expression[onlyCompile] 
{
if (!onlyCompile)
{
	if (ret.hasError)
	{
		return ret;
	}
}
}
(options{warnWhenFollowAmbig = false;}:
(l:STAR    {op_kind = 1;}
|
 m:DIVIDE  {op_kind = 2;}
|
 n:MOD     {op_kind = 3;}
)
{CcatTypeValue tmp;}
tmp = unary_expression[onlyCompile]
{
if (!onlyCompile)
{
	if (tmp.hasError)
	{
		ret.setErrorMsg(tmp.errorMsg);
		return ret;
	}
	switch(op_kind)
	{
	case 1:
		ret = (ret * tmp); 
		break;
	case 2:
		ret = (ret / tmp); 
		break;
	case 3:
		ret = (ret % tmp); 
		break;
	default:
		break;
	}
}
else
{
	if (((ret.type != CcatTypeValue::CTYPE_NONE)
		&& (ret.type != CcatTypeValue::CTYPE_NUMBER) 
		&& (ret.type == CcatTypeValue::CTYPE_FLOAT)
		&& (ret.type != CcatTypeValue::CTYPE_DOUBLE))
		||
	  ((tmp.type != CcatTypeValue::CTYPE_NONE)
		&& (tmp.type != CcatTypeValue::CTYPE_NUMBER) 
		&& (tmp.type == CcatTypeValue::CTYPE_FLOAT)
		&& (tmp.type != CcatTypeValue::CTYPE_DOUBLE))
		)
	{
		switch (op_kind)
		{
		case 1:
			throw CcatFormatException("The operator [*]'s operands are not match. ", l->getLine(), l->getColumn());
			break;
		case 2:
			throw CcatFormatException("The operator [/]'s operands are not match. ", m->getLine(), m->getColumn());
			break;
		case 3:
			throw CcatFormatException("The operator [%]'s operands are not match. ", n->getLine(), n->getColumn());
			break;
		default:
			break;
		}
	}
	ret.type = CcatTypeValue::CTYPE_DOUBLE;
}
}
)*
;
exception
catch [ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex] {
throw ex;
}
	
unary_expression[bool onlyCompile]  returns [CcatTypeValue ret]
:
	{bool beMinus = false;}
	m:MINUS 
	{
	beMinus = true;
	while ((LA(1) == MINUS) || (LA(1) == PLUS))
	{
		if (LA(1) == MINUS)
		{
			match(MINUS);
			beMinus = !beMinus;
		}
		else
		{
			match(PLUS);
		}
	}
	if ((LA(1) == HexLiteral) || (LA(1) == OctalLiteral) 
		|| (LA(1) == DecimalLiteral) || (LA(1) == FloatingPointLiteral))
	{
		isMinus = beMinus;
		beMinus = false;
	}
	}
	ret = unary_expression[onlyCompile]
	{
	if (!onlyCompile)
	{
		if (ret.hasError)
		{
			return ret;
		}
		if (ret.type == CcatTypeValue::CTYPE_NONE)
		{
		}
		else
		{
		    if (beMinus)
		    {
		    	ret = -ret;
		    }
		}
	}
	else
	{
		if ((ret.type != CcatTypeValue::CTYPE_NONE)
			&& (ret.type != CcatTypeValue::CTYPE_NUMBER) 
			&& (ret.type == CcatTypeValue::CTYPE_FLOAT)
			&& (ret.type != CcatTypeValue::CTYPE_DOUBLE))
		{
			throw CcatFormatException("The operator[-]'s operand is not match. ", m->getLine(), m->getColumn());
		}
		ret.type = CcatTypeValue::CTYPE_DOUBLE;
	}
	}
|	
	n:PLUS  ret = unary_expression[onlyCompile]
	{
	if (!onlyCompile)
	{
		if (ret.hasError)
		{
			return ret;
		}
		if (ret.type == CcatTypeValue::CTYPE_NONE)
		{
		}
		else
		{
		    ret = +ret;
		}
	}
	else
	{
		if ((ret.type != CcatTypeValue::CTYPE_NONE)
			&& (ret.type != CcatTypeValue::CTYPE_NUMBER) 
			&& (ret.type == CcatTypeValue::CTYPE_FLOAT)
			&& (ret.type != CcatTypeValue::CTYPE_DOUBLE))
		{
			throw CcatFormatException("The operator [+]'s operand is not match. ", n->getLine(), n->getColumn());
		}
		ret.type = CcatTypeValue::CTYPE_DOUBLE;
	}
	}
|	
	ret = unary_expression_notplusminus[onlyCompile]
;
exception
catch [ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex] {
throw ex;
}

unary_expression_notplusminus[bool onlyCompile] returns [CcatTypeValue ret]
:
	ret = primary_expression[onlyCompile]
|	
	m:NOT ret = unary_expression[onlyCompile]
	{
	if (!onlyCompile)
	{
		if (ret.hasError)
		{
			return ret;
		}
		if (ret.type == CcatTypeValue::CTYPE_NONE)
		{
		}
		else
		{
			ret = !ret;
		}
	}
	else
	{
		if ((ret.type != CcatTypeValue::CTYPE_BOOL)
			&& (ret.type != CcatTypeValue::CTYPE_NONE))
		{
			throw CcatFormatException("The operator [!]'s operand is not match. ", m->getLine(), m->getColumn());
		}
		ret.type = CcatTypeValue::CTYPE_BOOL;
	}
	}
;
exception
catch [ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex] {
throw ex;
}

primary_expression[bool onlyCompile] returns [CcatTypeValue ret]
:
	LPAREN 
	ret = expression[onlyCompile]
	{
	if (!onlyCompile)
	{
		if (ret.hasError)
		{
			return ret;
		}
	}
	}
	RPAREN
|  
	ret = literal[onlyCompile]
|   
	ret = fieldName[onlyCompile]
;
exception
catch [ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex] {
throw ex;
}

literal[bool onlyCompile] returns [CcatTypeValue ret]
:
	ret = integerLiteral[onlyCompile]
|	
	i:CharacterLiteral
	{
	ret.type = CcatTypeValue::CTYPE_CHAR;
	if (!onlyCompile)
	{
		string str = i->getText();
    const char* p = str.c_str();
		int length = i->getText().length();
		
		char* pszTmp = new char[length];
		strncpy(pszTmp, p+1, length-2);
		pszTmp[length-2] = 0;
		string errMsg = "";
		char* pResult = convertStringLiteral(pszTmp, errMsg);
		if (pResult == NULL)
		{
			ret.setErrorMsg(errMsg.c_str());
		}
		else
		{
			ret.utf8CharValue = pResult;
		}
	}
	}
|	
	j:StringLiteral
	{
	ret.type = CcatTypeValue::CTYPE_STRING;
	if (!onlyCompile)
	{
		int length = j->getText().length();
		if (length == 2)
		{
			ret.stringValue = "";
		}
		else
		{
		  string str = j->getText();
			const char* p = str.c_str();
			char* pszTmp = new char[length];
			strncpy(pszTmp, p+1, length-2);
			pszTmp[length-2] = 0;
			string errMsg = "";
			char* pResult = convertStringLiteral(pszTmp,errMsg);
			if (pResult == NULL)
			{
				ret.setErrorMsg(errMsg.c_str());
			}
			else
			{
				ret.stringValue = pResult;
				delete[] pResult;
			}
		}
	}		
	}		
|	
	l:FloatingPointLiteral
	{
	string str = l->getText();
	const char* p = str.c_str();
	int length = strlen(p);
	char* pszTmp = new char[length+2];
	memset(pszTmp, 0, length+2);
	if (isMinus)
		strcpy(pszTmp, "-");
	strcat(pszTmp, p);
	char suffix = pszTmp[strlen(pszTmp)-1];
	if ((suffix == 'd') || (suffix == 'D'))
	{
	    ret.type = CcatTypeValue::CTYPE_DOUBLE;
		double dTmp = atof(p);
		char szTmp[128];
		sprintf(szTmp, "%f", dTmp);
		if (strstr(szTmp, "INF") != NULL)
		{
			delete[] pszTmp;
		    throw CcatFormatException("double literal is out of range.", l->getLine(), l->getColumn());
		}
		if (!onlyCompile)
		{
		    ret.doubleValue = dTmp;
		}
	}
	else
	{
	    ret.type = CcatTypeValue::CTYPE_FLOAT;
		float dTmp = atof(p);
		char szTmp[128];
		sprintf(szTmp, "%f", dTmp);
		if (strstr(szTmp, "INF") != NULL)
		{
			delete[] pszTmp;
		    throw CcatFormatException("float literal is out of range.", l->getLine(), l->getColumn());
		}
		if (!onlyCompile)
		{
		    ret.floatValue = dTmp;
		}
	}
	delete[] pszTmp;
	isMinus = false;
	}
|	
	k:DateLiteral
	{
	ret.type =  CcatTypeValue::CTYPE_DATE_STRING;
	if (!onlyCompile)
	{
		int length = k->getText().length();
		string str = k->getText();
		const char* p = str.c_str();
		char* pszTmp = new char[length];
		strncpy(pszTmp, p+2, length-3);
		pszTmp[length-3] = 0;
		ret.stringValue = pszTmp;
	}
	}
|   
	ret = booleanLiteral[onlyCompile]
|	
	"null"
	{
		ret.type = CcatTypeValue::CTYPE_OBJECT_NULL;
	}
;
exception
catch [ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex] {
throw ex;
}

booleanLiteral[bool onlyCompile] returns [CcatTypeValue ret]
:
	"true" 
	{
	ret.type = CcatTypeValue::CTYPE_BOOL;
	if (!onlyCompile)
	{
		ret.boolValue = true;
	}
	}
|
	"false"
	{
	ret.type = CcatTypeValue::CTYPE_BOOL;
	if (!onlyCompile)
	{
		ret.boolValue = false;
	}
	}
;
exception
catch [ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex] {
throw ex;
}

integerLiteral[bool onlyCompile] returns [CcatTypeValue ret]
:
i:HexLiteral
{
	string str = i->getText();
	const char* p = str.c_str();
	int length = strlen(p);
	char* pszTmp = new char[length+2];
	memset(pszTmp, 0, length+2);
	if (isMinus)
		strcpy(pszTmp, "-");
	strcat(pszTmp, p);

    //remove suffix
	char* q = strchr(pszTmp, 'L');
	if (q == NULL)
		q = strchr(pszTmp, 'l');
    if (q != NULL)
    {
        q[0] = 0;
    }
    length = strlen(pszTmp);
    if (isMinus)
    	length -= 1;
	
	if (length > strlen("0x8000000000000000"))
	{
		delete[] pszTmp;
	    throw CcatFormatException("hex literal shoud be in range of -0x8000000000000000..0x7FFFFFFFFFFFFFFF.", i->getLine(), i->getColumn());
	}
	else if (length == strlen("0x8000000000000000"))
	{
		if (isMinus)
		{
		    if (strcmp(pszTmp+3,"8000000000000000") > 0)
		    {
				delete[] pszTmp;
			    throw CcatFormatException("hex literal shoud be in range of -0x8000000000000000..0x7FFFFFFFFFFFFFFF.", i->getLine(), i->getColumn());
		    }
		}
	    else
	    {
		    if (strcmp(pszTmp+2,"7FFFFFFFFFFFFFFF") > 0)
		    {
				delete[] pszTmp;
			    throw CcatFormatException("hex literal shoud be in range of -0x8000000000000000..0x7FFFFFFFFFFFFFFF.", i->getLine(), i->getColumn());
		    }
	    }
	}
	ret.type = CcatTypeValue::CTYPE_NUMBER;
	if (!onlyCompile)
	{
		ret.numberValue = strtoll(pszTmp, (char**)NULL,16);
	}
	delete[] pszTmp;
	isMinus = false;
}
|
j:OctalLiteral
{
	string str = j->getText();
	const char* p = str.c_str();
	int length = strlen(p);
	char* pszTmp = new char[length+2];
	memset(pszTmp, 0, length+2);
	if (isMinus)
		strcpy(pszTmp, "-");
	strcat(pszTmp, p);

    //remove suffix
	char* q = strchr(pszTmp, 'L');
	if (q == NULL)
		q = strchr(pszTmp, 'l');
    if (q != NULL)
    {
        q[0] = 0;
    }
    length = strlen(pszTmp);
    if (isMinus)
    {
    	length -= 1;
		if (length > strlen("01000000000000000000000"))
		{
			delete[] pszTmp;
		    throw CcatFormatException("octal literal shoud be in range of -01000000000000000000000..0777777777777777777777.", j->getLine(), j->getColumn());
		}
		else if (length == strlen("01000000000000000000000"))
		{
		    if (strcmp(pszTmp+1,"01000000000000000000000") > 0)
		    {
				delete[] pszTmp;
			    throw CcatFormatException("octal literal shoud be in range of -01000000000000000000000..0777777777777777777777.", j->getLine(), j->getColumn());
		    }
		}
    }
    else
    {
		if (length > strlen("0777777777777777777777"))
		{
			delete[] pszTmp;
		    throw CcatFormatException("octal literal shoud be in range of -01000000000000000000000..0777777777777777777777.", j->getLine(), j->getColumn());
		}
		else if (length == strlen("0777777777777777777777"))
		{
		    if (strcmp(pszTmp,"0777777777777777777777") > 0)
		    {
				delete[] pszTmp;
			    throw CcatFormatException("octal literal shoud be in range of -01000000000000000000000..0777777777777777777777.", j->getLine(), j->getColumn());
		    }
		}
    }
    
	ret.type = CcatTypeValue::CTYPE_NUMBER;
	if (!onlyCompile)
	{
		ret.numberValue = strtoll(pszTmp, (char**)NULL,8);
	}
	delete[] pszTmp;
	isMinus = false;
}
|
k:DecimalLiteral
{
	string str = k->getText();
	const char* p = str.c_str();
	int length = strlen(p);
	char* pszTmp = new char[length+2];
	memset(pszTmp, 0, length+2);
	if (isMinus)
		strcpy(pszTmp, "-");
	strcat(pszTmp, p);

    //remove suffix
	char* q = strchr(pszTmp, 'L');
	if (q == NULL)
		q = strchr(pszTmp, 'l');
    if (q != NULL)
    {
        q[0] = 0;
    }
    length = strlen(pszTmp);
    if (isMinus)
    	length -= 1;
	
	if (length > strlen("9223372036854775808"))
	{
		delete[] pszTmp;
	    throw CcatFormatException("decimal literal shoud be in range of -9223372036854775808..9223372036854775807.", k->getLine(), k->getColumn());
	}
	else if (length == strlen("9223372036854775808"))
	{
		if (isMinus)
		{
		    if (strcmp(pszTmp+1,"9223372036854775808") > 0)
		    {
				delete[] pszTmp;
			    throw CcatFormatException("decimal literal shoud be in range of -9223372036854775808..9223372036854775807.", k->getLine(), k->getColumn());
		    }
		}
	    else
	    {
		    if (strcmp(pszTmp,"9223372036854775807") > 0)
		    {
				delete[] pszTmp;
			    throw CcatFormatException("decimal literal shoud be in range of -9223372036854775808..9223372036854775807.", k->getLine(), k->getColumn());
		    }
	    }
	}
	ret.type = CcatTypeValue::CTYPE_NUMBER;
	if (!onlyCompile)
	{
		ret.numberValue = strtoll(pszTmp, (char**)NULL,10);
	}
	delete[] pszTmp;
	isMinus = false;
}
;
exception
catch [ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex] {
throw ex;
}

fieldName[bool onlyCompile] returns [CcatTypeValue ret]
:
(
	i:Identifier
	{
	if (!onlyCompile)
	{
		string str = i->getText();
		const char* p = str.c_str();
		ret.getAttribute(p);
		if (ret.hasError)
		{
			return ret;
		}
	}
	}
|	
	j:ParamName
	{
	if (!onlyCompile)
	{
		string str = j->getText();
		const char* p = str.c_str();
		ret.getParameter(p+1);
		if (ret.hasError)
		{
			return ret;
		}
	}
	}
)
(options {warnWhenFollowAmbig = false;}:
	LSQUARE 
	{CcatTypeValue tmp;}
	tmp = additive_expression[onlyCompile]
	RSQUARE
	{
	if (!onlyCompile)
	{
		if (tmp.hasError)
		{
			ret.setErrorMsg(tmp.errorMsg);
			return ret;
		}
		else
		{
			ret.getArrayElement(tmp);
		}
	}
	}
|   
	DOT 
	m:Identifier
	{
	if (!onlyCompile)
	{
		string str = m->getText();
		const char* p = str.c_str();
		ret.getObjectAttribute(p);
	}
	}
)*
;
exception
catch [ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex] {
throw ex;
}

{
#include "CcatFormatException.hpp"
USE_NS(NS_ANTLRPARSE)
}
class CcatLexer extends Lexer("UnicodeCharScanner");
options{
k=3;
caseSensitive = true;
//exportVocab = STDC;
testLiterals = true;
charVocabulary ='\u0000'..'\uFFFE';
noConstructors = true;
}


{
public:
	bool done;

	CcatLexer( std::istream& in )
	: UnicodeCharScanner(new UnicodeCharBuffer(in),true)
	{
		initLiterals();
	}
	CcatLexer( UnicodeCharBuffer& ib )
	: UnicodeCharScanner(ib,true)
	{
		initLiterals();
	}

	void uponEOF()
	{
		done = true;
	}
}

//TOKEN
ParamName
	:	'#' ('1'..'9')('0'..'9')*
	;

// Operators:
GREATERTHAN 		  : '>' ;
GREATERTHANOREQUALTO  : '>' '=' ;
LESSTHAN  			  : '<' ;
LESSTHANOREQUALTO  	  : '<' '=' ;

EQUAL  				  : '=' '=' ;
NOTEQUAL 			  : '!' '=' ;

NOT 				  : '!' ;
AND 				  : '&' '&';
OR  				  : '|' '|' ;

DIVIDE                : '/' ;
PLUS            	  : '+' ;
MINUS           	  : '-' ;
STAR            	  : '*' ;
MOD             	  : '%' ;

LPAREN  			  : '(' ;
RPAREN  			  : ')' ;
LSQUARE 			  : '[' ;
RSQUARE 			  : ']' ;

COMMA                 : ';';
DOT                   : '.';

// Numeric Constants: 
Number
	:	
		( (Digit)+ ('.' | 'e' | 'E') )=> 
		(Digit)+
		( '.' (Digit)* (Exponent)?
		| Exponent                
		)                         
		(FloatTypeSuffix)?              {_ttype = FloatingPointLiteral;}
	|	
		'0' ('0'..'7')+
		(IntegerTypeSuffix)?            {_ttype = OctalLiteral;}
	|	
		('0' | (('1'..'9') (Digit)*))
		(IntegerTypeSuffix)?            {_ttype = DecimalLiteral;}
	|	
		'0' ('x' | 'X') (HexDigit)+  
		(IntegerTypeSuffix)?            {_ttype = HexLiteral;}
	;

protected
Digit : '0'..'9' ;

protected
HexDigit : (Digit|'a'..'f'|'A'..'F') ;

protected
IntegerTypeSuffix : ('l'|'L') ;

protected
Exponent : ('e'|'E') ('+'|'-')? (Digit)+ ;

protected
FloatTypeSuffix : ('f'|'F'|'d'|'D') ;

CharacterLiteral
    :   '\'' ( EscapeSequence | ~('\''|'\\') ) '\''
    ;

StringLiteral
    :  '"' ( EscapeSequence | ~('\\'|'"') )* '"'
    ;

DateLiteral
    :  'D' '"' (Date (Whitespace Time)? | Time) '"'
    ;

protected
Date
{
char yyyy[5], mm[3], dd[3];
int  year = 0, mon = 0, day = 0;
memset(yyyy, 0, 5);
memset(mm, 0, 3);
memset(dd, 0, 3);
}
:	i1:Digit {yyyy[0] = i1->getText().c_str()[0];}
	i2:Digit {yyyy[1] = i2->getText().c_str()[0];}
	i3:Digit {yyyy[2] = i3->getText().c_str()[0];}
	i4:Digit {yyyy[3] = i4->getText().c_str()[0];}
	DIVIDE
	i5:Digit {mm[0] = i5->getText().c_str()[0];}
	i6:Digit {mm[1] = i6->getText().c_str()[0];}
	DIVIDE 
	i7:Digit {dd[0] = i7->getText().c_str()[0];}
	i8:Digit {dd[1] = i8->getText().c_str()[0];}
{
	mon = atoi(mm);

    if ((mon > 12) || (mon < 1))
    {
        throw CcatFormatException("month should be in range of 01..12.", getLine(), getColumn()-5);
    }

	day = atoi(dd);
    switch(mon)
    {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
	    if ((day > 31) || (day < 1))
	    {
	        throw CcatFormatException("day should be in range of 01..31.", getLine(), getColumn()-2);
	    }
	    break;
    case 2:
		year = atoi(yyyy);
		if ((year % 4) == 0)
		{
		    if ((day > 29) || (day < 1))
		    {
		        throw CcatFormatException("day should be in range of 01..29.", getLine(), getColumn()-2);
		    }
		}
	    else
	    {
		    if ((day > 28) || (day < 1))
		    {
		        throw CcatFormatException("day should be in range of 01..28.", getLine(), getColumn()-2);
		    }
	    }
	    break;
	default:
	    if ((day > 30) || (day < 1))
	    {
	        throw CcatFormatException("day should be in range of 01..30.", getLine(), getColumn()-2);
	    }
	    break;
    }
}
;

protected
Time
{
char hh[3], mm[3], ss[3];
memset(hh, 0, 3);
memset(mm, 0, 3);
memset(ss, 0, 3);
}
: i1:Digit {hh[0] = i1->getText().c_str()[0];}
  i2:Digit {hh[1] = i2->getText().c_str()[0];}
  ':'
  i3:Digit {mm[0] = i3->getText().c_str()[0];}
  i4:Digit {mm[1] = i4->getText().c_str()[0];}
  (':' 
  i5:Digit {ss[0] = i5->getText().c_str()[0];}
  i6:Digit {ss[1] = i6->getText().c_str()[0];}
  )?
{
	if (strcmp(hh, "24") >= 0)
	{
        throw CcatFormatException("hour should be in range of 00..23.", getLine(), getColumn()-8);
	}
	if (strcmp(mm, "60") >= 0)
	{
        throw CcatFormatException("minute should be in range of 00..59.", getLine(), getColumn()-5);
	}
	if (strlen(ss) > 0)
	{
		if (strcmp(ss, "60") >= 0)
		{
	        throw CcatFormatException("second should be in range of 00..59.", getLine(), getColumn()-2);
		}
	}
}
;

protected
EscapeSequence
    :   '\\' ('b'|'t'|'n'|'f'|'r'|'\"'|'\''|'\\')
    |   UnicodeEscape
    |   OctalEscape
    ;

protected
OctalEscape
    :   '\\' ('0'..'3') (('0'..'7') ('0'..'7')?)?
    |   '\\' ('4'..'7') ('0'..'7')?
    ;

protected	
UnicodeEscape
    :   '\\' 'u' HexDigit HexDigit HexDigit HexDigit
    ;

Identifier 
	:	
		('a'..'z'|'A'..'Z'|'_'|'$'| ('\u0080'..'\ufffe'))
		('a'..'z'|'A'..'Z'|'_'|'$'|Digit| ('\u0080'..'\ufffe'))*
	;

Whitespace
	:	
		(' ' | '\t')
		{ _ttype = ANTLR_USE_NAMESPACE(antlr)Token::SKIP; }
	;

