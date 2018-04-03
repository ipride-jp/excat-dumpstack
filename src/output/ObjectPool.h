#if !defined(_OBJECTPOOL_H)
#define _OBJECTPOOL_H

#pragma warning(disable : 4786) //警告C4786を禁止

#include <vector>
#include <map>
#include <jni.h>
#include <jvmti.h>
#include "../common/Define.h"

using namespace std;

BEGIN_NS(NS_OUTPUT)
class StackTrace;
typedef struct ObjectPoolParamTag
{
	JNIEnv *jni;
	jvmtiEnv *jvmti;
	jthread thread;
	StackTrace* parent;
} ObjectPoolParam;

class ObjectData;
class ObjectPool;
typedef struct TypedElementParamTag
{
	StackTrace* ancestor;
	JNIEnv *jni;
	jvmtiEnv *jvmti;
	jobject object;
	char *objDefinedType;
	char *objName;
	char *objValue;
	ObjectData *parent;
	ObjectPool *pool;
	int arrIndex;
	int objectRef;
	bool valid;
	bool unsure;//not sure of the type because there is no variable table
	jint modifiers;
} TypedElementParam;

class ObjectPool
{
public:
	ObjectPool(ObjectPoolParam *param);
	virtual ~ObjectPool();

	int addObject(jobject object);
	//jobject getObject(int id);
	int getObjectId(jobject object);
    void expandObject(jobject object,ObjectData* objData,StackTrace *ancestor,
							  int currentLayer);
	void addArrayElements(TypedElementParam *paramArr,StackTrace *ancestor);

	//add by Qiu Song on 20090903 for 指定タイプのオブジェクトを値のみダンプ
	void filterUselessObject(StackTrace* ancestor, ObjectData* objData, char* sig, int nPos, int nObjNumberBak);
	void setBigIntegerValue(ObjectData* objData, StackTrace *ancestor);
	void setBigDecimalValue(ObjectData* objData, StackTrace *ancestor);
	void setStringBufferValue(ObjectData* objData, StackTrace *ancestor);
	void setDateValue(ObjectData* objData, StackTrace *ancestor);
	void setCommonObjValue(ObjectData* objData, StackTrace *ancestor);
	void setArrayObjValue(ObjectData* objData, StackTrace *ancestor);
	void setDumpObjValue(ObjectData* objData, StackTrace *ancestor);
    void removeObjectMapFromPos(int nPos);
	bool bNeedFilterObject;
	long  expendingObjectId;
	//end of add by Qiu Song on 20090903 for 指定タイプのオブジェクトの値のみダンプ
private:
    map<jlong,int> *objectMap;
	ObjectPoolParam *param;
    int objNumber;
	static const char *TAG_OBJECT_POOL;
	static const char *TAG_OBJECT;
	static const char *ATTR_ID;
	static const char *ATTR_TYPE;
	static const char *ATTR_VALUE;

	static const char *CLASS_STRING;

	int getDumpArraySize(int arraySize,bool isObject);
};



END_NS

#endif
