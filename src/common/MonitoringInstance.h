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
 *ダンプするインスタンスのクラス名、クラスロード文字列とインスタンス個数を指定する
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
