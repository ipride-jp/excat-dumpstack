#if !defined(_GLOBAL_H)
#define _GLOBAL_H

struct _jvmtiEnv;
struct _jrawMonitorID;

#include "Logger.h"
#include "Config.h"
#include "../jniutility/Monitor.h"

#include <deque>
#define ENCODE_KEY_NUMBER  16

BEGIN_NS(NS_COMMON)

class SimpleStackTrace;

/**
 * ���̃N���X�ł́A�O���[�o���ϐ���ێ�����B
 *
 * @version 1.0
 * @date 2006/02/23
 * @author guanxh
 */
class Global
{
private:
	/**
	 * �R���X�g���N�^[�֎~]
	 */
	Global(void) {};

	/**
	 * �f�X�g���N�^[�֎~]
	 */
	~Global(void) {};

	/**�R���t�B�b�O�E�I�u�W�F�N�g*/
	static Config *config;

	/**error status*/
	static bool errorStatus;

	static unsigned char encodeKey[ENCODE_KEY_NUMBER];

	/**need encode*/
	static bool needEncode;

	static bool isIBMJvm;
    static bool isJrockitJvm;

	//add by Qiu Song on 20090910 for OpenJDK�Ή�
	static bool isOpenJDK;
	//end of add by Qiu Song on 20090910 for OpenJDK�Ή�

public:
    static void setErrorStatus(bool param){ errorStatus = param;};
    static bool isErrorStatus() {return errorStatus;};
    static bool isNeedEncode() {return needEncode;};
	static void setNeedEndode(bool param){ needEncode = param;};
	static unsigned char*getEncodeKey(){return encodeKey;};
	static void setEncodeKey(unsigned char* key,int keyBytes);
    static void setIsIBMJvm(bool param);
	static bool getIsIBMJvm();
	static void setIsJrockitJvm(bool param);
	static bool getIsJrockitJvm();

	//add by Qiu Song on 20090910 for OpenJDK�Ή�
	static void setIsOpenJDK(bool param){ isOpenJDK = param; };
	static bool getIsOpenJDK(){ return isOpenJDK; };
	//end of add by Qiu Song on 20090910 for OpenJDK�Ή�

	/**���O�o�͗p�̃I�u�W�F�N�g*/
	static Logger* logger;

	static deque<SimpleStackTrace*> *stackTraceCache;
	
	/**jvmti�̊��ϐ�*/
	static struct _jvmtiEnv *jvmti;
        static JavaVM* savedvm;

	static int curAttrDepth;

	//static Monitor monitor;
    static struct _jrawMonitorID *configMonitor;
	//added by gancl
	static int dumpFileDelThreadstatus; /* 1:running  0:stop  -1:wait for start  */
	static int monitorThreadStatus; /* 1:running  0:stop  -1:wait for start  */
	static int isJVMToUnload;
	static bool isTrial;
	//end of added by gancl

	static Config* getConfig();

	static void setConfig(Config* newConfig);

	//add by Qiu Song on 20090825 for ���\�b�h�Ď�
// 	static map<string, map<int,int>*>* classFieldTable;
//	static map<string, map<int, string>*>* classConstantPool;
	//end of add by Qiu Song on 20090825 for ���\�b�h�Ď�
};

END_NS

#endif  //_GLOBAL_H
