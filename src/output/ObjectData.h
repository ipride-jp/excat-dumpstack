#ifndef _OBJECT_DATA_H
#define _OBJECT_DATA_H

#pragma warning(disable : 4503)
#pragma warning(disable : 4786)

#include <jvmti.h>
#include <jni.h>
#include <string>
#include <stdio.h>
#include <wchar.h>

#include <vector>
#include "../common/Define.h"


using namespace std;
BEGIN_NS(NS_OUTPUT)

class TypeElementData;
class SuperClassData;
class OutputToFile;

class XMLData
{
public :
	XMLData();
	virtual ~XMLData();
protected:
	void escapeXMLChar(const char* str,char ** bufObj,int maxsize);
	int calEscapeXMLChar(const char* str);
};

class ObjectData :public XMLData
{
private:
	long id;
	char* type;
	char* value;
	SuperClassData* super; 
	//展開不能かどうどうかのフラグ
	bool expanded;
    //一回展開されているかどうかのフラグ
	bool expandedOnce;
	//今、メソッドから見ると、どの階層にある
    int  layersNumber;
	bool isArrayObject;

	//add by Qiu Song on 20091006 for HashCodeの追加
	jlong hashCode;
	//end of add by Qiu Song on 20091006 for HashCodeの追加
protected:
	int itemNumber; //配列の場合の要素項目
protected:
	vector<TypeElementData*> *attributes;
    
public :
	ObjectData();
	virtual ~ObjectData();
	long getDumpSize();
	void setId(long id);
	void setType(const char* typeParam);
	char* getType(){return type;};
	void setValue(const char* valueParam);
	char* getValue(){return value;};//add by Qiu Song on 20090818 for メソッド監視
    void addTypeElementData(TypeElementData *typeElementData);
	void setSuperClass(SuperClassData *superParam);
	SuperClassData* getSuperClass(){return super;};
	void writeToFile(FILE *fp,OutputToFile* opf);

	long  getId(){return id;};
	vector<TypeElementData*> * getSonVector(){return attributes;};
	bool getExpanded(){return expanded;};
	void setExpanded(bool param){ expanded = param;}
	bool getExpandedOnce(){return expandedOnce;};
	void setExpandedOnce(bool param){ expandedOnce = param;}
	int  getLayersNumber(){return layersNumber;};
	void setLayersNumber(int param){ layersNumber = param;};
	bool getIsArrayObject(){return isArrayObject;};
	void setIsArrayObject(bool param){isArrayObject = param;};
	void setItemNumber(int param){itemNumber = param;};
	int  getItemNumber(){return itemNumber;};

	//add by Qiu Song on 20090825 for メソッド監視
	TypeElementData* getAttribute(int nIndex);
	TypeElementData* getAttributeByName(char* attrName);
	TypeElementData* getAttributeByIndex(int attrName);
	void removeAllAttribute();
	void setHashCode(jlong objHashCode){ hashCode = objHashCode;};
	jlong getHashCode(){ return hashCode;};
	//end of add by Qiu Song on 20090825 for メソッド監視
};

class SuperClassData:public ObjectData
{
private:
	char* signature;

public:
    SuperClassData();
	virtual  ~SuperClassData();
	//override
	long getDumpSize();
    void setSignature(const char* param);
	void writeToFile(FILE *fp, OutputToFile* opf);
};

class MethodData:public ObjectData
{
private:
    char* name;
	char* signature;
	char* declaringClass;
	char* sourceFile;
    int location;
	int lineNumber;
	jint modifiers;

	//add by Qiu Song on 20090818 for メソッド監視
	char *objReturnType;
	char *objReturnValue;
	char *objReturnName;
	int objectRef;
	char *exceptionObjName;
	int  exceptionObjRef;
	char *monitorObjName;
	int  monitorObjRef;
	char *useMonitorThreadName;
	//end of add by Qiu Song on 20090818 for メソッド監視

public:
	MethodData();
	virtual  ~MethodData();
	//override
	long getDumpSize();
    void setName(const char* nameParam);
	char* getName(){return name;};
	void setSignature(const char* param);
	void setDeclaringClass(const char* param);
	void setSourceFile(const char* param);
	void setLocation(int param);
	void setLineNumber(int param);
	void setModifiers(jint param);
	const char* getDecalareingClass(){return declaringClass;};
	const char* getSignature(){return signature;}

	//add by Qiu Song on 20090818 for メソッド監視
	void setReturnType(char * returnType);
	char * getReturnType(){return objReturnType;};
	void setReturnValue(char * returnValue);
	char * getReturnValue(){ return objReturnValue;};
	void setReturnName(char * returnName);
	char * getReturnName(){ return objReturnName;};
	void setObjectRef(int refId){ objectRef = refId;};
	int getObjectRef(){ return objectRef;};
	void setExceptionObjName(char* objName);//{ exceptionObjName = objName;};
	void setExceptionRefId(int refId){ exceptionObjRef = refId;};
	void setMonitorObjName(char* objName);//{ monitorObjName = objName;};
	void setMonitorRefId(int refId){ monitorObjRef = refId;};
	void setUseMonitorThreadName(char* objName);//{ useMonitorThreadName = objName;};
	int getLocation(){return location;};
	//end of add by Qiu Song on 20090818 for メソッド監視
	void writeToFile(FILE *fp,OutputToFile* opf);
};

class TypeElementData:public ObjectData
{
private:
    char* tagName;      //Attribute,Argument,Variable,This,Item
    char* objName;
	char* objDefinedType;
	char* value;
	long objectRef;//object poolへの参照、primitiveの場合、-1
	int index;   //subItemである場合
    bool valid;  //変数である場合
	bool unsure; //変数である場合,タイプが確定かどうか
	jint modifiers;

public:
    TypeElementData();
	virtual ~TypeElementData();
	//override
	long getDumpSize();
	void setTagName(const char* tagNameParam);
	void setObjName(const char* objNameParam);
	void setObjDefinedType(const char* typeParam);
	void setValue(const char* valueParam);
	void setObjectRef(long refid);
	void setIndex(int id);
	void setValid(bool validParam);
	void setUnsure(bool param){ unsure = param;};
	void setModifiers(jint param);
	void writeToFile(FILE *fp,OutputToFile* opf);
	char* getObjName(){return objName;};
	int  getIndex(){return index;};
    long  getRefId(){return objectRef;};	
	bool isUnsure(){return unsure;};
	const char* getTagName(){return tagName;};
	//add by Qiu Song on 20090825 for メソッド監視
	char* getObjDefinedType(){return objDefinedType;};
	char* getValue(){return value;};
	//end of add by Qiu Song on 20090825 for メソッド監視
};

class InstanceData : public ObjectData
{
private:
    string className;
	string classUrl;
	vector<long>* objectRef;
	vector<long>* objectSize;
public:
	InstanceData(string& name, string& url);
	virtual ~InstanceData();

	void setObjectRef(vector<long>* objectRefV){objectRef = objectRefV;};
	void setObjectSize(vector<long>* objectSizeV){objectSize = objectSizeV;};
	vector<long>* getObjectRef(){return objectRef;};

	//override
	long getDumpSize();
	void writeToFile(FILE *fp,OutputToFile* opf);
};

class OutputToFile
{
private:
	char* inBuf;
	unsigned char* outBuf;
	long encodePos;
    long oldBufSize;
	int dumpType;
    void encode(char* from,int bytes);
	void freeBuf();
	//add by Qiu Song on 20091016 for RefIdはHashに変更する
	vector<ObjectData*> *m_objectDatas;
	//end of add by Qiu Song on 20091016 for RefIdはHashに変更する
public :
	OutputToFile();
	virtual  ~OutputToFile();
	void setBufSize(long size);
	void setDumpType(int paramType);
    //write to file in utf-8
	void mt2u(FILE* fp);
	char* getInBuffer(){return inBuf;};
	bool writeToFile(const char* fileName,JNIEnv *jni,
		             char* exceptionName,
					 char* threadName,
		             char* strDumptime,
		             vector<ObjectData*> *objectDataes,
					 vector<MethodData*> *methodDataes,
					 vector<InstanceData*> *instanceDataes,
					 int exceptionObjRefId,//add by Qiu Song on 20090811 for 例外オブジェクトダンプ
					 jint threadStutas,//add by Qiu Song on 20090818 for スレッド状態/待機理由
					 jlong cpuTime,//add by Qiu Song on 20090818 for CPU時間
					 jint  threadPriority, //add by Qiu Song on 20091021 for スレッド優先度の指定
					 int monitorObjRefId,  //add by Qiu Song on 20090819 for モニターオブジェクトダンプ
					 char* monitorClassName,//add by Qiu Song on 20090819 for モニターオブジェクトダンプ
					 string className,//add by Qiu Song on 20090828 for メソッド監視
					 string methodName,//add by Qiu Song on 20090828 for メソッド監視
					 string methodSig,//add by Qiu Song on 20090828 for メソッド監視
					 string dumpPosition,//add by Qiu Song on 20090828 for メソッド監視
					 char* useMonitorThreadName, //add by Qiu Song on 20091006 for モニターオブジェクトダンプ
					 bool dumpCompleted);
	//add by Qiu Song on 20091016 for RefIdはHashに変更する
	void setObjectDatas(vector<ObjectData*> *objectDataes){ m_objectDatas = objectDataes;};
	ObjectData* getObjectData(int id){ return (ObjectData*)m_objectDatas->at(id);};
	jlong getObjectHashCode(int id);
	//end of add by Qiu Song on 20091016 for RefIdはHashに変更する
};

END_NS
#endif
