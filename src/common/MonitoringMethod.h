#if !defined(_MONITORINGMETHOD_H)
#define _MONITORINGMETHOD_H

#pragma warning(disable : 4786) //警告C4786を禁止

#include <map>
#include <string>
#include <vector>
#include <xercesc/dom/DOM.hpp>
#include "Define.h"
#include "OutputSetting.h"
#include "jni.h"
#include "MonitoringClass.h"
#include "MonitoringInstance.h"

using namespace std;

#ifndef XERCESC20
XERCES_CPP_NAMESPACE_USE
#endif

BEGIN_NS(NS_COMMON)

/**
 * このクラスでは、XMLノードから監視情報を読み込み、保持する。
 *
 * @version 1.0
 * @date 2007/08/29
 * @author gancl
 */
class MethodDumpCondition;

class MonitoringMethod : public MonitoringClass
{
public:
	/**
	 * コンストラクタ
	 * 
	 * @param node コンフィッグ・ファイル名
	 */
	MonitoringMethod();
	int init(DOMElement *node, string& errMsg);
	virtual ~MonitoringMethod();

	virtual int operator==(const MonitoringClass &rhs) const;
    virtual bool isConflict(const MonitoringClass &rhs, bool& isSame);
	bool needRedefine(MonitoringClass &rhs);
	void addMonitoringInstance(MonitoringInstance* p);

private:
	char* classSuffix;
    vector<MethodDumpCondition*>*  methodInfo;
    OutputSetting* outputSetting;  //出力ルール 

protected:
	static const char *ATTR_SUFFIX;
	static const char *TAG_METHOD;
	static const char *ATTR_NAME;
	static const char *ATTR_SIGNATURE;
	static const char *ATTR_CONDITION;
	static const char *ATTR_MAX_DUMP_COUNT;
	//add by Qiu Song on 20090821 for 監視中止タスク
	static const char *ATTR_VALID;
	static const char *ATTR_POSITION;
	//end of add by Qiu Song on 20090821 for 監視中止タスク

public:
	void logConfigForMethod();
	void setOutputSetting(OutputSetting* outputSetting){this->outputSetting = outputSetting;}
	char* getClassSuffix(){return classSuffix;}
	vector<MethodDumpCondition*>* getMethodInfo(){return methodInfo;}
	OutputSetting* getOutputSetting(){return outputSetting;}
};

class MethodDumpCondition
{
private:
    char* methodName;
    char* methodSignature;
    char* dumpCondition;
	char* methodSuffix;
    int   maxDumpCount;
	//add by Qiu Song on 20090828 for メソッド監視
	int   dumpPosition;//0-both 1-start 2-end
	//end of add by Qiu Song on 20090828 for メソッド監視
public:
	MethodDumpCondition();
    MethodDumpCondition(char* name, char* signature, char* suffix, char* condition,int maxdumpCount, int position);
    ~MethodDumpCondition();   //dumpCondition

    int operator==(const MethodDumpCondition &rhs) const;
	int operator<(const MethodDumpCondition &rhs) const;
	bool isConflict(const MethodDumpCondition &rhs);
	void logConfig();

    char*  getMethodName(){return methodName;};
    char*  getMethodSignature(){return methodSignature;};
    char*  getMethodSuffix(){return methodSuffix;};
    char*  getDumpCondition(){return dumpCondition;};
    int    getMaxDumpCount(){return maxDumpCount;};
	int    getDumpPosition(){return dumpPosition;};
};

inline int MethodDumpCondition::operator<(const MethodDumpCondition &rhs) const
{
    //compare method name and signature
	int result = strcmp(this->methodName, rhs.methodName);
	if (result == 0)
	{
        result = strcmp(this->methodSignature, rhs.methodSignature);
	}
	return result < 0;
}

class MethodDumpInfo
{
public:
    string dumpCondition;
	string methodSuffix;
    int    maxDumpCount;
    int    dumpCount;
    bool   bValid;
	bool   useParameter;
	bool   useAttribute;

public:
	MethodDumpInfo(MethodDumpCondition*);
    virtual ~MethodDumpInfo(){}; 
};

class MonitoringMethodInfo
{
public:
	MonitoringMethodInfo(MonitoringMethod*);
	virtual ~MonitoringMethodInfo();
	OutputSetting*  getOutputSetting(){return outputSetting;}
	MethodDumpInfo* getMethodDumpInfo(char* methodName, char* methodSig);

    OutputSetting* outputSetting;
	string         classLoadString;
	string         classSuffix;

private:
    map<string, MethodDumpInfo*>*  methodInfo;
};

class MonitoringClassLoadInfo
{
public:
	MonitoringClassLoadInfo();
	virtual ~MonitoringClassLoadInfo();

private:
    //string className;
	//string classLoadString;
	vector<string>*  classUrls;
	vector<jobject>* classLoaders;

public:
	void  addClassUrl(string classUrl);
	void  addClassUrls(vector<string>* classUrls);
	void  addClassLoader(JNIEnv* jni, jobject classLoader);

	vector<jobject>* getClassLoaders(){return classLoaders;};
	vector<string>*  getClassUrls(){return classUrls;};
};

END_NS

#endif  //_MONITORINGMETHOD_H
