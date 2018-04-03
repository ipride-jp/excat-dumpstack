//////////////////////////////////////////////////////////////////////////////////////
// RegTxtPtr.h
// RegTxtPtrクラスの実装
//////////////////////////////////////////////////////////////////////////////////////

#if !defined( REGTXTPTR_H_INCLUDED_ )
#define REGTXTPTR_H_INCLUDED_

namespace N_SRegex {

namespace N_SRegTxtPtr {

enum CHARTYPE {
	RTP_NORMAL	=	0x000001,	// 通常の文字
	RTP_OR		=	0x000002,	// |
	RTP_PIERIOD	=	0x000003,	// .
	RTP_H_GRP	=	0x000014,	// (
	RTP_M_GRP	=	0x000320,	// )
	RTP_H_ZGRP	=	0x000015,	// (^
	RTP_M_ZGRP	=	0x000320,	// )
	RTP_H_MAX	=	0x000016,	// {^
	RTP_M_MAX	=	0x000120,	// }	= RTP_M_LOOP
	RTP_H_MIN	=	0x000017,	// {
	RTP_M_MIN	=	0x000120,	// }	= RTP_M_LOOP
	RTP_H_CC	=	0x000018,	// [
	RTP_M_CC	=	0x000220,	// ]	= RTP_M_CLASS
	RTP_H_UNCC	=	0x000019,	// [^
	RTP_M_UNCC	=	0x000220,	// ]	= RTP_M_CLASS
	RTP_ZID		=	0x00000A,	// \znnnn 前方参照
	RTP_NULL	=	0x00000E,	// 終端
	RTP_ERROR	=	0x00000F,	// エラー

	RTP_M_LOOP	=	0x000120,	// ループ端の右を示す		}
	RTP_M_CLASS	=	0x000220,	// 文字クラス端の右を示す	]
	RTP_M_KAKKO	=	0x000320	// 括弧の右を示す			)
};

template< typename T_Char, typename T_Ptn >
class RegTxtPtr
{
public:
	RegTxtPtr( T_Ptn ptn ) : CurPos( ptn ), BackwardIndex( 0 )
	{
		operator ++();
	};
	RegTxtPtr( const RegTxtPtr &r ) :
		CurPos( r.CurPos ),
		CurChar( r.CurChar ),
		CurType( r.CurType ),
		BackwardIndex( r.BackwardIndex )
		{};

	~RegTxtPtr(){};

	// 次へ。
	RegTxtPtr operator ++()
	{
		// 文字と文字種の対応を保持
		wchar_t c = 0;
		const struct { char c; CHARTYPE t; } gvCharType[] = {
			{ '(', RTP_H_GRP },	// ( 又は (^
			{ '{', RTP_H_MIN },		// { 又は {^
			{ '[', RTP_H_CC },		// [ 又は [^
			{ '|', RTP_OR },
			{ '.', RTP_PIERIOD },
			{ ')', RTP_M_KAKKO },
			{ '}', RTP_M_LOOP },
			{ ']', RTP_M_CLASS },
			{ '\0', RTP_NULL }
		};
		int i;

		if ( T_Char( '\0' ) == (*CurPos) ) {
			// 終端に達した
			CurType = RTP_NULL;
			CurChar = T_Char( '\0' );
			return (*this);
		}

		// エスケースシーケンスの場合
		if ( T_Char( '\\' ) == (*CurPos) ) {
			++CurPos;	// 次へ
			ProcEsc();
			return (*this);
		}

		// 文字を取得
		CurChar = (*CurPos);
		++CurPos;

		// 文字の種類を判断
		for ( i = 0; gvCharType[i].c && !( CurChar == T_Char( gvCharType[i].c ) ); i++ );
		if ( !( CurChar == T_Char( gvCharType[i].c ) ) )
			CurType = RTP_NORMAL;	// 通常の文字
		else {
			if ( ( i < 3 ) && T_Char( '^' ) == (*CurPos) ) {
				// (^ か {^ か [^ の場合
				switch ( gvCharType[i].t ) {
				case RTP_H_GRP:
					CurType = RTP_H_ZGRP;
					break;
				case RTP_H_MIN:
					CurType = RTP_H_MAX;
					break;
				case RTP_H_CC:
					CurType = RTP_H_UNCC;
					break;
				}
				++CurPos;
			}
			else
				CurType = gvCharType[i].t;	// 一文字からなる特殊記号
		}
		return (*this);
	};

	// 一文字取得
	CHARTYPE GetCharType() const
	{
		return CurType;
	};
	T_Char operator *() const
	{
		return CurChar;
	};

	// ポインタを取得
	T_Ptn GetPtr() const
	{
		return CurPos;
	};

	// 前方参照のインデックスを取得
	unsigned short GetBackwardIdx() const
	{
		assert( NULL != this && CurType == RTP_ZID );
		return BackwardIndex;
	};
protected:
	// エスケープシーケンスの後の文字を処理する
	void ProcEsc()
	{
		T_Char wc = (*CurPos);
		CurChar = wc;	// 作業用

		// \で終了している場合はエラーとする。
		if ( T_Char( '\0' ) == CurChar ) {
			throw SRE_EOF_NOT_ANTICIPATED;
			CurType = RTP_ERROR;
			return ;
		}

		// \の直後の一文字により判断する
		if ( T_Char( 'x' ) == wc ) {
			// 文字コードの指定。\xの後には最大4桁の16進数が指定される
			++CurPos;
			CurChar = ReadHexValue( &CurPos );
			CurType = RTP_NORMAL;
			return ;
		}
		if ( T_Char( 'z' ) == wc ) {
			// 前方参照。\zの後には最大4桁の16進数が指定される。
			++CurPos;
			CurChar = T_Char( '\0' );
			BackwardIndex = ReadHexValue( &CurPos );
			CurType = RTP_ZID;
			return ;
		}

		// 上記以外の文字は、すべて通常の文字とみなす
		CurType = RTP_NORMAL;
		++CurPos;
	};

	// 16進数の文字を読む
	wchar_t ReadHexValue( T_Ptn *pPtn ) const
	{
		const char *vDigit = "0123456789abcdef";
		wchar_t c = 0;
		for ( int i = 0; i < 4; i++ ) {	// 最大4桁
			int j;
			// vDigitに含まれる文字を検索する
			for ( j = 0; vDigit[j] && T_Char( vDigit[j] ) != *(*pPtn); j++ );
			if ( vDigit[j] == '\0' ) break;
			c = ( c << 4 ) + j;
			++(*pPtn);
		}
		return c;
	};

	CHARTYPE CurType;	// 現在の文字の種類
	T_Char CurChar;		// 現在の文字
	T_Ptn CurPos;		// 現在の位置
	unsigned short BackwardIndex;	// 前方参照のインデックス
};

}	// N_SRegTxtPtr

}; // N_SRegex

#endif // REGTXTPTR_H_INCLUDED_

