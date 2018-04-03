//////////////////////////////////////////////////////////////////////////////////////
// RegTNode.h
// SRegexのNFAで使用する、ノードのクラスを実装する
//////////////////////////////////////////////////////////////////////////////////////

#if !defined( REGTNODE_H_INCLUDED_ )
#define REGTNODE_H_INCLUDED_

#include <vector>

#include "InstanceMgr.h"

namespace N_SRegex {
namespace N_SRegexNode {

//////////////////////////////////////////////////////////////////////////////////////
// 基本的な物

// ε遷移のノード
template < typename T_Char, typename T_Ptn, typename T_Text >
class EpsilonNode
{
protected:
	typedef EpsilonNode< T_Char, T_Ptn, T_Text > T_EPSNODE;

public:
	EpsilonNode(){};

	// 次のノードを追加
	void AddNextNode( EpsilonNode *pNextNode )
	{
		vNextNode.push_back( pNextNode );
	};

	// 次のノードの数を取得
	int GetNextNodeCnt() const
	{
		return vNextNode.size();
	};

	// マッチング
	// マッチしたら真を返し、マッチした範囲の終端を示すポインタをppTermに返す
	virtual bool Match( T_Text pText, T_Text *ppTerm )
	{
		// ε遷移とする。順に試してゆき、真を返すものがあったら終了。
		// 無かったら偽を返す。
		for ( int i = 0; i < GetNextNodeCnt(); i++ ) {
			if ( vNextNode[i]->Match( pText, ppTerm ) )
				return true;
		}
		return false;
	};

private:
	std::vector< EpsilonNode* > vNextNode;	// 次のノードの配列
};

// 文字列とマッチするノード
template < typename T_Char, typename T_Ptn, typename T_Text >
class StringNode : public EpsilonNode< T_Char, T_Ptn, T_Text >
{
public:
	StringNode() : T_EPSNODE(){};

	// マッチング
	virtual bool Match( T_Text pText, T_Text *ppTerm )
	{
		// 保持している文字列とのマッチング
		for ( int i = 0; i < vString.size(); i++ ) {
			if ( !( vString[i] == (*pText) ) ) return false;
			++pText;
		}

		// 先に進める
		return T_EPSNODE::Match( pText, ppTerm );
	};

	// 文字列を設定する
	void PushBackChar( T_Char c )
	{
		vString.push_back( c );
	};

protected:
	std::vector< T_Char > vString;	// 比較対照の文字列
};

// 任意の一文字とマッチするノード
template < typename T_Char, typename T_Ptn, typename T_Text >
class ArbitCharNode : public EpsilonNode< T_Char, T_Ptn, T_Text >
{
public:
	ArbitCharNode() : T_EPSNODE(){};

	// マッチング
	virtual bool Match( T_Text pText, T_Text *ppTerm )
	{
		// pTextが末尾に達している場合はマッチしない。
		if ( T_Char( '\0' ) == (*pText) ) return false;

		++pText;	// 次の文字へ

		// 先に進める
		return T_EPSNODE::Match( pText, ppTerm );
	};
};

// 最終状態を示すノード
template < typename T_Char, typename T_Ptn, typename T_Text >
class TermNode : public EpsilonNode< T_Char, T_Ptn, T_Text >
{
public:
	TermNode() : T_EPSNODE(){};

	// マッチング
	virtual bool Match( T_Text pText, T_Text *ppTerm )
	{
		(*ppTerm) = pText;	// 終端の位置をppTermに設定する。
		return true;	// 最終状態に達したので真を返す
	}
};

// 前方参照の情報を保持するノード
template < typename T_Char, typename T_Ptn, typename T_Text >
class BackwardInfoNode : public EpsilonNode< T_Char, T_Ptn, T_Text >
{
public:
	BackwardInfoNode() : T_EPSNODE(), pRightNode( NULL ){};

	// マッチング
	virtual bool Match( T_Text pText, T_Text *ppTerm )
	{
		MatchPos = pText;
		// 自分が左側ノードであった場合、相手の右側ノードも初期化する。
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
	BackwardInfoNode *pRightNode;	// 右側の括弧のノード
};

// 前方参照の結果を参照するノード
template < typename T_Char, typename T_Ptn, typename T_Text >
class BackwardRefNode : public EpsilonNode< T_Char, T_Ptn, T_Text >
{
	typedef BackwardInfoNode< T_Char, T_Ptn, T_Text > T_BIN;
public:
	BackwardRefNode() : T_EPSNODE(), pBackwardInfo( NULL ){};

	// マッチング
	virtual bool Match( T_Text pText, T_Text *ppTerm )
	{
		T_Text s = pBackwardInfo->GetMatchPos();
		T_Text e = pBackwardInfo->GetRightPos();

		// pTextがsからeの範囲にマッチするか否か
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
	const T_BIN *pBackwardInfo;	// 前方参照の情報を保持するノード
};

//////////////////////////////////////////////////////////////////////////////////////
// 繰り返しを処理するノード

// 繰り返しのノード
template < typename T_Char, typename T_Ptn, typename T_Text >
class LoopNode
{
	typedef EpsilonNode< T_Char, T_Ptn, T_Text > T_EPS;
	typedef InstanceMgr< T_EPS > T_INSMGR;

protected:
	typedef LoopNode< T_Char, T_Ptn, T_Text > T_LOOPNODE;

public:
	LoopNode() : pLoop( NULL ){};

	// ループ対象のノードを設定する。
	bool SetLoopDest( T_EPS *pArgFirst, T_EPS *pArgLast, T_INSMGR *pInsMgr )
	{
		// ダミーの最終状態オブジェクトを構築する
		T_EPS *pDummyTerm = CreateNode< TermNode< T_Char, T_Ptn, T_Text > >( pInsMgr );

		pArgLast->AddNextNode( pDummyTerm );
		pLoop = pArgFirst;
		return true;
	};

protected:
	T_EPS *pLoop;
};

// 怠惰な繰り返し
template < typename T_Char, typename T_Ptn, typename T_Text >
class LazyLoopNode :
	public EpsilonNode< T_Char, T_Ptn, T_Text >,
	public LoopNode< T_Char, T_Ptn, T_Text >
{
public:
	LazyLoopNode() : T_EPSNODE(), T_LOOPNODE(){};

	// マッチング
	virtual bool Match( T_Text pText, T_Text *ppTerm )
	{
		do {
			// 後続のマッチを試みる。
			if ( T_EPSNODE::Match( pText, ppTerm ) )
				return true;	// 後続のマッチが成功したら終了する
		} // ループ対象を一回増やす。失敗したら終了
		while ( pLoop->Match( pText, &pText ) );	
		return false;
	};
};

// 貪欲な繰り返し
template < typename T_Char, typename T_Ptn, typename T_Text >
class GreedyLoopNode :
	public EpsilonNode< T_Char, T_Ptn, T_Text >,
	public LoopNode< T_Char, T_Ptn, T_Text >
{
public:
	GreedyLoopNode() : T_EPSNODE(), T_LOOPNODE(){};

	// マッチング
	virtual bool Match( T_Text pText, T_Text *ppTerm )
	{
		T_Text wTerm;
		bool f = false;

		// 怠惰な繰り返しを進めなくなるまで実行する
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
// 文字クラスを処理するノード

// 指定した文字とマッチするノード
template < typename T_Char, typename T_Ptn, typename T_Text >
class CharClassNode : public StringNode< T_Char, T_Ptn, T_Text >
{
public:
	CharClassNode() : StringNode< T_Char, T_Ptn, T_Text >(){};

	// マッチング
	virtual bool Match( T_Text pText, T_Text *ppTerm )
	{
		// 保持している文字と同じ文字であった場合は、マッチしたと見なす
		for ( int i = 0; i < vString.size(); i++ ) {
			if ( vString[i] == (*pText) ) {
				++pText;	// 次の文字へ
				return T_EPSNODE::Match( pText, ppTerm );
			}
		}
		return false;
	};
};

// 指定した文字以外の文字とマッチするノード
template < typename T_Char, typename T_Ptn, typename T_Text >
class UnCharClassNode : public StringNode< T_Char, T_Ptn, T_Text >
{
public:
	UnCharClassNode() : StringNode< T_Char, T_Ptn, T_Text >(){};

	// マッチング
	virtual bool Match( T_Text pText, T_Text *ppTerm )
	{
		// 保持している文字と異なる文字であった場合は、マッチしたと見なす
		for ( int i = 0; i < vString.size(); i++ ) {
			if ( vString[i] == (*pText) )
				return false;	// 同じ文字があった
		}
		++pText;
		return T_EPSNODE::Match( pText, ppTerm );
	};
};

}	// namespace N_SRegexNode

};	// namespace N_SRegex

#endif // REGTNODE_H_INCLUDED_

