#if !defined(_CONFIG_H)
#define _CONFIG_H

#pragma warning(disable : 4786) //警告C4786を禁止

#include <vector>
#include <map>
#include <string>
#ifdef XERCESC20
#include <xercesc/dom/DOMErrorHandler.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#endif
#include <iostream>

#include "OutputSetting.h"
#include "ExcludeClass.h"
#include "DumpObject.h"
#include "Define.h"
#include "MonitoringClass.h"
#include "MonitoringInstance.h"
#include "MonitoringMethod.h"

//add by Qiu Song on 20091007 for 自動監視と指定タイプ監視の排他処理
#define MONITOR_MODE_NULL 0
#define MONITOR_MODE_AUTO 1
#define MONITOR_MODE_EXCEPTION 2
//end of add by Qiu Song on 20091007 for 自動監視と指定タイプ監視の排他処理

using namespace std;
#ifndef XERCESC20
XERCES_CPP_NAMESPACE_USE
#endif
BEGIN_NS(NS_COMMON)

/**
 * このクラスでは、コンフィッグファイルから情報を読み込み、保持する。
 *
 * @version 1.0
 * @date 2006/02/23
 * @author guanxh
 */


class Config
{
public:
	/**
	 * コンストラクタ
	 * 
	 */
	Config();

	/**
	 * デストラクタ
	 */
	virtual ~Config();

	/**
	 * クラスのインスタンスの初期化
	 *
	 * @param configFileName コンフィッグ・ファイル名
	 *
	 */
	void init(char *configFileName);


	/**
	 * 監視ターゲットを取得
	 * 
	 * @return 監視ターゲット・ベクタ
	 */
	vector<MonitoringTarget*> *getMonitoringTargets() const {return monitoringTargets;};

	/**
	 * 出力設定を取得
	 *
	 * @return 出力設定
	 */
	OutputSetting *getOutputSetting() const {return outputSetting;};
	void setOutputSetting(char* monitorClassName);
	void setOutputSetting(OutputSetting * setting){outputSetting = setting;};
	bool setStackOverflowErrorOutputSetting();

	bool isMonitoringTarget(const char *throwableClassName, const char *classSig, 
		const char *methodName, const char *methodSig);

	char* getConfigFileName(){return configFileFullPath;};
	int getSleepTime(){return sleepTime;};
	char* getDumpFilePath(){return dumpFilePath;};
	char* getDumpFilePathUtf8(){return dumpFilePathUtf8;};
	char* getDumpFilePrefix(){return dumpFilePrefix;};
	unsigned __int64 getMinDiskSpace(){return minDiskSpace;};
	bool  hasMonitoringClassesChanged(const Config &rhs);
	int operator==(const Config &rhs) const;
	DumpObject* getDumpObject(char* objectClassName);
	bool execeedDumpDataLimit(long lsize);
	void logConfig();
    MonitoringTarget* getMonitorTargetByName(const char *throwableClassName);

	ExcludeClass* getCurrentExcludeClass(){return currentExcludeClass;};
	void setCurrentExcludeClass(ExcludeClass* param){currentExcludeClass = param;};

	bool isRedefineClassName(const char* className);
	vector<MonitoringClass*>* getMonitorClassInfo(const char* className, const char* classUrl);
	vector<MonitoringClass*>* getMonitorMethodDefine(){return monitoringClasses;};
	//vector<MonitoringInstance*>* getDumpInstanceVec(){return dumpInstanceVec;};
	void setMonitorMethodDefine(vector<MonitoringClass*>* methodsDefine){monitoringClasses = methodsDefine;};

    //check if the prefix is valid
	static bool isValidPrefix(const char* prefix);

	bool toMonitorSignal(){return monitorSignal;};
    void setSignalOutputSetting(){outputSetting = signalOutputSetting;};
	int  getSignalDumpKind(){return signalDumpKind;};
    char* getThreadStatus(){return threadStatus;};//add by Qiu Song on 20090819 for 指定メソッドダンプ
	map<string, string>* getMailSetting() {return mailSettingMap;};	
	int getThreadPriority(){return threadPriority;};//add by Qiu Song on 20091022 for ダンプスレッド優先度の指定
	bool isCheckDuplication() {return checkDuplication;}
	bool isDumpDuplicationWhenThreadDiff() {return dumpDuplicationWhenThreadDiff;};
	int getTimeLimit() {return timeLimit;};

	int  getMaxInstanceCount(string& className, string& classUrl);

	bool isMonitorPackageForAllException(const char *classSig);
    bool isExcludeClassForAllException(const char *exceptionClassSig);

	bool isMonitorAllExceptionMode(){return monitorAllException;};

public:
	/* xml解析のエラーメッセージを保存する*/
    static string errorMsg;

private:
	
    char* configFileFullPath;//path is included
    int sleepTime;  //the sleep time for the thread to monitor the config file
	/**監視ターゲット・ポインタの集まり*/
	vector<MonitoringTarget*> *monitoringTargets;

	/**監視メソッドの集まり*/
	//vector<MonitoringMethod*> *monitoringMethods;
	vector<MonitoringClass*> *monitoringClasses;

    /**dump object templates*/
	vector<DumpObject*> *dumpObjectTemplates;

	/** the map of dump class name and output setting*/
    map<string,OutputSetting*> *outputSettingMap;
    vector<OutputSetting*> *outputSettingList;

	/** the map of dump class name and dumpObject*/
    map<string, DumpObject*> *dumpObjectMap;

	/**出力設定*/
	OutputSetting *outputSetting;
    ExcludeClass* currentExcludeClass;

	/** attribute of dumpfile*/
	char* dumpFilePath;
	char* dumpFilePathUtf8;
	char* dumpFilePrefix;
    long  maxDumpData;
	unsigned __int64 minDiskSpace;
	
	/**attribute of Log*/
	char* logPath;
	char* logPathUtf8;
	int logLevel;

	/**attribute of mail*/
    map<string,string> *mailSettingMap;

	/**monitor signal*/
    bool  monitorSignal;
    short signalDumpKind;
	char* threadStatus;//add by Qiu Song on 20090819 for スレッド状態
	int   threadPriority;//add by Qiu Song on 20091022 for スレッド優先度

	OutputSetting* signalOutputSetting;

	/**check duplication*/
	bool checkDuplication;
	bool dumpDuplicationWhenThreadDiff;
	int timeLimit;

	/** すべてのException（Throwable）を監視するかどか*/
	bool monitorAllException;

	/**XMLスキーマにの監視ターゲットのタグ名*/
	static const char *TAG_MONITORING_TARGET;
	
	/**XMLスキーマにの監視メソッドのタグ名*/
	static const char *TAG_MONITORING_METHOD;

	/**XMLスキーマにの出力設定のタグ名*/
	static const char *TAG_OUTPUT_SETTING;
	
	/** XMLスキーマのSleepタグ名*/
	static const char *TAG_SLEEP_SETTING;

	static const char *TAG_OBJECTELEMENT;
    static const char *TAG_OBJECT;
	static const char *TAG_OTHERS;
	static const char *TAG_DUMPFILE;
	static const char *TAG_TASK;
	static const char *TAG_LOGFILE;
	static const char *TAG_MONITORSIGNAL;
	static const char *ATTR_DUMP_KIND;
	static const char *ATTR_THREAD_STATUS;//add by Qiu Song on 20090821 for 指定状態のスレッドダウンロード
	//add by Qiu Song on 20090821 for 監視中止タスク
	static const char *ATTR_VALID;
	static const char *ATTR_SUFFIX;
	//end of add by Qiu Song on 20090821 for 監視中止タスク
    static const char *ATTR_THREAD_PRIORITY;//add by Qiu Song on 20091022 for ダンプスレッド優先度の指定
	
	static const char *ATTR_PATH;
	static const char *ATTR_PREFIX;
	static const char *ATTR_MIN_DISK_SPACE;
	static const char *ATTR_SLEEP;
    static const char* TAG_MAXDUMPDATA_SETTING;
	static const char* ATTR_LIMIT;
	static const char* TAG_LOG;
	static const char* ATTR_LOGPATH;
	static const char* ATTR_LOGLEVEL;

	static const char* TAG_DUMP_INSTANCE;
	static const char* TAG_INSTANCE;
	static const char* TAG_MAIL;

	static const char* TAG_CHECK_DUPLICATION;
	static const char* ATTR_DUMP_DUPLICATION_WHEN_THREAD_DIFF;
	static const char* ATTR_TIME_LIMIT;

	//initial xerces-c 
	DOMBuilder* initXerces_c();

    //parse xml file
    DOMDocument* parseXML(DOMBuilder* parser);

	//get monitor class info
	void readMonitorTargets(DOMDocument *doc);

	//get object templates info
	void readObjectTemplates(DOMDocument *doc);

	//get others element info
	void readOthersElement(DOMDocument *doc);

	//get dump instance info
	void readDumpInstance(DOMDocument *doc);
	
	//すべてのExceptionを監視するかどうか
	void chkIfToMonitorAllException();

private:

	/**
	 * コンフィッグ・ファイルを解析する際に発生するエラーを処理する。
	 */
	class DOMConfigErrorHandler : public DOMErrorHandler
	{
	public:

		/**
		 * コンストラクタ
		 */
		DOMConfigErrorHandler();

		/**
		 * デストラクタ
		 */
		~DOMConfigErrorHandler();

		/**
		 * エラーが発生したか否かを判断
		 *
		 * @return エラーが発生したか否か
		 */
		bool getSawErrors() const;

		/**
		 * エラーを処理
		 *
		 * @param domError エラーを表す
		 */
		bool handleError(const DOMError& domError);

		/**
		 * エラー発生を示すフラッグをリセット
		 */
		void resetErrors();

	private :

		/**エラー発生を示すフラッグ*/
		bool fSawErrors;
	};

	
	/**
	 * このクラスでは、XMLChタイプの文字列を受け取り、ローカル形式に変換する。
	 *
	 */
	class StrX
	{
	public :

		/**
		 * コンストラクタ
		 *
		 * @param toTranscode XMLChタイプの文字列
		 */
		StrX(const XMLCh* const toTranscode)
		{
			// Call the private transcoding method
			fLocalForm = XMLString::transcode(toTranscode);
		}

		/**
		 * デストラクタ
		 */
		~StrX()
		{
			XMLString::release(&fLocalForm);
		}

		/**
		 * ローカル形式の文字列を取得
		 *
		 * @return ローカル形式の文字列
		 */
		const char* localForm() const
		{
			return fLocalForm;
		}

	private :
		
		/**ローカル形式の文字列*/
		char*   fLocalForm;
	};
};

inline bool Config::DOMConfigErrorHandler::getSawErrors() const
{
	return fSawErrors;
}
END_NS

#endif  //_CONFIG_H
