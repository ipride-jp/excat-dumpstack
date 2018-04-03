#if !defined(_METHOD_H)
#define _METHOD_H
#pragma warning(disable : 4503)
#pragma warning(disable : 4786)

#include "jni.h"
#include "jvmti.h"
#include "DumpParam.h"
#include "ObjectPool.h"
#include "ObjectData.h"
#include "StackTrace.h"
#include "TypedElement.h"

BEGIN_NS(NS_OUTPUT)

typedef struct MethodParamTag
{
	JNIEnv *jni;
	jvmtiEnv *jvmti;
	jthread thread;
	StackTrace *parent;
	jint depth;
	jvmtiFrameInfo *frameInfo;
	ObjectPool *pool;
	MethodData* methodData;
} MethodParam;


class Method  
{
	private:
		MethodParam *param;
        jvmtiLocalVariableEntry *table;
		static const char *TAG_METHOD;
		static const char *ATTR_NAME;
		static const char *ATTR_SIG;
		static const char *ATTR_DECLARING_CLASS;
		static const char *ATTR_LINE_NUM;
		static const char *ATTR_LOCATION;
		
		//add by Qiu Song on 20090817 for メソッド監視
		char * m_objDefinedType;
		char * m_objValue;
		char *m_objName;
		int m_objectRef;
		bool bIsCurrentDumpMethod;
		//end of add by Qiu Song on 20090817 for メソッド監視

	public:
		Method(MethodParam *param, bool bCurDumpMethod = false);
		virtual ~Method();

	private:
		void init();
		void setLineNumber(MethodData* methodData);
        jint setAccessFlags(MethodData* methodData);
		void setMethodName(MethodData* methodData, char* methodName, char* methodSig);
		bool setDeclareClassInfo(MethodData* methodData,char** pClassSig);
        //if has variable talbe return true,others return false;
        bool checkVariableTable(vector<jvmtiLocalVariableEntry *> *pEntryVec);
		jint getLocalVarNumberWithoutDebug(bool isStaticMethod,char** pVarTypes);
		int getArgumentSizeWithotDebug(bool isStaticMethod);
		int checkHasThis(vector<jvmtiLocalVariableEntry *> *pEntryVec,bool isStaticMethod,bool bHasVarTable);
		bool getVariableInfo(TypedElementParam *pTypedParam,jmethodID methodId,int slot,char scanType,jvmtiError* err);
		void ReleaseVariableEntryTable(jvmtiLocalVariableEntry *table, int count);
		bool isNextFrameLoadClass();

		//add by Qiu Song on 20090817 for メソッド監視
		void getMethodReturnValue();
		char* getReturnValueFromVarTable(char valueType, int valueSlot);
//		void getReturnValueFromGetField(int valueSlot);
//		char* getReturnValueFromLdcValueTable(int valueSlot);
		char* getReturnValueFromPushCode(unsigned opCode,int value);
		char* getReturnValueFromConstCode(unsigned opCode);
		//char* getReturnValueFromArray(jint opCodePos);

		void setObjNameFromVarTable(jlocation curPosition, int valueSlot);
//		TypeElementData* getElementData(int valueSlot);
		ObjectData* getArrayObj(int nOpType, int valueSlot);
		int getArrayIndexValue(int nIndexType, unsigned nextCode, int valueSlot);
		char* getVarObjectValue(int valueSlot);
		char* getVarIntegerValue(int valueSlot);
		char* getVarLongValue(int valueSlot);
		char* getVarFloatValue(int valueSlot);
		char* getVarDoubleValue(int valueSlot);
	
		ObjectData* getVarObjectData(int valueSlot, jobject& object);

		void setReturnInfoToMethodData();
		//end of add by Qiu Song on 20090817 for メソッド監視

		//add by Qiu Song on 20091127 for チケット:536
		void releaseMemory();
		//end of add by Qiu Song on 20091127 for チケット:536
};
END_NS

#endif  //_METHOD_H
