//////////////////////////////////////////////////////////////////////////////////////
// InstanceMgr.h
// InstanceMgr���饹�μ���
// ���֥������ȤΥ��󥹥��󥹤γ��ݤȲ�����Ԥ�
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

	// ���ۺѤߤΥΡ��ɤ�����оݤ˲ä���
	void AddInstance( T_Obj *p )
	{
		vNode.push_back( p );
	}

	// ���󥹥��󥹤��������
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

// �Ρ��ɤΥ��󥹥��󥹤��ۤ���
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

