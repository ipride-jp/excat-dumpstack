#ifndef INC_CcatParser_hpp_
#define INC_CcatParser_hpp_

#include <antlr/config.hpp>
/* $ANTLR 2.7.7 (2006-11-01): "ccat.g" -> "CcatParser.hpp"$ */
#include <antlr/TokenStream.hpp>
#include <antlr/TokenBuffer.hpp>
#include "CcatParserTokenTypes.hpp"
#include <antlr/LLkParser.hpp>


#include <stdlib.h>
#include <stdio.h>
#include "CcatTypeValue.hpp"
#include "CcatFormatException.hpp"
#include "UnicodeCharBuffer.hpp"
#include "UnicodeCharScanner.hpp"
BEGIN_NS(NS_ANTLRPARSE)

class CUSTOM_API CcatParser : public ANTLR_USE_NAMESPACE(antlr)LLkParser, public CcatParserTokenTypes
{

private:
	char* convertStringLiteral(char* pStringLiteral, string& errMsg);

public:
    static ParserParam* jvmEnv;
    static bool isMinus;
public:
	void initializeASTFactory( ANTLR_USE_NAMESPACE(antlr)ASTFactory& factory );
protected:
	CcatParser(ANTLR_USE_NAMESPACE(antlr)TokenBuffer& tokenBuf, int k);
public:
	CcatParser(ANTLR_USE_NAMESPACE(antlr)TokenBuffer& tokenBuf);
protected:
	CcatParser(ANTLR_USE_NAMESPACE(antlr)TokenStream& lexer, int k);
public:
	CcatParser(ANTLR_USE_NAMESPACE(antlr)TokenStream& lexer);
	CcatParser(const ANTLR_USE_NAMESPACE(antlr)ParserSharedInputState& state);
	int getNumTokens() const
	{
		return CcatParser::NUM_TOKENS;
	}
	const char* getTokenName( int type ) const
	{
		if( type > getNumTokens() ) return 0;
		return CcatParser::tokenNames[type];
	}
	const char* const* getTokenNames() const
	{
		return CcatParser::tokenNames;
	}
	public: bool  expr(
		char* errMsg, bool isCompile, ParserParam* parserParam
	);
	public: CcatTypeValue  expression(
		bool onlyCompile
	);
	public: CcatTypeValue  logical_and_expression(
		bool onlyCompile
	);
	public: CcatTypeValue  equality_expression(
		bool onlyCompile
	);
	public: CcatTypeValue  relational_expression(
		bool onlyCompile
	);
	public: CcatTypeValue  additive_expression(
		bool onlyCompile
	);
	public: CcatTypeValue  multiplicative_expression(
		bool onlyCompile
	);
	public: CcatTypeValue  unary_expression(
		bool onlyCompile
	);
	public: CcatTypeValue  unary_expression_notplusminus(
		bool onlyCompile
	);
	public: CcatTypeValue  primary_expression(
		bool onlyCompile
	);
	public: CcatTypeValue  literal(
		bool onlyCompile
	);
	public: CcatTypeValue  fieldName(
		bool onlyCompile
	);
	public: CcatTypeValue  integerLiteral(
		bool onlyCompile
	);
	public: CcatTypeValue  booleanLiteral(
		bool onlyCompile
	);
public:
	ANTLR_USE_NAMESPACE(antlr)RefAST getAST()
	{
		return returnAST;
	}
	
protected:
	ANTLR_USE_NAMESPACE(antlr)RefAST returnAST;
private:
	static const char* tokenNames[];
#ifndef NO_STATIC_CONSTS
	static const int NUM_TOKENS = 48;
#else
	enum {
		NUM_TOKENS = 48
	};
#endif
	
};
END_NS
#endif /*INC_CcatParser_hpp_*/
