#if !defined(_MONITORING_INSTANCE_H)
#define _MONITORING_INSTANCE_H

#include <vector>
#include <xercesc/dom/DOM.hpp>
#include "Define.h"

using namespace std;
#ifndef XERCESC20
XERCES_CPP_NAMESPACE_USE
#endif

BEGIN_NS(NS_COMMON)
/**
 *�_���v����C���X�^���X�̃N���X���A�N���X���[�h������ƃC���X�^���X�����w�肷��
 */
class MonitoringInstance : public MonitoringClass
{
public:
	MonitoringInstance();
    virtual ~MonitoringInstance(){};
	
	int init(DOMElement *node, string& errMsg);

	bool needRedefine(MonitoringClass &rhs);
	
	int operator==(const MonitoringClass &rhs) const;
	bool isConflict(const MonitoringClass &rhs, bool& isSame);
	bool isValid() {return valid;};

private:
	
	static const char* TAG_INSTANCE;
	static const char* ATTR_MAX_INSTANCE_COUNT;
	static const char* ATTR_VALID;

private:
	bool   valid;
};

END_NS
#endif
