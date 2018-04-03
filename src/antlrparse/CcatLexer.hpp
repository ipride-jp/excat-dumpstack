#ifndef INC_CcatLexer_hpp_
#define INC_CcatLexer_hpp_

#include <antlr/config.hpp>
/* $ANTLR 2.7.7 (20060906): "ccat.g" -> "CcatLexer.hpp"$ */
#include <antlr/CommonToken.hpp>
#include <antlr/InputBuffer.hpp>
#include <antlr/BitSet.hpp>
#include "CcatParserTokenTypes.hpp"

// Include correct superclass header with a header statement for example:
// header "post_include_hpp" {
// #include "UnicodeCharScanner.hpp"
// }
// Or....
// header {
// #include "UnicodeCharScanner.hpp"
// }

#line 1 "ccat.g"

#include <stdlib.h>
#include <stdio.h>
#include "CcatTypeValue.hpp"
#include "CcatFormatException.hpp"
#include "UnicodeCharBuffer.hpp"
#include "UnicodeCharScanner.hpp"
BEGIN_NS(NS_ANTLRPARSE)

#line 31 "CcatLexer.hpp"
class CUSTOM_API CcatLexer : public UnicodeCharScanner, public CcatParserTokenTypes
{
#line 1175 "ccat.g"

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
#line 35 "CcatLexer.hpp"
private:
	void initLiterals();
public:
	bool getCaseSensitiveLiterals() const
	{
		return true;
	}
public:
#if 0
// constructor creation turned of with 'noConstructor' option
	CcatLexer(ANTLR_USE_NAMESPACE(std)istream& in);
	CcatLexer(ANTLR_USE_NAMESPACE(antlr)InputBuffer& ib);
	CcatLexer(const ANTLR_USE_NAMESPACE(antlr)LexerSharedInputState& state);
// constructor creation turned of with 'noConstructor' option
#endif
	ANTLR_USE_NAMESPACE(antlr)RefToken nextToken();
	public: void mParamName(bool _createToken);
	public: void mGREATERTHAN(bool _createToken);
	public: void mGREATERTHANOREQUALTO(bool _createToken);
	public: void mLESSTHAN(bool _createToken);
	public: void mLESSTHANOREQUALTO(bool _createToken);
	public: void mEQUAL(bool _createToken);
	public: void mNOTEQUAL(bool _createToken);
	public: void mNOT(bool _createToken);
	public: void mAND(bool _createToken);
	public: void mOR(bool _createToken);
	public: void mDIVIDE(bool _createToken);
	public: void mPLUS(bool _createToken);
	public: void mMINUS(bool _createToken);
	public: void mSTAR(bool _createToken);
	public: void mMOD(bool _createToken);
	public: void mLPAREN(bool _createToken);
	public: void mRPAREN(bool _createToken);
	public: void mLSQUARE(bool _createToken);
	public: void mRSQUARE(bool _createToken);
	public: void mCOMMA(bool _createToken);
	public: void mDOT(bool _createToken);
	public: void mNumber(bool _createToken);
	protected: void mDigit(bool _createToken);
	protected: void mExponent(bool _createToken);
	protected: void mFloatTypeSuffix(bool _createToken);
	protected: void mIntegerTypeSuffix(bool _createToken);
	protected: void mHexDigit(bool _createToken);
	public: void mCharacterLiteral(bool _createToken);
	protected: void mEscapeSequence(bool _createToken);
	public: void mStringLiteral(bool _createToken);
	public: void mDateLiteral(bool _createToken);
	protected: void mDate(bool _createToken);
	public: void mWhitespace(bool _createToken);
	protected: void mTime(bool _createToken);
	protected: void mUnicodeEscape(bool _createToken);
	protected: void mOctalEscape(bool _createToken);
	public: void mIdentifier(bool _createToken);
private:
	
	static const unsigned long _tokenSet_0_data_[];
	static const ANTLR_USE_NAMESPACE(antlr)BitSet _tokenSet_0;
	static const unsigned long _tokenSet_1_data_[];
	static const ANTLR_USE_NAMESPACE(antlr)BitSet _tokenSet_1;
	static const unsigned long _tokenSet_2_data_[];
	static const ANTLR_USE_NAMESPACE(antlr)BitSet _tokenSet_2;
	static const unsigned long _tokenSet_3_data_[];
	static const ANTLR_USE_NAMESPACE(antlr)BitSet _tokenSet_3;
	static const unsigned long _tokenSet_4_data_[];
	static const ANTLR_USE_NAMESPACE(antlr)BitSet _tokenSet_4;
	static const unsigned long _tokenSet_5_data_[];
	static const ANTLR_USE_NAMESPACE(antlr)BitSet _tokenSet_5;
	static const unsigned long _tokenSet_6_data_[];
	static const ANTLR_USE_NAMESPACE(antlr)BitSet _tokenSet_6;
};
END_NS
#endif /*INC_CcatLexer_hpp_*/
