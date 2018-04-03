#if !defined(_CONFIG_H)
#define _CONFIG_H

#pragma warning(disable : 4786) //�x��C4786���֎~

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

//add by Qiu Song on 20091007 for �����Ď��Ǝw��^�C�v�Ď��̔r������
#define MONITOR_MODE_NULL 0
#define MONITOR_MODE_AUTO 1
#define MONITOR_MODE_EXCEPTION 2
//end of add by Qiu Song on 20091007 for �����Ď��Ǝw��^�C�v�Ď��̔r������

using namespace std;
#ifndef XERCESC20
XERCES_CPP_NAMESPACE_USE
#endif
BEGIN_NS(NS_COMMON)

/**
 * ���̃N���X�ł́A�R���t�B�b�O�t�@�C���������ǂݍ��݁A�ێ�����B
 *
 * @version 1.0
 * @date 2006/02/23
 * @author guanxh
 */


class Config
{
public:
	/**
	 * �R���X�g���N�^
	 * 
	 */
	Config();

	/**
	 * �f�X�g���N�^
	 */
	virtual ~Config();

	/**
	 * �N���X�̃C���X�^���X�̏�����
	 *
	 * @param configFileName �R���t�B�b�O�E�t�@�C����
	 *
	 */
	void init(char *configFileName);


	/**
	 * �Ď��^�[�Q�b�g���擾
	 * 
	 * @return �Ď��^�[�Q�b�g�E�x�N�^
	 */
	vector<MonitoringTarget*> *getMonitoringTargets() const {return monitoringTargets;};

	/**
	 * �o�͐ݒ���擾
	 *
	 * @return �o�͐ݒ�
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
    char* getThreadStatus(){return threadStatus;};//add by Qiu Song on 20090819 for �w�胁�\�b�h�_���v
	map<string, string>* getMailSetting() {return mailSettingMap;};	
	int getThreadPriority(){return threadPriority;};//add by Qiu Song on 20091022 for �_���v�X���b�h�D��x�̎w��
	bool isCheckDuplication() {return checkDuplication;}
	bool isDumpDuplicationWhenThreadDiff() {return dumpDuplicationWhenThreadDiff;};
	int getTimeLimit() {return timeLimit;};

	int  getMaxInstanceCount(string& className, string& classUrl);

	bool isMonitorPackageForAllException(const char *classSig);
    bool isExcludeClassForAllException(const char *exceptionClassSig);

	bool isMonitorAllExceptionMode(){return monitorAllException;};

public:
	/* xml��͂̃G���[���b�Z�[�W��ۑ�����*/
    static string errorMsg;

private:
	
    char* configFileFullPath;//path is included
    int sleepTime;  //the sleep time for the thread to monitor the config file
	/**�Ď��^�[�Q�b�g�E�|�C���^�̏W�܂�*/
	vector<MonitoringTarget*> *monitoringTargets;

	/**�Ď����\�b�h�̏W�܂�*/
	//vector<MonitoringMethod*> *monitoringMethods;
	vector<MonitoringClass*> *monitoringClasses;

    /**dump object templates*/
	vector<DumpObject*> *dumpObjectTemplates;

	/** the map of dump class name and output setting*/
    map<string,OutputSetting*> *outputSettingMap;
    vector<OutputSetting*> *outputSettingList;

	/** the map of dump class name and dumpObject*/
    map<string, DumpObject*> *dumpObjectMap;

	/**�o�͐ݒ�*/
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
	char* threadStatus;//add by Qiu Song on 20090819 for �X���b�h���
	int   threadPriority;//add by Qiu Song on 20091022 for �X���b�h�D��x

	OutputSetting* signalOutputSetting;

	/**check duplication*/
	bool checkDuplication;
	bool dumpDuplicationWhenThreadDiff;
	int timeLimit;

	/** ���ׂĂ�Exception�iThrowable�j���Ď����邩�ǂ�*/
	bool monitorAllException;

	/**XML�X�L�[�}�ɂ̊Ď��^�[�Q�b�g�̃^�O��*/
	static const char *TAG_MONITORING_TARGET;
	
	/**XML�X�L�[�}�ɂ̊Ď����\�b�h�̃^�O��*/
	static const char *TAG_MONITORING_METHOD;

	/**XML�X�L�[�}�ɂ̏o�͐ݒ�̃^�O��*/
	static const char *TAG_OUTPUT_SETTING;
	
	/** XML�X�L�[�}��Sleep�^�O��*/
	static const char *TAG_SLEEP_SETTING;

	static const char *TAG_OBJECTELEMENT;
    static const char *TAG_OBJECT;
	static const char *TAG_OTHERS;
	static const char *TAG_DUMPFILE;
	static const char *TAG_TASK;
	static const char *TAG_LOGFILE;
	static const char *TAG_MONITORSIGNAL;
	static const char *ATTR_DUMP_KIND;
	static const char *ATTR_THREAD_STATUS;//add by Qiu Song on 20090821 for �w���Ԃ̃X���b�h�_�E�����[�h
	//add by Qiu Song on 20090821 for �Ď����~�^�X�N
	static const char *ATTR_VALID;
	static const char *ATTR_SUFFIX;
	//end of add by Qiu Song on 20090821 for �Ď����~�^�X�N
    static const char *ATTR_THREAD_PRIORITY;//add by Qiu Song on 20091022 for �_���v�X���b�h�D��x�̎w��
	
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
	
	//���ׂĂ�Exception���Ď����邩�ǂ���
	void chkIfToMonitorAllException();

private:

	/**
	 * �R���t�B�b�O�E�t�@�C������͂���ۂɔ�������G���[����������B
	 */
	class DOMConfigErrorHandler : public DOMErrorHandler
	{
	public:

		/**
		 * �R���X�g���N�^
		 */
		DOMConfigErrorHandler();

		/**
		 * �f�X�g���N�^
		 */
		~DOMConfigErrorHandler();

		/**
		 * �G���[�������������ۂ��𔻒f
		 *
		 * @return �G���[�������������ۂ�
		 */
		bool getSawErrors() const;

		/**
		 * �G���[������
		 *
		 * @param domError �G���[��\��
		 */
		bool handleError(const DOMError& domError);

		/**
		 * �G���[�����������t���b�O�����Z�b�g
		 */
		void resetErrors();

	private :

		/**�G���[�����������t���b�O*/
		bool fSawErrors;
	};

	
	/**
	 * ���̃N���X�ł́AXMLCh�^�C�v�̕�������󂯎��A���[�J���`���ɕϊ�����B
	 *
	 */
	class StrX
	{
	public :

		/**
		 * �R���X�g���N�^
		 *
		 * @param toTranscode XMLCh�^�C�v�̕�����
		 */
		StrX(const XMLCh* const toTranscode)
		{
			// Call the private transcoding method
			fLocalForm = XMLString::transcode(toTranscode);
		}

		/**
		 * �f�X�g���N�^
		 */
		~StrX()
		{
			XMLString::release(&fLocalForm);
		}

		/**
		 * ���[�J���`���̕�������擾
		 *
		 * @return ���[�J���`���̕�����
		 */
		const char* localForm() const
		{
			return fLocalForm;
		}

	private :
		
		/**���[�J���`���̕�����*/
		char*   fLocalForm;
	};
};

inline bool Config::DOMConfigErrorHandler::getSawErrors() const
{
	return fSawErrors;
}
END_NS

#endif  //_CONFIG_H
