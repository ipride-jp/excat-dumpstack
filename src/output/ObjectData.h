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
	//�W�J�s�\���ǂ��ǂ����̃t���O
	bool expanded;
    //���W�J����Ă��邩�ǂ����̃t���O
	bool expandedOnce;
	//���A���\�b�h���猩��ƁA�ǂ̊K�w�ɂ���
    int  layersNumber;
	bool isArrayObject;

	//add by Qiu Song on 20091006 for HashCode�̒ǉ�
	jlong hashCode;
	//end of add by Qiu Song on 20091006 for HashCode�̒ǉ�
protected:
	int itemNumber; //�z��̏ꍇ�̗v�f����
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
	char* getValue(){return value;};//add by Qiu Song on 20090818 for ���\�b�h�Ď�
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

	//add by Qiu Song on 20090825 for ���\�b�h�Ď�
	TypeElementData* getAttribute(int nIndex);
	TypeElementData* getAttributeByName(char* attrName);
	TypeElementData* getAttributeByIndex(int attrName);
	void removeAllAttribute();
	void setHashCode(jlong objHashCode){ hashCode = objHashCode;};
	jlong getHashCode(){ return hashCode;};
	//end of add by Qiu Song on 20090825 for ���\�b�h�Ď�
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

	//add by Qiu Song on 20090818 for ���\�b�h�Ď�
	char *objReturnType;
	char *objReturnValue;
	char *objReturnName;
	int objectRef;
	char *exceptionObjName;
	int  exceptionObjRef;
	char *monitorObjName;
	int  monitorObjRef;
	char *useMonitorThreadName;
	//end of add by Qiu Song on 20090818 for ���\�b�h�Ď�

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

	//add by Qiu Song on 20090818 for ���\�b�h�Ď�
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
	//end of add by Qiu Song on 20090818 for ���\�b�h�Ď�
	void writeToFile(FILE *fp,OutputToFile* opf);
};

class TypeElementData:public ObjectData
{
private:
    char* tagName;      //Attribute,Argument,Variable,This,Item
    char* objName;
	char* objDefinedType;
	char* value;
	long objectRef;//object pool�ւ̎Q�ƁAprimitive�̏ꍇ�A-1
	int index;   //subItem�ł���ꍇ
    bool valid;  //�ϐ��ł���ꍇ
	bool unsure; //�ϐ��ł���ꍇ,�^�C�v���m�肩�ǂ���
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
	//add by Qiu Song on 20090825 for ���\�b�h�Ď�
	char* getObjDefinedType(){return objDefinedType;};
	char* getValue(){return value;};
	//end of add by Qiu Song on 20090825 for ���\�b�h�Ď�
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
	//add by Qiu Song on 20091016 for RefId��Hash�ɕύX����
	vector<ObjectData*> *m_objectDatas;
	//end of add by Qiu Song on 20091016 for RefId��Hash�ɕύX����
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
					 int exceptionObjRefId,//add by Qiu Song on 20090811 for ��O�I�u�W�F�N�g�_���v
					 jint threadStutas,//add by Qiu Song on 20090818 for �X���b�h���/�ҋ@���R
					 jlong cpuTime,//add by Qiu Song on 20090818 for CPU����
					 jint  threadPriority, //add by Qiu Song on 20091021 for �X���b�h�D��x�̎w��
					 int monitorObjRefId,  //add by Qiu Song on 20090819 for ���j�^�[�I�u�W�F�N�g�_���v
					 char* monitorClassName,//add by Qiu Song on 20090819 for ���j�^�[�I�u�W�F�N�g�_���v
					 string className,//add by Qiu Song on 20090828 for ���\�b�h�Ď�
					 string methodName,//add by Qiu Song on 20090828 for ���\�b�h�Ď�
					 string methodSig,//add by Qiu Song on 20090828 for ���\�b�h�Ď�
					 string dumpPosition,//add by Qiu Song on 20090828 for ���\�b�h�Ď�
					 char* useMonitorThreadName, //add by Qiu Song on 20091006 for ���j�^�[�I�u�W�F�N�g�_���v
					 bool dumpCompleted);
	//add by Qiu Song on 20091016 for RefId��Hash�ɕύX����
	void setObjectDatas(vector<ObjectData*> *objectDataes){ m_objectDatas = objectDataes;};
	ObjectData* getObjectData(int id){ return (ObjectData*)m_objectDatas->at(id);};
	jlong getObjectHashCode(int id);
	//end of add by Qiu Song on 20091016 for RefId��Hash�ɕύX����
};

END_NS
#endif
