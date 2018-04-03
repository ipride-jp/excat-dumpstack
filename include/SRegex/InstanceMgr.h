//////////////////////////////////////////////////////////////////////////////////////
// InstanceMgr.h
// InstanceMgrクラスの実装
// オブジェクトのインスタンスの確保と解放を行う
//////////////////////////////////////////////////////////////////////////////////////

#if !defined( INSTANCEMGR_H_INCLUDED_ )
#define INSTANCEMGR_H_INCLUDED_

#include <vector>

namespace N_SRegex {

template < typename T_Obj >
class InstanceMgr
{
public:
	InstanceMgr(){};

	~InstanceMgr()
	{
		clear();
	};

	// 構築済みのノードを管理対象に加える
	void AddInstance( T_Obj *p )
	{
		vNode.push_back( p );
	}

	// インスタンスを解放する
	void clear()
	{
		for ( int i = 0; i < vNode.size(); i++ )
			delete vNode[i];
		vNode.clear();
		// vNode.swap( std::vector< T_Obj* >() );
	};

protected:
	std::vector< T_Obj* > vNode;

};	// class InstanceMgr

// ノードのインスタンスを構築する
template< typename T_Node, typename T_InsMgr >
T_Node* CreateNode( T_InsMgr *pMgr )
{
	T_Node* p = new T_Node();
	if ( NULL == p ) throw SRE_OUT_OF_MEMORY;
	pMgr->AddInstance( p );
	return p;
}

}; // namespace N_SRegex

#endif // INSTANCEMGR_H_INCLUDED_

