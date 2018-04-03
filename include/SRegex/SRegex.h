//////////////////////////////////////////////////////////////////////////////////////
// SRegex.h
// SRegexクラスの実装
//////////////////////////////////////////////////////////////////////////////////////

#if !defined( SREGEX_H_INCLUDED_ )
#define SREGEX_H_INCLUDED_

/*
() : 優先順位の変更
(^) : 優先順位の変更（前方参照）
| : or
. : 任意の一文字
[] : 文字クラスの指定
[^] : 文字クラスの指定（否定）
{} : 0回以上の繰り返し（最小）
{^} : 0回以上の繰り返し（最大）
\ : エスケープシーケンス
\xnnnn : 0xnnnnの文字（nは0〜f、最大4文字）
\znnnn ; 後方参照0xnnnn
*/

#include <assert.h>
#include <vector>

#include "RegTxtPtr.h"
#include "RegTNode.h"
#include "InstanceMgr.h"

namespace N_SRegex {

enum SREGEX_ERR {
	SRE_NON					= 0x00000000,	// 正常
	SRE_EOF_NOT_ANTICIPATED	= 0xF0000001,	// 予期しないEOF
	SRE_OUT_OF_MEMORY		= 0xF0000002,	// メモリ不足
	SRE_UNCERTAIN_CHAR		= 0xF0000003,	// 不明な文字
	SRE_ZERO_LENGTH_PTN		= 0xF0000004,	// 長さ0のパターン
	SRE_UNDEFINED_BACKWARD		= 0xF0000005,	// 不正な前方参照
	SRE_UNEXPENTED			= 0xF0000006	// その他
};

template < typename T_Char, typename T_Ptn, typename T_Text >
class SRegex
{
	// パターンで使用する擬似ポインタのデータ型
	typedef N_SRegTxtPtr::RegTxtPtr< T_Char, T_Ptn >	T_PtnPtr;

	// ノードの型を示す
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

	// T_CHARTYPEの宣言
	typedef N_SRegTxtPtr::CHARTYPE	T_CHARTYPE;

public:
	SRegex() :
		pNFA( NULL ),
		LastError( SRE_NON ),
		IsMatched( false )
	{};
	~SRegex(){};

	// 初期化
	bool Initialize( T_Ptn pattern )
	{
		assert( NULL != this );

		T_PtnPtr PatternPtr( pattern );

		LastError = SRE_NON;	// エラー値を初期化
		pErrorPos = pattern;
		pNFA = NULL;
		IsMatched = false;
		vBackwardInfo.clear();
		try {
			// NFAを構築する
			pNFA = CreateNFA( &PatternPtr );
		}
		catch( SREGEX_ERR e ) {
			// エラーの種類とエラー発生位置を取得
			LastError = e;
			pErrorPos = PatternPtr.GetPtr();
		}

		// 失敗した場合は全ノードのインスタンスを解放する。
		if ( NULL == pNFA ) {
			m_InsMgr.clear();
			vBackwardInfo.clear();
			return false;
		}
		return true;
	};

	// マッチする範囲を検索
	bool FindMatchStr( T_Text pText, T_Text *ppSPos, T_Text *ppEPos )
	{
		assert( NULL != this && NULL != pNFA );
		T_Text wEPos;
		IsMatched = false;

		// 先頭から一文字ずつマッチングいてゆく
		while ( !( T_Char( '\0' ) == (*pText) ) ) {
			if ( pNFA->Match( pText, &wEPos ) ) {
				// マッチした
				if ( ppSPos ) (*ppSPos) = pText;
				if ( ppEPos ) (*ppEPos) = wEPos;
				IsMatched = true;
				return true;
			}
			++pText;	// 次の文字から
		}
		return false;	// 最後までマッチしなかった
	};

	// マッチング
	bool Match( T_Text pText, T_Text *ppEPos )
	{
		assert( NULL != this && NULL != pNFA && NULL != ppEPos );
		IsMatched = pNFA->Match( pText, ppEPos );
		return IsMatched;
	};

	// 前方参照の情報を取得
	void GetBackwardInfo( std::vector< std::pair< T_Text, T_Text > > *pVec ) const
	{
		// マッチしていなければ、情報は取得できない
		if ( !IsMatched ) return ;

		for ( int i = 0; i < vBackwardInfo.size(); i++ ) {
			T_Text s = vBackwardInfo[i]->GetMatchPos();
			T_Text e = vBackwardInfo[i]->GetRightPos();
			pVec->push_back( std::pair< T_Text, T_Text >( s, e ) );
		}
	};

	// エラー値を取得
	SREGEX_ERR GetLastError() const
	{
		assert( NULL != this );
		return LastError;
	};

	// エラーが発生した箇所を取得
	T_Ptn GetErrorPos() const
	{
		assert( NULL != this );
		return pErrorPos;
	};

protected:

	// 正規表現のNFAを構築する
	T_NODE* CreateNFA( T_PtnPtr *pPtr )
	{
		assert( NULL != this && NULL != pPtr );

		T_NODE *pFirst = NULL;
		T_NODE *pLast = NULL;

		// NFAの最終状態のノードを構築する
		T_NODE_TERM *pTerm = CreateNode< T_NODE_TERM >( &m_InsMgr );

		// NFAの本体の部分を構築する
		if ( !CreateSubNode( pPtr, N_SRegTxtPtr::RTP_NULL, &pFirst, &pLast ) )
			return NULL;

		// 最終状態のノードを登録する
		pLast->AddNextNode( pTerm );

		return pFirst;
	};

	// 通称：サブノードを構築する
	// pPtn : パターンのポインタのアドレス
	// EndCharType : ここに指定したタイプの文字までのサブノードを構築する。
	// pFirst : 構築したサブノードの内の、先頭のサブノードを返す。
	// pLast : 同じく、末尾のサブノードを返す。
	bool CreateSubNode( T_PtnPtr *pPtn, T_CHARTYPE EndCharType, T_NODE **pFirst, T_NODE **pLast )
	{
		assert( NULL != this && NULL != pPtn && NULL != pFirst && NULL != pLast );

		T_NODE *pWork = NULL;	// 構築中のノードを保持

		// 先頭と末尾のε遷移のノードを構築
		(*pFirst) = CreateNode< T_NODE >( &m_InsMgr );
		(*pLast) = CreateNode< T_NODE >( &m_InsMgr );

		pWork = (*pFirst);
		while ( pPtn->GetCharType() != EndCharType ) {
			T_NODE *wpFirst = NULL;	// 呼び出し先の関数で構築された
			T_NODE *wpLast = NULL;	// 先頭と最後のノードを保持する

			switch ( pPtn->GetCharType() ) {
			case N_SRegTxtPtr::RTP_OR:
				// |が先頭や括弧の直後に存在する。もしくは|が連続している。
				if ( pWork == (*pFirst) ) throw SRE_ZERO_LENGTH_PTN;

				// ORが出現した場合、(*pFirst)から(*pLast)までを複数のリンクで接続する。
				pWork->AddNextNode( (*pLast) );
				pWork = (*pFirst);
				++(*pPtn);	// |の分を一文字進める
				continue;
				break;

			case N_SRegTxtPtr::RTP_NORMAL:
				// 文字列にマッチする
				if ( !CreateSubNode_STRING( pPtn, &wpFirst, &wpLast ) )
					return false;
				break;

			case N_SRegTxtPtr::RTP_PIERIOD:
				// 一文字にマッチするノードを構築
				wpFirst = CreateNode< T_NODE_OC >( &m_InsMgr );
				wpLast = wpFirst;
				++(*pPtn);	// .の分を一文字進める
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

			// 構築したノードを構築中の列に追加する。
			pWork->AddNextNode( wpFirst );
			pWork = wpLast;
		}

		// 括弧内にパターンがない。もしくは|で終わっている。
		if ( pWork == (*pFirst) ) throw SRE_ZERO_LENGTH_PTN;

		// 構築してきたリンクを終端までつなげる。
		pWork->AddNextNode( (*pLast) );
		return true;
	};

	// 通称：括弧内のサブノードを構築。
	bool CreateSubNode_KAKKO( T_PtnPtr *pPtn, T_NODE **pFirst, T_NODE **pLast, T_CHARTYPE EndCharType )
	{
		assert( NULL != this && NULL != pPtn && NULL != pFirst && NULL != pLast );

		++(*pPtn);	// 左括弧の分を一文字進める
		if ( !CreateSubNode( pPtn, EndCharType, pFirst, pLast ) )
			return false;
		++(*pPtn);	// 右括弧の分を一文字進める
		return true;
	};

	// 通称：括弧（前方参照）内のサブノードを構築。
	bool CreateSubNode_ZKAKKO( T_PtnPtr *pPtn, T_NODE **pFirst, T_NODE **pLast )
	{
		assert( NULL != this && NULL != pPtn && NULL != pFirst && NULL != pLast );
		T_NODE *wpFirst;
		T_NODE *wpLast;

		// 前方参照用の情報を保持するためのノードを構築する
		T_NODE_BWINFO *pBI_L = CreateNode< T_NODE_BWINFO >( &m_InsMgr );	// 左側用
		T_NODE_BWINFO *pBI_R = CreateNode< T_NODE_BWINFO >( &m_InsMgr );	// 右側用
		pBI_L->SetRightNode( pBI_R );	// 左側用ノードに右側用ノードを通知
		vBackwardInfo.push_back( pBI_L );	// 左側用ノードを配列に登録

		++(*pPtn);	// 左括弧の分を一文字進める
		if ( !CreateSubNode( pPtn, N_SRegTxtPtr::RTP_M_KAKKO, &wpFirst, &wpLast ) )
			return false;
		++(*pPtn);	// 右括弧の分を一文字進める

		// 括弧内サブノードの先頭ノードの前に、左側用ノードを挿入
		pBI_L->AddNextNode( wpFirst );
		(*pFirst) = pBI_L;

		// 括弧内サブノードの末尾ノードの後ろに、右側用ノードを挿入
		wpLast->AddNextNode( pBI_R );
		(*pLast) = pBI_R;

		return true;
	};

	// 通称：[]内のサブノードを構築
	// 機能：*pFirstと*pLast間のリンク上にはGreedyLoopノードを構築する。
	// 　　　括弧内のサブノードはGreedyLoopに引き渡す。
	bool CreateSubNode_MAX( T_PtnPtr *pPtn, T_NODE **pFirst, T_NODE **pLast )
	{
		assert( NULL != pPtn && NULL != pFirst && NULL != pLast );

		T_NODE *wpFirst = NULL;
		T_NODE *wpLast = NULL;
		T_NODE_GREEDY *pGreedyNode = CreateNode< T_NODE_GREEDY >( &m_InsMgr );

		// 括弧内のノードを構築
		if ( !CreateSubNode_KAKKO( pPtn, &wpFirst, &wpLast, N_SRegTxtPtr::RTP_M_MAX ) )
			return false;

		// ループ対象を設定
		pGreedyNode->SetLoopDest( wpFirst, wpLast, &m_InsMgr );

		// 戻り値を設定
		(*pFirst) = pGreedyNode;
		(*pLast) = pGreedyNode;
		return true;
	};

	// 通称：{}内のサブノードを構築
	bool CreateSubNode_MIN( T_PtnPtr *pPtn, T_NODE **pFirst, T_NODE **pLast )
	{
		assert( NULL != pPtn && NULL != pFirst && NULL != pLast );

		T_NODE *wpFirst = NULL;
		T_NODE *wpLast = NULL;
		T_NODE_LAZY *pLazyNode = CreateNode< T_NODE_LAZY >( &m_InsMgr );

		// 括弧内のノードを構築
		if ( !CreateSubNode_KAKKO( pPtn, &wpFirst, &wpLast, N_SRegTxtPtr::RTP_M_MIN ) )
			return false;

		// ループ対象を設定
		pLazyNode->SetLoopDest( wpFirst, wpLast, &m_InsMgr );

		// 戻り値を設定
		(*pFirst) = pLazyNode;
		(*pLast) = pLazyNode;
		return true;
	};

	// 通称：文字列にマッチするードを構築
	bool CreateSubNode_STRING( T_PtnPtr *pPtn, T_NODE **pFirst, T_NODE **pLast )
	{
		assert( NULL != pPtn && NULL != pFirst && NULL != pLast );

		T_NODE_STR *p = CreateNode< T_NODE_STR >( &m_InsMgr );
		SetStringToNode( pPtn, pFirst, pLast, p );
		(*pFirst) = p;
		(*pLast) = p;
		return true;
	};

	// 通称：文字クラスのノードを構築
	bool CreateSubNode_CC( T_PtnPtr *pPtn, T_NODE **pFirst, T_NODE **pLast )
	{
		assert( NULL != pPtn && NULL != pFirst && NULL != pLast );

		T_NODE_STR *p = CreateNode< T_NODE_CC >( &m_InsMgr );	// ノードを構築
		++(*pPtn);	// {の分をスキップ
		SetStringToNode( pPtn, pFirst, pLast, p );
		++(*pPtn);	// }の分をスキップ
		(*pFirst) = p;
		(*pLast) = p;
		return true;
	};

	// 通称：文字クラス（否定）のノードを構築
	bool CreateSubNode_UNCC( T_PtnPtr *pPtn, T_NODE **pFirst, T_NODE **pLast )
	{
		assert( NULL != pPtn && NULL != pFirst && NULL != pLast );

		T_NODE_STR *p = CreateNode< T_NODE_UNCC >( &m_InsMgr );
		++(*pPtn);	// {^の分をスキップ
		SetStringToNode( pPtn, pFirst, pLast, p );
		++(*pPtn);	// }の分をスキップ
		(*pFirst) = p;
		(*pLast) = p;
		return true;
	};

	// 通称：前方参照の情報を参照するノードを構築
	bool CreateSubNode_ZID( T_PtnPtr *pPtn, T_NODE **pFirst, T_NODE **pLast )
	{
		assert( NULL != pPtn && NULL != pFirst && NULL != pLast );
		unsigned short BWIdx = pPtn->GetBackwardIdx();

		// 参照の範囲をチェック
		if ( vBackwardInfo.size() <= BWIdx )
			throw SRE_UNDEFINED_BACKWARD;

		T_NODE_BWREF *p = CreateNode< T_NODE_BWREF >( &m_InsMgr );	// ノードを構築
		++(*pPtn);	// \znnnnの分をスキップ
		p->SetpBackwardInfoNode( vBackwardInfo[ BWIdx ] );
		(*pFirst) = p;
		(*pLast) = p;
		return true;
	};

	// ノードに指定された範囲の文字列を設定する
	void SetStringToNode( T_PtnPtr *pPtn, T_NODE **pFirst, T_NODE **pLast, T_NODE_STR *pNode )
	{
		assert( NULL != pPtn && NULL != pFirst && NULL != pLast && NULL != pNode );

		// 文字列を設定する
		while ( pPtn->GetCharType() == N_SRegTxtPtr::RTP_NORMAL ) {
			pNode->PushBackChar( (**pPtn) );
			++(*pPtn);
		}
	};

protected:
	InstanceMgr< T_NODE > m_InsMgr;
	T_NODE *pNFA;	// NFA
	std::vector< const T_NODE_BWINFO* > vBackwardInfo;	// 前方参照用のノードの配列
	bool IsMatched;	// マッチしたか否か

	SREGEX_ERR LastError;	// 最後に発生したエラー
	T_Ptn pErrorPos;		// エラーが発生した箇所
};

}; // N_SRegex

#endif // SREGEX_H_INCLUDED_

