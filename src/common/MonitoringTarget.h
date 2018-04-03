#if !defined(_MONITORINGTARGET_H)
#define _MONITORINGTARGET_H

#include <vector>
#include <xercesc/dom/DOM.hpp>
#include "Define.h"

using namespace std;
#ifndef XERCESC20
XERCES_CPP_NAMESPACE_USE
#endif

BEGIN_NS(NS_COMMON)

/**
 * このクラスでは、XMLノードから監視情報を読み込み、保持する。
 *
 * @version 1.0
 * @date 2006/03/23
 * @author guanxh
 */
class ExcludeClass;
class MonitoringTarget
{
public:
	/**
	 * コンストラクタ
	 * 
	 * @param node コンフィッグ・ファイル名
	 */
	MonitoringTarget();
	int init(DOMElement *node);
	virtual ~MonitoringTarget();

	int operator==(const MonitoringTarget &rhs) const;
	int operator<(const MonitoringTarget &rhs) const;
 
    bool shouldDump(const char *throwableClassName, const char *classSig, 
						   const char *methodName, const char *methodSig);
	//add by Qiu Song on 20090821 for 監視タスク中止
    bool getThrowableClassValid(){return bThrowableClassValid;};
	//end of add by Qiu Song on 20090821 for 監視タスク中止

    
	bool isMonitorPackage(const char *classSig);
private:
	char *throwableClass;
    vector<ExcludeClass*> *excludeClassVector;

	//add by Qiu Song on 20090821 for 監視タスク中止
    bool bThrowableClassValid;
	//end of add by Qiu Song on 20090821 for 監視タスク中止

	//add by Qiu Song on 20091009 for 自動監視機能のパッケージ重複指定
	bool bAutoMonitorMode;
	//end of add by Qiu Song on 20091009 for 自動監視機能のパッケージ重複指定
protected:
    vector<char*> *placeVector;
	vector<char*> *methodNameVector;
	vector<char*> *methodSignatureVector;

	static const char *TAG_THROWABLE;
	static const char *TAG_PLACE;
	static const char *TAG_FILTERS;
	static const char *TAG_FILTER;
	static const char *ATTR_THROWABLE_CLASS;
	static const char *ATTR_PLACE_CLASS;
	static const char *ATTR_PLACE_METHOD;
	static const char *ATTR_PLACE_METHOD_SIG;
	static const char *ATTR_EXCLUDE_CLASS;

	//add by Qiu Song on 20090821 for 監視中止タスク
	static const char *ATTR_VALID;
	//end of add by Qiu Song on 20090821 for 監視中止タスク

    //compare string which may be null
	bool  compareStr(char* s,char *ct) const;
	void  getPlaceInfo(DOMNodeList *nodeList);

public:
	char *getThrowableClass() {return throwableClass;};
	void logConfig();
	vector<ExcludeClass*>* getExcludeClassVector(){return excludeClassVector;};
    //does the name exist in the exclude class vector?
    bool excludeClassExist(const char* name);

	//add by Qiu Song on 20091009 for 自動監視のパッケージ重複指定
	bool getAutoMonitorMode(){ return bAutoMonitorMode;};
	void setAutoMonitorMode(bool bAuto){ bAutoMonitorMode = bAuto;};
	//end of add by Qiu Song on 20091009 for 自動監視のパッケージ重複指定

};

inline int MonitoringTarget::operator<(const MonitoringTarget &rhs) const
{
	//compare throwable class
	int result = strcmp(this->throwableClass, rhs.throwableClass);
	return result < 0;

};

inline bool  MonitoringTarget::compareStr(char* s,char *ct) const
{
	if(s == NULL && ct != NULL)
		return false;
	if(s != NULL && ct == NULL)
		return false;
	if(s == NULL && ct ==NULL)
		return true;

	return strcmp(s,ct) == 0;
}

END_NS

#endif  //_MONITORINGTARGET_H
