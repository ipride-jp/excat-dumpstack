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
 * このクラスでは、グローバル変数を保持する。
 *
 * @version 1.0
 * @date 2006/02/23
 * @author guanxh
 */
class Global
{
private:
	/**
	 * コンストラクタ[禁止]
	 */
	Global(void) {};

	/**
	 * デストラクタ[禁止]
	 */
	~Global(void) {};

	/**コンフィッグ・オブジェクト*/
	static Config *config;

	/**error status*/
	static bool errorStatus;

	static unsigned char encodeKey[ENCODE_KEY_NUMBER];

	/**need encode*/
	static bool needEncode;

	static bool isIBMJvm;
    static bool isJrockitJvm;

	//add by Qiu Song on 20090910 for OpenJDK対応
	static bool isOpenJDK;
	//end of add by Qiu Song on 20090910 for OpenJDK対応

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

	//add by Qiu Song on 20090910 for OpenJDK対応
	static void setIsOpenJDK(bool param){ isOpenJDK = param; };
	static bool getIsOpenJDK(){ return isOpenJDK; };
	//end of add by Qiu Song on 20090910 for OpenJDK対応

	/**ログ出力用のオブジェクト*/
	static Logger* logger;

	static deque<SimpleStackTrace*> *stackTraceCache;
	
	/**jvmtiの環境変数*/
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

	//add by Qiu Song on 20090825 for メソッド監視
// 	static map<string, map<int,int>*>* classFieldTable;
//	static map<string, map<int, string>*>* classConstantPool;
	//end of add by Qiu Song on 20090825 for メソッド監視
};

END_NS

#endif  //_GLOBAL_H
