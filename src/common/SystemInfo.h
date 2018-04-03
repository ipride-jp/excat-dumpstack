// SystemInfo.h: SystemInfo クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SYSTEMINFO_H__92C80954_5B09_4F7D_B28B_66F8AAD8CE3B__INCLUDED_)
#define AFX_SYSTEMINFO_H__92C80954_5B09_4F7D_B28B_66F8AAD8CE3B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <jni.h>
#include "Define.h"
#include "../output/ObjectData.h"

BEGIN_NS(NS_COMMON)

USE_NS(NS_OUTPUT)
class SystemInfo:public XMLData
{
public:
	SystemInfo();
	virtual ~SystemInfo();
private:
	char* systemID;
	char* jvmID;
	char* jvmIDEscaped;
	char* jvmVersion;
	char* jvmName;
	char* jvmVendor;
	bool initialized;
	static SystemInfo* systemInfo;
	
public:
	char* getSystemID(){return systemID;};
	char* getJvmID(){return jvmID;};
	char* getJvmVersion(){return jvmVersion;};
	char* getJvmName(){return jvmName;};
	char* getJvmVendor(){return jvmVendor;};
	char* getJvmIDEscaped(){return  jvmIDEscaped;};
	void  setJvmID(char* param);
    static SystemInfo* getInstance();
	static void deleteInstance();
	void  getJVMInfo(JNIEnv* jni);
private:
	bool getValueOfKey(JNIEnv* jni,jclass classSystem,
		jmethodID methodGetProperty,char* key,char** value);
	bool getSystemName();
};

END_NS

#endif // !defined(AFX_SYSTEMINFO_H__92C80954_5B09_4F7D_B28B_66F8AAD8CE3B__INCLUDED_)
