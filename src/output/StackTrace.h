#if !defined(_STACKTRACE_H)
#define _STACKTRACE_H

#include <string>
#include <vector>
#include <stdio.h>
#include <jni.h>
#include <jvmti.h>

#include "DumpParam.h"
#include "ObjectPool.h"
#include "ObjectData.h"

using namespace std;

//moved by Qiu Song on 20090810 for excat Version 3.0 
//move from BEGIN_IS(NS_OUTPUT) to here
typedef struct DumpFileTime
{
	int m_year;
	int m_month;
	int m_day;
	int m_hour;
	int m_min;
	int m_sec;
	int m_millianSec;
} DumpFileTime;
//end of moved by Qiu Song on 20090810 for excat Version 3.0

BEGIN_NS(NS_OUTPUT)
//using namespace std;

//add by Qiu Song on 20090908 for �o�O�F�t�@�C�����Ɠ��e�ɕ\������鎞�Ԃ͈�v���Ȃ�
typedef struct StackTraceParamTag
{
	JNIEnv   *jni;
	jvmtiEnv *jvmti;
	jthread  thread;
	jclass   exceptionCls;
	short    dumpType; //0: for exceprion; 1: for method
	const char* utf8ExceptionName;
	char* exceptionName;
	char* monitorClassName;
	string   classUrl;
	string   className;
	string   methodName;
	string   methodSignature;
	string   methodSuffix;
	string   classSuffix;
	bool     dumpInstance;
	int      dumpCount;//count for one trigger
	jobject  exceptionObj;//��O�I�u�W�F�N�g
	jint     threadStutas;//�X���b�h���
	jlong    cpuTime;//CPU����
	jint     threadPriority;//�X���b�h�D��x
	jobject  monitorObj;//���j�^�[�I�u�W�F�N�g
	char*  useMonitorThread;//�������Ă��郂�j�^�[�̏��L�X���b�h
	char*   packageName;//�p�b�P�[�W��(�����Ď���p)
	virtual ~StackTraceParamTag();
} StackTraceParam;

//end of add by Qiu Song on 20090908 for �o�O�F�t�@�C�����Ɠ��e�ɕ\������鎞�Ԃ͈�v���Ȃ�

class  ExceedMemLimitException: public exception  {

public:
	ExceedMemLimitException(const char *msg);

    virtual ~ExceedMemLimitException() throw();
    const char* what() throw(){
		return msg;
	};
private:
	char* msg;
};

class StackTrace
{
private:
	StackTraceParam *param;
	static const char *ATTR_DUMP_TIME;
	static const char *TAG_STACK_TRACE;
	class ObjectPool *objectPool;
    
	vector<ObjectData*> *objectDataes;

    char strDumptime[64];  //�_���v������String
	long  iThreadId;   //�_���v�X���b�h��ID
    vector<MethodData*> *methodDataes;
    vector<InstanceData*> *instanceDataes;
    long  lDumpDataSize;
    bool  dumpCompleted;
	char* threadName;
	//add by Qiu Song on 20090811 for Excat Version 3.0
	int   exceptionObjRefId; //��O�I�u�W�F�N�g��Ref ID
	int   monitorObjRefId;   //���j�^�[�I�u�W�F�N�g��Ref ID
	char* monitorClassName;  //���j�^�[�I�u�W�F�N�g�̃N���X��
	string strDumpPosition;  //�Ď����\�b�h�̃_���v�ʒu
	DumpFileTime* curDumpTime;    //�_���v�J�n����
	//end of add by Qiu Song on 20090811 for Excat Version 3.0
public:
	StackTrace(StackTraceParam *param);
	virtual ~StackTrace();
    void addObjectData(ObjectData *objectData);
	void addMethodData(MethodData *methodData);
	void addInstanceData(InstanceData *instanceData);
	bool writeToFile(const char* fileName,JNIEnv *jni);
	int getMethodNum(){return methodDataes->size();};
	int getInstanceNum(){return instanceDataes->size();};
    ObjectData* getObjectData(int id);
    void addDumpDataSize(long dataSize);
    void init();
	void dumpThread(bool currentThreadFlag);
	void dumpInstanceForSignal();
	bool shouldDumpException();
	bool shouldDumpMethod();

	//add by Qiu Song on 20090818 for Excat Version 3.0
	short getDumpType(){return param->dumpType;};
	void getThreadStatusAndCPUTime();
	int getObjRefIdFromObjectPool(jobject obj,ObjectPool* objectPool);
	void setMonitorClassName();
	ObjectData* getFieldObjectData(const char* className);
	void setDumpPosition(string strPos){strDumpPosition = strPos;};
	string getDumpPosition(){return strDumpPosition;};
	vector<ObjectData*> * getAllObjDatas(){return objectDataes;};
	void removeObjectFromPos(int nPos);
	void setDumpTime();
	DumpFileTime* getDumpFileTime();
	bool isThreadStatusMatch();
	string getParamClassURL(){return param->classUrl;};
	//end of add by Qiu Song on 20090818 for Excat Version 3.0
private:
	void dumpStack(jvmtiFrameInfo *frameInfos,int pos ,jint count,bool suspendThread);
	bool validateExceptionPlace(int frameIndex,jvmtiFrameInfo *frameInfos,int pos);
    bool shouldExclude(int posException,int frameCount,jvmtiFrameInfo *frameInfos);
    bool shouldDump(int checkPos,jvmtiFrameInfo *frameInfos);
    bool validateExcludePlace(int frameIndex,jvmtiFrameInfo *frameInfos);
	bool isSubClass(char* objectClass,char * parentClass);						
	bool isDuplicate(jvmtiFrameInfo *frameInfos, int pos, int count);
	void appendAutoDelFileName(const char* fileName);
	char* getAutoDelListFileName(int saveDays);
	bool exceptionInMail(int frameCount, jvmtiFrameInfo *frameInfos);
};
END_NS

#endif  //_STACKTRACE_H
