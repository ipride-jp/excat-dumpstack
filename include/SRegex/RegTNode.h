//////////////////////////////////////////////////////////////////////////////////////
// RegTNode.h
// SRegex��NFA�ǻ��Ѥ��롢�Ρ��ɤΥ��饹���������
//////////////////////////////////////////////////////////////////////////////////////

#if !defined( REGTNODE_H_INCLUDED_ )
#define REGTNODE_H_INCLUDED_

#include <vector>

#include "InstanceMgr.h"

namespace N_SRegex {
namespace N_SRegexNode {

//////////////////////////////////////////////////////////////////////////////////////
// ����Ū��ʪ

// �����ܤΥΡ���
template < typename T_Char, typename T_Ptn, typename T_Text >
class EpsilonNode
{
protected:
	typedef EpsilonNode< T_Char, T_Ptn, T_Text > T_EPSNODE;

public:
	EpsilonNode(){};

	// ���ΥΡ��ɤ��ɲ�
	void AddNextNode( EpsilonNode *pNextNode )
	{
		vNextNode.push_back( pNextNode );
	};

	// ���ΥΡ��ɤο������
	int GetNextNodeCnt() const
	{
		return vNextNode.size();
	};

	// �ޥå���
	// �ޥå������鿿���֤����ޥå������ϰϤν�ü�򼨤��ݥ��󥿤�ppTerm���֤�
	virtual bool Match( T_Text pText, T_Text *ppTerm )
	{
		// �����ܤȤ��롣��˻�Ƥ椭�������֤���Τ����ä��齪λ��
		// ̵���ä��鵶���֤���
		for ( int i = 0; i < GetNextNodeCnt(); i++ ) {
			if ( vNextNode[i]->Match( pText, ppTerm ) )
				return true;
		}
		return false;
	};

private:
	std::vector< EpsilonNode* > vNextNode;	// ���ΥΡ��ɤ�����
};

// ʸ����ȥޥå�����Ρ���
template < typename T_Char, typename T_Ptn, typename T_Text >
class StringNode : public EpsilonNode< T_Char, T_Ptn, T_Text >
{
public:
	StringNode() : T_EPSNODE(){};

	// �ޥå���
	virtual bool Match( T_Text pText, T_Text *ppTerm )
	{
		// �ݻ����Ƥ���ʸ����ȤΥޥå���
		for ( int i = 0; i < vString.size(); i++ ) {
			if ( !( vString[i] == (*pText) ) ) return false;
			++pText;
		}

		// ��˿ʤ��
		return T_EPSNODE::Match( pText, ppTerm );
	};

	// ʸ��������ꤹ��
	void PushBackChar( T_Char c )
	{
		vString.push_back( c );
	};

protected:
	std::vector< T_Char > vString;	// ����оȤ�ʸ����
};

// Ǥ�դΰ�ʸ���ȥޥå�����Ρ���
template < typename T_Char, typename T_Ptn, typename T_Text >
class ArbitCharNode : public EpsilonNode< T_Char, T_Ptn, T_Text >
{
public:
	ArbitCharNode() : T_EPSNODE(){};

	// �ޥå���
	virtual bool Match( T_Text pText, T_Text *ppTerm )
	{
		// pText��������ã���Ƥ�����ϥޥå����ʤ���
		if ( T_Char( '\0' ) == (*pText) ) return false;

		++pText;	// ����ʸ����

		// ��˿ʤ��
		return T_EPSNODE::Match( pText, ppTerm );
	};
};

// �ǽ����֤򼨤��Ρ���
template < typename T_Char, typename T_Ptn, typename T_Text >
class TermNode : public EpsilonNode< T_Char, T_Ptn, T_Text >
{
public:
	TermNode() : T_EPSNODE(){};

	// �ޥå���
	virtual bool Match( T_Text pText, T_Text *ppTerm )
	{
		(*ppTerm) = pText;	// ��ü�ΰ��֤�ppTerm�����ꤹ�롣
		return true;	// �ǽ����֤�ã�����Τǿ����֤�
	}
};

// �������Ȥξ�����ݻ�����Ρ���
template < typename T_Char, typename T_Ptn, typename T_Text >
class BackwardInfoNode : public EpsilonNode< T_Char, T_Ptn, T_Text >
{
public:
	BackwardInfoNode() : T_EPSNODE(), pRightNode( NULL ){};

	// �ޥå���
	virtual bool Match( T_Text pText, T_Text *ppTerm )
	{
		MatchPos = pText;
		// ��ʬ����¦�Ρ��ɤǤ��ä���硢���α�¦�Ρ��ɤ��������롣
		if ( pRightNode ) pRightNode->MatchPos = pText;
		return T_EPSNODE::Match( pText, ppTerm );
	};

	T_Text GetMatchPos() const {
		return MatchPos;
	};
	T_Text GetRightPos() const {
		assert( NULL != pRightNode );
		return pRightNode->GetMatchPos();
	}

	void SetRightNode( BackwardInfoNode* argp )
	{
		pRightNode = argp;
	}
private:
	T_Text MatchPos;
	BackwardInfoNode *pRightNode;	// ��¦�γ�̤ΥΡ���
};

// �������Ȥη�̤򻲾Ȥ���Ρ���
template < typename T_Char, typename T_Ptn, typename T_Text >
class BackwardRefNode : public EpsilonNode< T_Char, T_Ptn, T_Text >
{
	typedef BackwardInfoNode< T_Char, T_Ptn, T_Text > T_BIN;
public:
	BackwardRefNode() : T_EPSNODE(), pBackwardInfo( NULL ){};

	// �ޥå���
	virtual bool Match( T_Text pText, T_Text *ppTerm )
	{
		T_Text s = pBackwardInfo->GetMatchPos();
		T_Text e = pBackwardInfo->GetRightPos();

		// pText��s����e���ϰϤ˥ޥå����뤫�ݤ�
		while ( !( s == e ) ) {
			if ( !( (*pText) == (*s) ) ) return false;
			++s;
			++pText;
		}
		return T_EPSNODE::Match( pText, ppTerm );
	}

	void SetpBackwardInfoNode( const T_BIN *argp )
	{
		pBackwardInfo = argp;
	}
private:
	const T_BIN *pBackwardInfo;	// �������Ȥξ�����ݻ�����Ρ���
};

//////////////////////////////////////////////////////////////////////////////////////
// �����֤����������Ρ���

// �����֤��ΥΡ���
template < typename T_Char, typename T_Ptn, typename T_Text >
class LoopNode
{
	typedef EpsilonNode< T_Char, T_Ptn, T_Text > T_EPS;
	typedef InstanceMgr< T_EPS > T_INSMGR;

protected:
	typedef LoopNode< T_Char, T_Ptn, T_Text > T_LOOPNODE;

public:
	LoopNode() : pLoop( NULL ){};

	// �롼���оݤΥΡ��ɤ����ꤹ�롣
	bool SetLoopDest( T_EPS *pArgFirst, T_EPS *pArgLast, T_INSMGR *pInsMgr )
	{
		// ���ߡ��κǽ����֥��֥������Ȥ��ۤ���
		T_EPS *pDummyTerm = CreateNode< TermNode< T_Char, T_Ptn, T_Text > >( pInsMgr );

		pArgLast->AddNextNode( pDummyTerm );
		pLoop = pArgFirst;
		return true;
	};

protected:
	T_EPS *pLoop;
};

// ���Ƥʷ����֤�
template < typename T_Char, typename T_Ptn, typename T_Text >
class LazyLoopNode :
	public EpsilonNode< T_Char, T_Ptn, T_Text >,
	public LoopNode< T_Char, T_Ptn, T_Text >
{
public:
	LazyLoopNode() : T_EPSNODE(), T_LOOPNODE(){};

	// �ޥå���
	virtual bool Match( T_Text pText, T_Text *ppTerm )
	{
		do {
			// ��³�Υޥå����ߤ롣
			if ( T_EPSNODE::Match( pText, ppTerm ) )
				return true;	// ��³�Υޥå������������齪λ����
		} // �롼���оݤ������䤹�����Ԥ����齪λ
		while ( pLoop->Match( pText, &pText ) );	
		return false;
	};
};

// ���ߤʷ����֤�
template < typename T_Char, typename T_Ptn, typename T_Text >
class GreedyLoopNode :
	public EpsilonNode< T_Char, T_Ptn, T_Text >,
	public LoopNode< T_Char, T_Ptn, T_Text >
{
public:
	GreedyLoopNode() : T_EPSNODE(), T_LOOPNODE(){};

	// �ޥå���
	virtual bool Match( T_Text pText, T_Text *ppTerm )
	{
		T_Text wTerm;
		bool f = false;

		// ���Ƥʷ����֤���ʤ�ʤ��ʤ�ޤǼ¹Ԥ���
		do {
			if ( T_EPSNODE::Match( pText, &wTerm ) ) {
				(*ppTerm) = wTerm;
				f = true;
			}
		} while ( pLoop->Match( pText, &pText ) );	
		return f;
	};
};

//////////////////////////////////////////////////////////////////////////////////////
// ʸ�����饹���������Ρ���

// ���ꤷ��ʸ���ȥޥå�����Ρ���
template < typename T_Char, typename T_Ptn, typename T_Text >
class CharClassNode : public StringNode< T_Char, T_Ptn, T_Text >
{
public:
	CharClassNode() : StringNode< T_Char, T_Ptn, T_Text >(){};

	// �ޥå���
	virtual bool Match( T_Text pText, T_Text *ppTerm )
	{
		// �ݻ����Ƥ���ʸ����Ʊ��ʸ���Ǥ��ä����ϡ��ޥå������ȸ��ʤ�
		for ( int i = 0; i < vString.size(); i++ ) {
			if ( vString[i] == (*pText) ) {
				++pText;	// ����ʸ����
				return T_EPSNODE::Match( pText, ppTerm );
			}
		}
		return false;
	};
};

// ���ꤷ��ʸ���ʳ���ʸ���ȥޥå�����Ρ���
template < typename T_Char, typename T_Ptn, typename T_Text >
class UnCharClassNode : public StringNode< T_Char, T_Ptn, T_Text >
{
public:
	UnCharClassNode() : StringNode< T_Char, T_Ptn, T_Text >(){};

	// �ޥå���
	virtual bool Match( T_Text pText, T_Text *ppTerm )
	{
		// �ݻ����Ƥ���ʸ���Ȱۤʤ�ʸ���Ǥ��ä����ϡ��ޥå������ȸ��ʤ�
		for ( int i = 0; i < vString.size(); i++ ) {
			if ( vString[i] == (*pText) )
				return false;	// Ʊ��ʸ�������ä�
		}
		++pText;
		return T_EPSNODE::Match( pText, ppTerm );
	};
};

}	// namespace N_SRegexNode

};	// namespace N_SRegex

#endif // REGTNODE_H_INCLUDED_

