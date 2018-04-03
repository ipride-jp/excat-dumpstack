//////////////////////////////////////////////////////////////////////////////////////
// RegTxtPtr.h
// RegTxtPtr���饹�μ���
//////////////////////////////////////////////////////////////////////////////////////

#if !defined( REGTXTPTR_H_INCLUDED_ )
#define REGTXTPTR_H_INCLUDED_

namespace N_SRegex {

namespace N_SRegTxtPtr {

enum CHARTYPE {
	RTP_NORMAL	=	0x000001,	// �̾��ʸ��
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
	RTP_ZID		=	0x00000A,	// \znnnn ��������
	RTP_NULL	=	0x00000E,	// ��ü
	RTP_ERROR	=	0x00000F,	// ���顼

	RTP_M_LOOP	=	0x000120,	// �롼��ü�α��򼨤�		}
	RTP_M_CLASS	=	0x000220,	// ʸ�����饹ü�α��򼨤�	]
	RTP_M_KAKKO	=	0x000320	// ��̤α��򼨤�			)
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

	// ���ء�
	RegTxtPtr operator ++()
	{
		// ʸ����ʸ������б����ݻ�
		wchar_t c = 0;
		const struct { char c; CHARTYPE t; } gvCharType[] = {
			{ '(', RTP_H_GRP },	// ( ���� (^
			{ '{', RTP_H_MIN },		// { ���� {^
			{ '[', RTP_H_CC },		// [ ���� [^
			{ '|', RTP_OR },
			{ '.', RTP_PIERIOD },
			{ ')', RTP_M_KAKKO },
			{ '}', RTP_M_LOOP },
			{ ']', RTP_M_CLASS },
			{ '\0', RTP_NULL }
		};
		int i;

		if ( T_Char( '\0' ) == (*CurPos) ) {
			// ��ü��ã����
			CurType = RTP_NULL;
			CurChar = T_Char( '\0' );
			return (*this);
		}

		// �����������������󥹤ξ��
		if ( T_Char( '\\' ) == (*CurPos) ) {
			++CurPos;	// ����
			ProcEsc();
			return (*this);
		}

		// ʸ�������
		CurChar = (*CurPos);
		++CurPos;

		// ʸ���μ����Ƚ��
		for ( i = 0; gvCharType[i].c && !( CurChar == T_Char( gvCharType[i].c ) ); i++ );
		if ( !( CurChar == T_Char( gvCharType[i].c ) ) )
			CurType = RTP_NORMAL;	// �̾��ʸ��
		else {
			if ( ( i < 3 ) && T_Char( '^' ) == (*CurPos) ) {
				// (^ �� {^ �� [^ �ξ��
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
				CurType = gvCharType[i].t;	// ��ʸ������ʤ��ü쵭��
		}
		return (*this);
	};

	// ��ʸ������
	CHARTYPE GetCharType() const
	{
		return CurType;
	};
	T_Char operator *() const
	{
		return CurChar;
	};

	// �ݥ��󥿤����
	T_Ptn GetPtr() const
	{
		return CurPos;
	};

	// �������ȤΥ���ǥå��������
	unsigned short GetBackwardIdx() const
	{
		assert( NULL != this && CurType == RTP_ZID );
		return BackwardIndex;
	};
protected:
	// ���������ץ������󥹤θ��ʸ�����������
	void ProcEsc()
	{
		T_Char wc = (*CurPos);
		CurChar = wc;	// �����

		// \�ǽ�λ���Ƥ�����ϥ��顼�Ȥ��롣
		if ( T_Char( '\0' ) == CurChar ) {
			throw SRE_EOF_NOT_ANTICIPATED;
			CurType = RTP_ERROR;
			return ;
		}

		// \��ľ��ΰ�ʸ���ˤ��Ƚ�Ǥ���
		if ( T_Char( 'x' ) == wc ) {
			// ʸ�������ɤλ��ꡣ\x�θ�ˤϺ���4���16�ʿ������ꤵ���
			++CurPos;
			CurChar = ReadHexValue( &CurPos );
			CurType = RTP_NORMAL;
			return ;
		}
		if ( T_Char( 'z' ) == wc ) {
			// �������ȡ�\z�θ�ˤϺ���4���16�ʿ������ꤵ��롣
			++CurPos;
			CurChar = T_Char( '\0' );
			BackwardIndex = ReadHexValue( &CurPos );
			CurType = RTP_ZID;
			return ;
		}

		// �嵭�ʳ���ʸ���ϡ����٤��̾��ʸ���Ȥߤʤ�
		CurType = RTP_NORMAL;
		++CurPos;
	};

	// 16�ʿ���ʸ�����ɤ�
	wchar_t ReadHexValue( T_Ptn *pPtn ) const
	{
		const char *vDigit = "0123456789abcdef";
		wchar_t c = 0;
		for ( int i = 0; i < 4; i++ ) {	// ����4��
			int j;
			// vDigit�˴ޤޤ��ʸ���򸡺�����
			for ( j = 0; vDigit[j] && T_Char( vDigit[j] ) != *(*pPtn); j++ );
			if ( vDigit[j] == '\0' ) break;
			c = ( c << 4 ) + j;
			++(*pPtn);
		}
		return c;
	};

	CHARTYPE CurType;	// ���ߤ�ʸ���μ���
	T_Char CurChar;		// ���ߤ�ʸ��
	T_Ptn CurPos;		// ���ߤΰ���
	unsigned short BackwardIndex;	// �������ȤΥ���ǥå���
};

}	// N_SRegTxtPtr

}; // N_SRegex

#endif // REGTXTPTR_H_INCLUDED_

