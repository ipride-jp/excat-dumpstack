/* $ANTLR 2.7.7 (2006-11-01): "ccat.g" -> "CcatParser.cpp"$ */
#include "CcatParser.hpp"
#include <antlr/NoViableAltException.hpp>
#include <antlr/SemanticException.hpp>
#include <antlr/ASTFactory.hpp>

#include "../common/HelperFunc.h"

USE_NS(NS_ANTLRPARSE)
USE_NS(NS_COMMON)

ParserParam* CcatParser::jvmEnv = NULL;
bool         CcatParser::isMinus = false;

extern __int64 strtoll(const char *p,char **pend,int base);

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


CcatParser::CcatParser(ANTLR_USE_NAMESPACE(antlr)TokenBuffer& tokenBuf, int k)
: ANTLR_USE_NAMESPACE(antlr)LLkParser(tokenBuf,k)
{
}

CcatParser::CcatParser(ANTLR_USE_NAMESPACE(antlr)TokenBuffer& tokenBuf)
: ANTLR_USE_NAMESPACE(antlr)LLkParser(tokenBuf,1)
{
}

CcatParser::CcatParser(ANTLR_USE_NAMESPACE(antlr)TokenStream& lexer, int k)
: ANTLR_USE_NAMESPACE(antlr)LLkParser(lexer,k)
{
}

CcatParser::CcatParser(ANTLR_USE_NAMESPACE(antlr)TokenStream& lexer)
: ANTLR_USE_NAMESPACE(antlr)LLkParser(lexer,1)
{
}

CcatParser::CcatParser(const ANTLR_USE_NAMESPACE(antlr)ParserSharedInputState& state)
: ANTLR_USE_NAMESPACE(antlr)LLkParser(state,1)
{
}

bool  CcatParser::expr(
	char* errMsg, bool isCompile, ParserParam* parserParam
) {
	bool ret = false;
	
	jvmEnv = parserParam;
	CcatTypeValue tmp;
	isMinus = false;
	
	
	try {      // for error handling
		{
		switch ( LA(1)) {
		case PLUS:
		case MINUS:
		case NOT:
		case LPAREN:
		case CharacterLiteral:
		case StringLiteral:
		case FloatingPointLiteral:
		case DateLiteral:
		case LITERAL_null:
		case LITERAL_true:
		case LITERAL_false:
		case HexLiteral:
		case OctalLiteral:
		case DecimalLiteral:
		case Identifier:
		case ParamName:
		{
			tmp=expression(isCompile);
			
			if (tmp.hasError)
			{
			strcpy(errMsg, tmp.errorMsg);
			ret = false;
			return ret;
			}
			
			break;
		}
		case COMMA:
		{
			break;
		}
		default:
		{
			throw ANTLR_USE_NAMESPACE(antlr)NoViableAltException(LT(1), getFilename());
		}
		}
		}
		match(COMMA);
		
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
	catch (ANTLR_USE_NAMESPACE(antlr)ANTLRException & ex1) {
		
		// catch all exceptions and report it
		//reportError(ex.toString());
		strcpy(errMsg, ex1.toString().c_str());
		
	}
	catch (CcatFormatException & ex2) {
		
		strcpy(errMsg, ex2.getErrorMsg());
		
	}
	return ret;
}

CcatTypeValue  CcatParser::expression(
	bool onlyCompile
) {
	CcatTypeValue ret;
	ANTLR_USE_NAMESPACE(antlr)RefToken  m = ANTLR_USE_NAMESPACE(antlr)nullToken;
	bool bTmp = onlyCompile;
	
	try {      // for error handling
		ret=logical_and_expression(onlyCompile);
		
		if (!onlyCompile)
		{
			if (ret.hasError)
			{
			    return ret;
			}
		}
		
		{ // ( ... )*
		for (;;) {
			if ((LA(1) == OR)) {
				m = LT(1);
				match(OR);
				CcatTypeValue tmp;
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
				
				tmp=logical_and_expression(bTmp);
				
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
			else {
				goto _loop5;
			}
			
		}
		_loop5:;
		} // ( ... )*
	}
	catch (ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex) {
		
		throw ex;
		
	}
	return ret;
}

CcatTypeValue  CcatParser::logical_and_expression(
	bool onlyCompile
) {
	CcatTypeValue ret;
	ANTLR_USE_NAMESPACE(antlr)RefToken  m = ANTLR_USE_NAMESPACE(antlr)nullToken;
	bool bTmp = onlyCompile;
	
	try {      // for error handling
		ret=equality_expression(onlyCompile);
		
		if (!onlyCompile)
		{
			if (ret.hasError)
			{
			    return ret;
			}
		}
		
		{ // ( ... )*
		for (;;) {
			if ((LA(1) == AND)) {
				m = LT(1);
				match(AND);
				CcatTypeValue tmp;
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
				
				tmp=equality_expression(bTmp);
				
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
			else {
				goto _loop8;
			}
			
		}
		_loop8:;
		} // ( ... )*
	}
	catch (ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex) {
		
		throw ex;
		
	}
	return ret;
}

CcatTypeValue  CcatParser::equality_expression(
	bool onlyCompile
) {
	CcatTypeValue ret;
	ANTLR_USE_NAMESPACE(antlr)RefToken  m = ANTLR_USE_NAMESPACE(antlr)nullToken;
	ANTLR_USE_NAMESPACE(antlr)RefToken  n = ANTLR_USE_NAMESPACE(antlr)nullToken;
	bool isEqual = false;
	
	try {      // for error handling
		ret=relational_expression(onlyCompile);
		
		if (!onlyCompile)
		{
			if (ret.hasError)
			{
				return ret;
			}
		}
		
		{ // ( ... )*
		for (;;) {
			if ((LA(1) == NOTEQUAL || LA(1) == EQUAL)) {
				{
				switch ( LA(1)) {
				case NOTEQUAL:
				{
					m = LT(1);
					match(NOTEQUAL);
					isEqual = false;
					break;
				}
				case EQUAL:
				{
					n = LT(1);
					match(EQUAL);
					isEqual = true;
					break;
				}
				default:
				{
					throw ANTLR_USE_NAMESPACE(antlr)NoViableAltException(LT(1), getFilename());
				}
				}
				}
				CcatTypeValue tmp;
				tmp=relational_expression(onlyCompile);
				
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
			else {
				goto _loop12;
			}
			
		}
		_loop12:;
		} // ( ... )*
	}
	catch (ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex) {
		
		throw ex;
		
	}
	return ret;
}

CcatTypeValue  CcatParser::relational_expression(
	bool onlyCompile
) {
	CcatTypeValue ret;
	ANTLR_USE_NAMESPACE(antlr)RefToken  m = ANTLR_USE_NAMESPACE(antlr)nullToken;
	ANTLR_USE_NAMESPACE(antlr)RefToken  n = ANTLR_USE_NAMESPACE(antlr)nullToken;
	ANTLR_USE_NAMESPACE(antlr)RefToken  k = ANTLR_USE_NAMESPACE(antlr)nullToken;
	ANTLR_USE_NAMESPACE(antlr)RefToken  g = ANTLR_USE_NAMESPACE(antlr)nullToken;
	short op_kind = 0;
	
	try {      // for error handling
		ret=additive_expression(onlyCompile);
		
		if (!onlyCompile)
		{
			if (ret.hasError)
			{
				return ret;
			}
		}
		
		{ // ( ... )*
		for (;;) {
			if (((LA(1) >= LESSTHAN && LA(1) <= GREATERTHANOREQUALTO))) {
				{
				switch ( LA(1)) {
				case LESSTHAN:
				{
					m = LT(1);
					match(LESSTHAN);
					op_kind = 1;
					break;
				}
				case GREATERTHAN:
				{
					n = LT(1);
					match(GREATERTHAN);
					op_kind = 2;
					break;
				}
				case LESSTHANOREQUALTO:
				{
					k = LT(1);
					match(LESSTHANOREQUALTO);
					op_kind = 3;
					break;
				}
				case GREATERTHANOREQUALTO:
				{
					g = LT(1);
					match(GREATERTHANOREQUALTO);
					op_kind = 4;
					break;
				}
				default:
				{
					throw ANTLR_USE_NAMESPACE(antlr)NoViableAltException(LT(1), getFilename());
				}
				}
				}
				CcatTypeValue tmp;
				tmp=additive_expression(onlyCompile);
				
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
			else {
				goto _loop16;
			}
			
		}
		_loop16:;
		} // ( ... )*
	}
	catch (ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex) {
		
		throw ex;
		
	}
	return ret;
}

CcatTypeValue  CcatParser::additive_expression(
	bool onlyCompile
) {
	CcatTypeValue ret;
	ANTLR_USE_NAMESPACE(antlr)RefToken  m = ANTLR_USE_NAMESPACE(antlr)nullToken;
	ANTLR_USE_NAMESPACE(antlr)RefToken  n = ANTLR_USE_NAMESPACE(antlr)nullToken;
	bool isPlus = false;
	
	try {      // for error handling
		ret=multiplicative_expression(onlyCompile);
		
		if (!onlyCompile)
		{
			if (ret.hasError)
			{
				return ret;
			}
		}
		
		{ // ( ... )*
		for (;;) {
			if ((LA(1) == PLUS || LA(1) == MINUS)) {
				{
				switch ( LA(1)) {
				case PLUS:
				{
					m = LT(1);
					match(PLUS);
					isPlus = true;
					break;
				}
				case MINUS:
				{
					n = LT(1);
					match(MINUS);
					isPlus = false;
					break;
				}
				default:
				{
					throw ANTLR_USE_NAMESPACE(antlr)NoViableAltException(LT(1), getFilename());
				}
				}
				}
				CcatTypeValue tmp;
				tmp=multiplicative_expression(onlyCompile);
				
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
			else {
				goto _loop20;
			}
			
		}
		_loop20:;
		} // ( ... )*
	}
	catch (ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex) {
		
		throw ex;
		
	}
	return ret;
}

CcatTypeValue  CcatParser::multiplicative_expression(
	bool onlyCompile
) {
	CcatTypeValue ret;
	ANTLR_USE_NAMESPACE(antlr)RefToken  l = ANTLR_USE_NAMESPACE(antlr)nullToken;
	ANTLR_USE_NAMESPACE(antlr)RefToken  m = ANTLR_USE_NAMESPACE(antlr)nullToken;
	ANTLR_USE_NAMESPACE(antlr)RefToken  n = ANTLR_USE_NAMESPACE(antlr)nullToken;
	short op_kind = 0;
	
	try {      // for error handling
		ret=unary_expression(onlyCompile);
		
		if (!onlyCompile)
		{
			if (ret.hasError)
			{
				return ret;
			}
		}
		
		{ // ( ... )*
		for (;;) {
			if (((LA(1) >= STAR && LA(1) <= MOD))) {
				{
				switch ( LA(1)) {
				case STAR:
				{
					l = LT(1);
					match(STAR);
					op_kind = 1;
					break;
				}
				case DIVIDE:
				{
					m = LT(1);
					match(DIVIDE);
					op_kind = 2;
					break;
				}
				case MOD:
				{
					n = LT(1);
					match(MOD);
					op_kind = 3;
					break;
				}
				default:
				{
					throw ANTLR_USE_NAMESPACE(antlr)NoViableAltException(LT(1), getFilename());
				}
				}
				}
				CcatTypeValue tmp;
				tmp=unary_expression(onlyCompile);
				
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
			else {
				goto _loop24;
			}
			
		}
		_loop24:;
		} // ( ... )*
	}
	catch (ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex) {
		
		throw ex;
		
	}
	return ret;
}

CcatTypeValue  CcatParser::unary_expression(
	bool onlyCompile
) {
	CcatTypeValue ret;
	ANTLR_USE_NAMESPACE(antlr)RefToken  m = ANTLR_USE_NAMESPACE(antlr)nullToken;
	ANTLR_USE_NAMESPACE(antlr)RefToken  n = ANTLR_USE_NAMESPACE(antlr)nullToken;
	
	try {      // for error handling
		switch ( LA(1)) {
		case MINUS:
		{
			bool beMinus = false;
			m = LT(1);
			match(MINUS);
			
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
				
			ret=unary_expression(onlyCompile);
			
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
				
			break;
		}
		case PLUS:
		{
			n = LT(1);
			match(PLUS);
			ret=unary_expression(onlyCompile);
			
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
				
			break;
		}
		case NOT:
		case LPAREN:
		case CharacterLiteral:
		case StringLiteral:
		case FloatingPointLiteral:
		case DateLiteral:
		case LITERAL_null:
		case LITERAL_true:
		case LITERAL_false:
		case HexLiteral:
		case OctalLiteral:
		case DecimalLiteral:
		case Identifier:
		case ParamName:
		{
			ret=unary_expression_notplusminus(onlyCompile);
			break;
		}
		default:
		{
			throw ANTLR_USE_NAMESPACE(antlr)NoViableAltException(LT(1), getFilename());
		}
		}
	}
	catch (ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex) {
		
		throw ex;
		
	}
	return ret;
}

CcatTypeValue  CcatParser::unary_expression_notplusminus(
	bool onlyCompile
) {
	CcatTypeValue ret;
	ANTLR_USE_NAMESPACE(antlr)RefToken  m = ANTLR_USE_NAMESPACE(antlr)nullToken;
	
	try {      // for error handling
		switch ( LA(1)) {
		case LPAREN:
		case CharacterLiteral:
		case StringLiteral:
		case FloatingPointLiteral:
		case DateLiteral:
		case LITERAL_null:
		case LITERAL_true:
		case LITERAL_false:
		case HexLiteral:
		case OctalLiteral:
		case DecimalLiteral:
		case Identifier:
		case ParamName:
		{
			ret=primary_expression(onlyCompile);
			break;
		}
		case NOT:
		{
			m = LT(1);
			match(NOT);
			ret=unary_expression(onlyCompile);
			
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
				
			break;
		}
		default:
		{
			throw ANTLR_USE_NAMESPACE(antlr)NoViableAltException(LT(1), getFilename());
		}
		}
	}
	catch (ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex) {
		
		throw ex;
		
	}
	return ret;
}

CcatTypeValue  CcatParser::primary_expression(
	bool onlyCompile
) {
	CcatTypeValue ret;
	
	try {      // for error handling
		switch ( LA(1)) {
		case LPAREN:
		{
			match(LPAREN);
			ret=expression(onlyCompile);
			
				if (!onlyCompile)
				{
					if (ret.hasError)
					{
						return ret;
					}
				}
				
			match(RPAREN);
			break;
		}
		case CharacterLiteral:
		case StringLiteral:
		case FloatingPointLiteral:
		case DateLiteral:
		case LITERAL_null:
		case LITERAL_true:
		case LITERAL_false:
		case HexLiteral:
		case OctalLiteral:
		case DecimalLiteral:
		{
			ret=literal(onlyCompile);
			break;
		}
		case Identifier:
		case ParamName:
		{
			ret=fieldName(onlyCompile);
			break;
		}
		default:
		{
			throw ANTLR_USE_NAMESPACE(antlr)NoViableAltException(LT(1), getFilename());
		}
		}
	}
	catch (ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex) {
		
		throw ex;
		
	}
	return ret;
}

CcatTypeValue  CcatParser::literal(
	bool onlyCompile
) {
	CcatTypeValue ret;
	ANTLR_USE_NAMESPACE(antlr)RefToken  i = ANTLR_USE_NAMESPACE(antlr)nullToken;
	ANTLR_USE_NAMESPACE(antlr)RefToken  j = ANTLR_USE_NAMESPACE(antlr)nullToken;
	ANTLR_USE_NAMESPACE(antlr)RefToken  l = ANTLR_USE_NAMESPACE(antlr)nullToken;
	ANTLR_USE_NAMESPACE(antlr)RefToken  k = ANTLR_USE_NAMESPACE(antlr)nullToken;
	
	try {      // for error handling
		switch ( LA(1)) {
		case HexLiteral:
		case OctalLiteral:
		case DecimalLiteral:
		{
			ret=integerLiteral(onlyCompile);
			break;
		}
		case CharacterLiteral:
		{
			i = LT(1);
			match(CharacterLiteral);
			
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
				
			break;
		}
		case StringLiteral:
		{
			j = LT(1);
			match(StringLiteral);
			
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
						const char* p =  str.c_str();
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
				
			break;
		}
		case FloatingPointLiteral:
		{
			l = LT(1);
			match(FloatingPointLiteral);
			
				string str = l->getText();
				const char* p =  str.c_str();
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
				
			break;
		}
		case DateLiteral:
		{
			k = LT(1);
			match(DateLiteral);
			
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
				
			break;
		}
		case LITERAL_true:
		case LITERAL_false:
		{
			ret=booleanLiteral(onlyCompile);
			break;
		}
		case LITERAL_null:
		{
			match(LITERAL_null);
			
					ret.type = CcatTypeValue::CTYPE_OBJECT_NULL;
				
			break;
		}
		default:
		{
			throw ANTLR_USE_NAMESPACE(antlr)NoViableAltException(LT(1), getFilename());
		}
		}
	}
	catch (ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex) {
		
		throw ex;
		
	}
	return ret;
}

CcatTypeValue  CcatParser::fieldName(
	bool onlyCompile
) {
	CcatTypeValue ret;
	ANTLR_USE_NAMESPACE(antlr)RefToken  i = ANTLR_USE_NAMESPACE(antlr)nullToken;
	ANTLR_USE_NAMESPACE(antlr)RefToken  j = ANTLR_USE_NAMESPACE(antlr)nullToken;
	ANTLR_USE_NAMESPACE(antlr)RefToken  m = ANTLR_USE_NAMESPACE(antlr)nullToken;
	
	try {      // for error handling
		{
		switch ( LA(1)) {
		case Identifier:
		{
			i = LT(1);
			match(Identifier);
			
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
				
			break;
		}
		case ParamName:
		{
			j = LT(1);
			match(ParamName);
			
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
				
			break;
		}
		default:
		{
			throw ANTLR_USE_NAMESPACE(antlr)NoViableAltException(LT(1), getFilename());
		}
		}
		}
		{ // ( ... )*
		for (;;) {
			switch ( LA(1)) {
			case LSQUARE:
			{
				match(LSQUARE);
				CcatTypeValue tmp;
				tmp=additive_expression(onlyCompile);
				match(RSQUARE);
				
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
					
				break;
			}
			case DOT:
			{
				match(DOT);
				m = LT(1);
				match(Identifier);
				
					if (!onlyCompile)
					{
						string str = m->getText();
						const char* p = str.c_str();
						ret.getObjectAttribute(p);
					}
					
				break;
			}
			default:
			{
				goto _loop34;
			}
			}
		}
		_loop34:;
		} // ( ... )*
	}
	catch (ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex) {
		
		throw ex;
		
	}
	return ret;
}

CcatTypeValue  CcatParser::integerLiteral(
	bool onlyCompile
) {
	CcatTypeValue ret;
	ANTLR_USE_NAMESPACE(antlr)RefToken  i = ANTLR_USE_NAMESPACE(antlr)nullToken;
	ANTLR_USE_NAMESPACE(antlr)RefToken  j = ANTLR_USE_NAMESPACE(antlr)nullToken;
	ANTLR_USE_NAMESPACE(antlr)RefToken  k = ANTLR_USE_NAMESPACE(antlr)nullToken;
	
	try {      // for error handling
		switch ( LA(1)) {
		case HexLiteral:
		{
			i = LT(1);
			match(HexLiteral);
			
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
			
			break;
		}
		case OctalLiteral:
		{
			j = LT(1);
			match(OctalLiteral);
			
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
			
			break;
		}
		case DecimalLiteral:
		{
			k = LT(1);
			match(DecimalLiteral);
			
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
			
			break;
		}
		default:
		{
			throw ANTLR_USE_NAMESPACE(antlr)NoViableAltException(LT(1), getFilename());
		}
		}
	}
	catch (ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex) {
		
		throw ex;
		
	}
	return ret;
}

CcatTypeValue  CcatParser::booleanLiteral(
	bool onlyCompile
) {
	CcatTypeValue ret;
	
	try {      // for error handling
		switch ( LA(1)) {
		case LITERAL_true:
		{
			match(LITERAL_true);
			
				ret.type = CcatTypeValue::CTYPE_BOOL;
				if (!onlyCompile)
				{
					ret.boolValue = true;
				}
				
			break;
		}
		case LITERAL_false:
		{
			match(LITERAL_false);
			
				ret.type = CcatTypeValue::CTYPE_BOOL;
				if (!onlyCompile)
				{
					ret.boolValue = false;
				}
				
			break;
		}
		default:
		{
			throw ANTLR_USE_NAMESPACE(antlr)NoViableAltException(LT(1), getFilename());
		}
		}
	}
	catch (ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex) {
		
		throw ex;
		
	}
	return ret;
}

void CcatParser::initializeASTFactory( ANTLR_USE_NAMESPACE(antlr)ASTFactory& )
{
}
const char* CcatParser::tokenNames[] = {
	"<0>",
	"EOF",
	"<2>",
	"NULL_TREE_LOOKAHEAD",
	"COMMA",
	"OR",
	"AND",
	"NOTEQUAL",
	"EQUAL",
	"LESSTHAN",
	"GREATERTHAN",
	"LESSTHANOREQUALTO",
	"GREATERTHANOREQUALTO",
	"PLUS",
	"MINUS",
	"STAR",
	"DIVIDE",
	"MOD",
	"NOT",
	"LPAREN",
	"RPAREN",
	"CharacterLiteral",
	"StringLiteral",
	"FloatingPointLiteral",
	"DateLiteral",
	"\"null\"",
	"\"true\"",
	"\"false\"",
	"HexLiteral",
	"OctalLiteral",
	"DecimalLiteral",
	"Identifier",
	"ParamName",
	"LSQUARE",
	"RSQUARE",
	"DOT",
	"Number",
	"Digit",
	"HexDigit",
	"IntegerTypeSuffix",
	"Exponent",
	"FloatTypeSuffix",
	"Date",
	"Time",
	"EscapeSequence",
	"OctalEscape",
	"UnicodeEscape",
	"Whitespace",
	0
};



