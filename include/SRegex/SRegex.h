//////////////////////////////////////////////////////////////////////////////////////
// SRegex.h
// SRegex���饹�μ���
//////////////////////////////////////////////////////////////////////////////////////

#if !defined( SREGEX_H_INCLUDED_ )
#define SREGEX_H_INCLUDED_

/*
() : ͥ���̤��ѹ�
(^) : ͥ���̤��ѹ����������ȡ�
| : or
. : Ǥ�դΰ�ʸ��
[] : ʸ�����饹�λ���
[^] : ʸ�����饹�λ���������
{} : 0��ʾ�η����֤��ʺǾ���
{^} : 0��ʾ�η����֤��ʺ����
\ : ���������ץ�������
\xnnnn : 0xnnnn��ʸ����n��0��f������4ʸ����
\znnnn ; ��������0xnnnn
*/

#include <assert.h>
#include <vector>

#include "RegTxtPtr.h"
#include "RegTNode.h"
#include "InstanceMgr.h"

namespace N_SRegex {

enum SREGEX_ERR {
	SRE_NON					= 0x00000000,	// ����
	SRE_EOF_NOT_ANTICIPATED	= 0xF0000001,	// ͽ�����ʤ�EOF
	SRE_OUT_OF_MEMORY		= 0xF0000002,	// ������­
	SRE_UNCERTAIN_CHAR		= 0xF0000003,	// ������ʸ��
	SRE_ZERO_LENGTH_PTN		= 0xF0000004,	// Ĺ��0�Υѥ�����
	SRE_UNDEFINED_BACKWARD		= 0xF0000005,	// ��������������
	SRE_UNEXPENTED			= 0xF0000006	// ����¾
};

template < typename T_Char, typename T_Ptn, typename T_Text >
class SRegex
{
	// �ѥ�����ǻ��Ѥ��뵼���ݥ��󥿤Υǡ�����
	typedef N_SRegTxtPtr::RegTxtPtr< T_Char, T_Ptn >	T_PtnPtr;

	// �Ρ��ɤη��򼨤�
	typedef N_SRegexNode::EpsilonNode< T_Char, T_PtnPtr, T_Text >		T_NODE;
	typedef N_SRegexNode::StringNode< T_Char, T_PtnPtr, T_Text >		T_NODE_STR;
	typedef N_SRegexNode::ArbitCharNode< T_Char, T_PtnPtr, T_Text >		T_NODE_OC;
	typedef N_SRegexNode::TermNode< T_Char, T_PtnPtr, T_Text >			T_NODE_TERM;
	typedef N_SRegexNode::LazyLoopNode< T_Char, T_PtnPtr, T_Text >		T_NODE_LAZY;
	typedef N_SRegexNode::GreedyLoopNode< T_Char, T_PtnPtr, T_Text >	T_NODE_GREEDY;
	typedef N_SRegexNode::CharClassNode< T_Char, T_PtnPtr, T_Text >		T_NODE_CC;
	typedef N_SRegexNode::UnCharClassNode< T_Char, T_PtnPtr, T_Text >	T_NODE_UNCC;
	typedef N_SRegexNode::BackwardInfoNode< T_Char, T_PtnPtr, T_Text >	T_NODE_BWINFO;
	typedef N_SRegexNode::BackwardRefNode< T_Char, T_PtnPtr, T_Text >	T_NODE_BWREF;

	// T_CHARTYPE�����
	typedef N_SRegTxtPtr::CHARTYPE	T_CHARTYPE;

public:
	SRegex() :
		pNFA( NULL ),
		LastError( SRE_NON ),
		IsMatched( false )
	{};
	~SRegex(){};

	// �����
	bool Initialize( T_Ptn pattern )
	{
		assert( NULL != this );

		T_PtnPtr PatternPtr( pattern );

		LastError = SRE_NON;	// ���顼�ͤ�����
		pErrorPos = pattern;
		pNFA = NULL;
		IsMatched = false;
		vBackwardInfo.clear();
		try {
			// NFA���ۤ���
			pNFA = CreateNFA( &PatternPtr );
		}
		catch( SREGEX_ERR e ) {
			// ���顼�μ���ȥ��顼ȯ�����֤����
			LastError = e;
			pErrorPos = PatternPtr.GetPtr();
		}

		// ���Ԥ����������Ρ��ɤΥ��󥹥��󥹤�������롣
		if ( NULL == pNFA ) {
			m_InsMgr.clear();
			vBackwardInfo.clear();
			return false;
		}
		return true;
	};

	// �ޥå������ϰϤ򸡺�
	bool FindMatchStr( T_Text pText, T_Text *ppSPos, T_Text *ppEPos )
	{
		assert( NULL != this && NULL != pNFA );
		T_Text wEPos;
		IsMatched = false;

		// ��Ƭ�����ʸ�����ĥޥå��󥰤��Ƥ椯
		while ( !( T_Char( '\0' ) == (*pText) ) ) {
			if ( pNFA->Match( pText, &wEPos ) ) {
				// �ޥå�����
				if ( ppSPos ) (*ppSPos) = pText;
				if ( ppEPos ) (*ppEPos) = wEPos;
				IsMatched = true;
				return true;
			}
			++pText;	// ����ʸ������
		}
		return false;	// �Ǹ�ޤǥޥå����ʤ��ä�
	};

	// �ޥå���
	bool Match( T_Text pText, T_Text *ppEPos )
	{
		assert( NULL != this && NULL != pNFA && NULL != ppEPos );
		IsMatched = pNFA->Match( pText, ppEPos );
		return IsMatched;
	};

	// �������Ȥξ�������
	void GetBackwardInfo( std::vector< std::pair< T_Text, T_Text > > *pVec ) const
	{
		// �ޥå����Ƥ��ʤ���С�����ϼ����Ǥ��ʤ�
		if ( !IsMatched ) return ;

		for ( int i = 0; i < vBackwardInfo.size(); i++ ) {
			T_Text s = vBackwardInfo[i]->GetMatchPos();
			T_Text e = vBackwardInfo[i]->GetRightPos();
			pVec->push_back( std::pair< T_Text, T_Text >( s, e ) );
		}
	};

	// ���顼�ͤ����
	SREGEX_ERR GetLastError() const
	{
		assert( NULL != this );
		return LastError;
	};

	// ���顼��ȯ�������ս�����
	T_Ptn GetErrorPos() const
	{
		assert( NULL != this );
		return pErrorPos;
	};

protected:

	// ����ɽ����NFA���ۤ���
	T_NODE* CreateNFA( T_PtnPtr *pPtr )
	{
		assert( NULL != this && NULL != pPtr );

		T_NODE *pFirst = NULL;
		T_NODE *pLast = NULL;

		// NFA�κǽ����֤ΥΡ��ɤ��ۤ���
		T_NODE_TERM *pTerm = CreateNode< T_NODE_TERM >( &m_InsMgr );

		// NFA�����Τ���ʬ���ۤ���
		if ( !CreateSubNode( pPtr, N_SRegTxtPtr::RTP_NULL, &pFirst, &pLast ) )
			return NULL;

		// �ǽ����֤ΥΡ��ɤ���Ͽ����
		pLast->AddNextNode( pTerm );

		return pFirst;
	};

	// �̾Ρ����֥Ρ��ɤ��ۤ���
	// pPtn : �ѥ�����Υݥ��󥿤Υ��ɥ쥹
	// EndCharType : �����˻��ꤷ�������פ�ʸ���ޤǤΥ��֥Ρ��ɤ��ۤ��롣
	// pFirst : ���ۤ������֥Ρ��ɤ���Ρ���Ƭ�Υ��֥Ρ��ɤ��֤���
	// pLast : Ʊ�����������Υ��֥Ρ��ɤ��֤���
	bool CreateSubNode( T_PtnPtr *pPtn, T_CHARTYPE EndCharType, T_NODE **pFirst, T_NODE **pLast )
	{
		assert( NULL != this && NULL != pPtn && NULL != pFirst && NULL != pLast );

		T_NODE *pWork = NULL;	// ������ΥΡ��ɤ��ݻ�

		// ��Ƭ�������Φ����ܤΥΡ��ɤ���
		(*pFirst) = CreateNode< T_NODE >( &m_InsMgr );
		(*pLast) = CreateNode< T_NODE >( &m_InsMgr );

		pWork = (*pFirst);
		while ( pPtn->GetCharType() != EndCharType ) {
			T_NODE *wpFirst = NULL;	// �ƤӽФ���δؿ��ǹ��ۤ��줿
			T_NODE *wpLast = NULL;	// ��Ƭ�ȺǸ�ΥΡ��ɤ��ݻ�����

			switch ( pPtn->GetCharType() ) {
			case N_SRegTxtPtr::RTP_OR:
				// |����Ƭ���̤�ľ���¸�ߤ��롣�⤷����|��Ϣ³���Ƥ��롣
				if ( pWork == (*pFirst) ) throw SRE_ZERO_LENGTH_PTN;

				// OR���и�������硢(*pFirst)����(*pLast)�ޤǤ�ʣ���Υ�󥯤���³���롣
				pWork->AddNextNode( (*pLast) );
				pWork = (*pFirst);
				++(*pPtn);	// |��ʬ���ʸ���ʤ��
				continue;
				break;

			case N_SRegTxtPtr::RTP_NORMAL:
				// ʸ����˥ޥå�����
				if ( !CreateSubNode_STRING( pPtn, &wpFirst, &wpLast ) )
					return false;
				break;

			case N_SRegTxtPtr::RTP_PIERIOD:
				// ��ʸ���˥ޥå�����Ρ��ɤ���
				wpFirst = CreateNode< T_NODE_OC >( &m_InsMgr );
				wpLast = wpFirst;
				++(*pPtn);	// .��ʬ���ʸ���ʤ��
				break;

			case N_SRegTxtPtr::RTP_H_GRP:
				if ( !CreateSubNode_KAKKO( pPtn, &wpFirst, &wpLast, N_SRegTxtPtr::RTP_M_KAKKO ) )
					return false;
				break;

			case N_SRegTxtPtr::RTP_H_ZGRP:
				if ( !CreateSubNode_ZKAKKO( pPtn, &wpFirst, &wpLast ) )
					return false;
				break;

			case N_SRegTxtPtr::RTP_H_MAX:
				if ( !CreateSubNode_MAX( pPtn, &wpFirst, &wpLast ) )
					return false;
				break;

			case N_SRegTxtPtr::RTP_H_MIN:
				if ( !CreateSubNode_MIN( pPtn, &wpFirst, &wpLast ) )
					return false;
				break;

			case N_SRegTxtPtr::RTP_H_CC:
				if ( !CreateSubNode_CC( pPtn, &wpFirst, &wpLast ) )
					return false;
				break;

			case N_SRegTxtPtr::RTP_H_UNCC:
				if ( !CreateSubNode_UNCC( pPtn, &wpFirst, &wpLast ) )
					return false;
				break;

			case N_SRegTxtPtr::RTP_ZID:
				if ( !CreateSubNode_ZID( pPtn, &wpFirst, &wpLast ) )
					return false;
				break;

			default:
				throw SRE_UNCERTAIN_CHAR;
			}
			if ( NULL == wpFirst || NULL == wpLast )
				throw SRE_UNEXPENTED;

			// ���ۤ����Ρ��ɤ����������ɲä��롣
			pWork->AddNextNode( wpFirst );
			pWork = wpLast;
		}

		// �����˥ѥ����󤬤ʤ����⤷����|�ǽ���äƤ��롣
		if ( pWork == (*pFirst) ) throw SRE_ZERO_LENGTH_PTN;

		// ���ۤ��Ƥ�����󥯤�ü�ޤǤĤʤ��롣
		pWork->AddNextNode( (*pLast) );
		return true;
	};

	// �̾Ρ������Υ��֥Ρ��ɤ��ۡ�
	bool CreateSubNode_KAKKO( T_PtnPtr *pPtn, T_NODE **pFirst, T_NODE **pLast, T_CHARTYPE EndCharType )
	{
		assert( NULL != this && NULL != pPtn && NULL != pFirst && NULL != pLast );

		++(*pPtn);	// ����̤�ʬ���ʸ���ʤ��
		if ( !CreateSubNode( pPtn, EndCharType, pFirst, pLast ) )
			return false;
		++(*pPtn);	// ����̤�ʬ���ʸ���ʤ��
		return true;
	};

	// �̾Ρ���̡��������ȡ���Υ��֥Ρ��ɤ��ۡ�
	bool CreateSubNode_ZKAKKO( T_PtnPtr *pPtn, T_NODE **pFirst, T_NODE **pLast )
	{
		assert( NULL != this && NULL != pPtn && NULL != pFirst && NULL != pLast );
		T_NODE *wpFirst;
		T_NODE *wpLast;

		// ���������Ѥξ�����ݻ����뤿��ΥΡ��ɤ��ۤ���
		T_NODE_BWINFO *pBI_L = CreateNode< T_NODE_BWINFO >( &m_InsMgr );	// ��¦��
		T_NODE_BWINFO *pBI_R = CreateNode< T_NODE_BWINFO >( &m_InsMgr );	// ��¦��
		pBI_L->SetRightNode( pBI_R );	// ��¦�ѥΡ��ɤ˱�¦�ѥΡ��ɤ�����
		vBackwardInfo.push_back( pBI_L );	// ��¦�ѥΡ��ɤ��������Ͽ

		++(*pPtn);	// ����̤�ʬ���ʸ���ʤ��
		if ( !CreateSubNode( pPtn, N_SRegTxtPtr::RTP_M_KAKKO, &wpFirst, &wpLast ) )
			return false;
		++(*pPtn);	// ����̤�ʬ���ʸ���ʤ��

		// ����⥵�֥Ρ��ɤ���Ƭ�Ρ��ɤ����ˡ���¦�ѥΡ��ɤ�����
		pBI_L->AddNextNode( wpFirst );
		(*pFirst) = pBI_L;

		// ����⥵�֥Ρ��ɤ������Ρ��ɤθ��ˡ���¦�ѥΡ��ɤ�����
		wpLast->AddNextNode( pBI_R );
		(*pLast) = pBI_R;

		return true;
	};

	// �̾Ρ�[]��Υ��֥Ρ��ɤ���
	// ��ǽ��*pFirst��*pLast�֤Υ�󥯾�ˤ�GreedyLoop�Ρ��ɤ��ۤ��롣
	// �����������Υ��֥Ρ��ɤ�GreedyLoop�˰����Ϥ���
	bool CreateSubNode_MAX( T_PtnPtr *pPtn, T_NODE **pFirst, T_NODE **pLast )
	{
		assert( NULL != pPtn && NULL != pFirst && NULL != pLast );

		T_NODE *wpFirst = NULL;
		T_NODE *wpLast = NULL;
		T_NODE_GREEDY *pGreedyNode = CreateNode< T_NODE_GREEDY >( &m_InsMgr );

		// �����ΥΡ��ɤ���
		if ( !CreateSubNode_KAKKO( pPtn, &wpFirst, &wpLast, N_SRegTxtPtr::RTP_M_MAX ) )
			return false;

		// �롼���оݤ�����
		pGreedyNode->SetLoopDest( wpFirst, wpLast, &m_InsMgr );

		// ����ͤ�����
		(*pFirst) = pGreedyNode;
		(*pLast) = pGreedyNode;
		return true;
	};

	// �̾Ρ�{}��Υ��֥Ρ��ɤ���
	bool CreateSubNode_MIN( T_PtnPtr *pPtn, T_NODE **pFirst, T_NODE **pLast )
	{
		assert( NULL != pPtn && NULL != pFirst && NULL != pLast );

		T_NODE *wpFirst = NULL;
		T_NODE *wpLast = NULL;
		T_NODE_LAZY *pLazyNode = CreateNode< T_NODE_LAZY >( &m_InsMgr );

		// �����ΥΡ��ɤ���
		if ( !CreateSubNode_KAKKO( pPtn, &wpFirst, &wpLast, N_SRegTxtPtr::RTP_M_MIN ) )
			return false;

		// �롼���оݤ�����
		pLazyNode->SetLoopDest( wpFirst, wpLast, &m_InsMgr );

		// ����ͤ�����
		(*pFirst) = pLazyNode;
		(*pLast) = pLazyNode;
		return true;
	};

	// �̾Ρ�ʸ����˥ޥå����롼�ɤ���
	bool CreateSubNode_STRING( T_PtnPtr *pPtn, T_NODE **pFirst, T_NODE **pLast )
	{
		assert( NULL != pPtn && NULL != pFirst && NULL != pLast );

		T_NODE_STR *p = CreateNode< T_NODE_STR >( &m_InsMgr );
		SetStringToNode( pPtn, pFirst, pLast, p );
		(*pFirst) = p;
		(*pLast) = p;
		return true;
	};

	// �̾Ρ�ʸ�����饹�ΥΡ��ɤ���
	bool CreateSubNode_CC( T_PtnPtr *pPtn, T_NODE **pFirst, T_NODE **pLast )
	{
		assert( NULL != pPtn && NULL != pFirst && NULL != pLast );

		T_NODE_STR *p = CreateNode< T_NODE_CC >( &m_InsMgr );	// �Ρ��ɤ���
		++(*pPtn);	// {��ʬ�򥹥��å�
		SetStringToNode( pPtn, pFirst, pLast, p );
		++(*pPtn);	// }��ʬ�򥹥��å�
		(*pFirst) = p;
		(*pLast) = p;
		return true;
	};

	// �̾Ρ�ʸ�����饹������ˤΥΡ��ɤ���
	bool CreateSubNode_UNCC( T_PtnPtr *pPtn, T_NODE **pFirst, T_NODE **pLast )
	{
		assert( NULL != pPtn && NULL != pFirst && NULL != pLast );

		T_NODE_STR *p = CreateNode< T_NODE_UNCC >( &m_InsMgr );
		++(*pPtn);	// {^��ʬ�򥹥��å�
		SetStringToNode( pPtn, pFirst, pLast, p );
		++(*pPtn);	// }��ʬ�򥹥��å�
		(*pFirst) = p;
		(*pLast) = p;
		return true;
	};

	// �̾Ρ��������Ȥξ���򻲾Ȥ���Ρ��ɤ���
	bool CreateSubNode_ZID( T_PtnPtr *pPtn, T_NODE **pFirst, T_NODE **pLast )
	{
		assert( NULL != pPtn && NULL != pFirst && NULL != pLast );
		unsigned short BWIdx = pPtn->GetBackwardIdx();

		// ���Ȥ��ϰϤ�����å�
		if ( vBackwardInfo.size() <= BWIdx )
			throw SRE_UNDEFINED_BACKWARD;

		T_NODE_BWREF *p = CreateNode< T_NODE_BWREF >( &m_InsMgr );	// �Ρ��ɤ���
		++(*pPtn);	// \znnnn��ʬ�򥹥��å�
		p->SetpBackwardInfoNode( vBackwardInfo[ BWIdx ] );
		(*pFirst) = p;
		(*pLast) = p;
		return true;
	};

	// �Ρ��ɤ˻��ꤵ�줿�ϰϤ�ʸ��������ꤹ��
	void SetStringToNode( T_PtnPtr *pPtn, T_NODE **pFirst, T_NODE **pLast, T_NODE_STR *pNode )
	{
		assert( NULL != pPtn && NULL != pFirst && NULL != pLast && NULL != pNode );

		// ʸ��������ꤹ��
		while ( pPtn->GetCharType() == N_SRegTxtPtr::RTP_NORMAL ) {
			pNode->PushBackChar( (**pPtn) );
			++(*pPtn);
		}
	};

protected:
	InstanceMgr< T_NODE > m_InsMgr;
	T_NODE *pNFA;	// NFA
	std::vector< const T_NODE_BWINFO* > vBackwardInfo;	// ���������ѤΥΡ��ɤ�����
	bool IsMatched;	// �ޥå��������ݤ�

	SREGEX_ERR LastError;	// �Ǹ��ȯ���������顼
	T_Ptn pErrorPos;		// ���顼��ȯ�������ս�
};

}; // N_SRegex

#endif // SREGEX_H_INCLUDED_

