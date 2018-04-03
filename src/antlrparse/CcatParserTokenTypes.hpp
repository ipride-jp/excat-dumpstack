#ifndef INC_CcatParserTokenTypes_hpp_
#define INC_CcatParserTokenTypes_hpp_

/* $ANTLR 2.7.7 (20060906): "ccat.g" -> "CcatParserTokenTypes.hpp"$ */

#ifndef CUSTOM_API
# define CUSTOM_API
#endif

#ifdef __cplusplus
struct CUSTOM_API CcatParserTokenTypes {
#endif
	enum {
		EOF_ = 1,
		COMMA = 4,
		OR = 5,
		AND = 6,
		NOTEQUAL = 7,
		EQUAL = 8,
		LESSTHAN = 9,
		GREATERTHAN = 10,
		LESSTHANOREQUALTO = 11,
		GREATERTHANOREQUALTO = 12,
		PLUS = 13,
		MINUS = 14,
		STAR = 15,
		DIVIDE = 16,
		MOD = 17,
		NOT = 18,
		LPAREN = 19,
		RPAREN = 20,
		CharacterLiteral = 21,
		StringLiteral = 22,
		FloatingPointLiteral = 23,
		DateLiteral = 24,
		LITERAL_null = 25,
		LITERAL_true = 26,
		LITERAL_false = 27,
		HexLiteral = 28,
		OctalLiteral = 29,
		DecimalLiteral = 30,
		Identifier = 31,
		ParamName = 32,
		LSQUARE = 33,
		RSQUARE = 34,
		DOT = 35,
		Number = 36,
		Digit = 37,
		HexDigit = 38,
		IntegerTypeSuffix = 39,
		Exponent = 40,
		FloatTypeSuffix = 41,
		Date = 42,
		Time = 43,
		EscapeSequence = 44,
		OctalEscape = 45,
		UnicodeEscape = 46,
		Whitespace = 47,
		NULL_TREE_LOOKAHEAD = 3
	};
#ifdef __cplusplus
};
#endif
#endif /*INC_CcatParserTokenTypes_hpp_*/
