#if !defined(_CLASSFILE_H)
#define _CLASSFILE_H

#include <jni.h>
#include <string>
#include <vector>
#include <map>

#include "BaseInfo.h"

using namespace std;

USE_NS(NS_JNI_UTILITY)
BEGIN_NS(NS_CLASS_FILE_OPERATION)

class CpInfo;
class FieldInfo;
class AttributeInfo;
class MethodInfo;

#define ACC_INTERFACE  0x0200
#define ACC_ABSTRACT   0x0400
#define ACC_NATIVE     0x0100

class ClassFile : public BaseInfo
{
public:
	ClassFile(jint classDataLen, const unsigned char *classData,ExceptionTableMan* param);
	ClassFile(jint classDataLen, const unsigned char *classData);
	virtual ~ClassFile();

	int insertByteCodeToClass(int modifyType, int insertMethodCount, 
							  string *insertMethodName, string *insertMethodSig, 
							  int *insertMethodPosition,string paramClassName,
							  string paramClassUrl,unsigned char **newClassData,
							  long *newClassDataLen);

	void insertMethodIntoConstructor(const char *destClassName, const char *className, const char *methodName, 
		const char*methodSig, unsigned char **newClassData, long *newClassDataLen);

	void getUtf8ConstantInfoNeedMemory(int cpIndex, char **buf);
	void getUtf8ConstantInfo(int cpIndex, char *buf);
    bool  isInterface(); 
    bool  isMethodDefValid(const char* methodName, 
		                   const char* methodSignature, 
						   char* errorMsg); 
    bool isClassNameEqual(const char* className);
	void getConstantClassName(int cpIndex, char *buf);

	static U2 getU2ReverseValue(U2 *obj) {return ((*(U1*)obj) << 8) + *((U1*)obj + 1);};
	static U4 getU4ReverseValue(U4 *obj) {return (getU2ReverseValue((U2*)obj) << 16) + getU2ReverseValue((U2*)obj + 1);};

	//add by Qiu Song on 20090812 for メソッド監視
	bool insertCallbackFuncBeforeReturn(unsigned char** outputPos, int paramIndex_forMethod,
		                                int methodIndex_forMethod, unsigned char * byteCodeData,
										int len, map<int,int>* byteCodeLineChangeMap, int& nNewPos);
	bool findReturnPos(unsigned char * byteCodeData, int len, int nInsertLength,map<int,int>* byteCodeLineChangeMap);
	unsigned getCodePosAfterInsertCallbacks(unsigned oldPos, map<int, int>* byteCodeLineChangeMap, bool bBeforeEnd = false);
	unsigned getAddedSizeAfterInsertCallbacks(map<int, int>* byteCodeLineChangeMap);
	void modifyJumpTarget(unsigned char** outputPos, unsigned opCode, unsigned char * byteCodeData,
						  int& nPos, int& nNewPos, map<int, int>* byteCodeLineChangeMap, bool bBeforeEnd = false);
	void modifySwitchTarget(unsigned char** outputPos, unsigned opCode, unsigned char * byteCodeData,
							int& nPos, int& nNewPos, map<int, int>* byteCodeLineChangeMap, bool bBeforeEnd = false);
	void modifySwitchCommon(unsigned char** outputPos, unsigned opCode, unsigned char * byteCodeData,
							int& nPos, int& nNewPos, int nStartPos, int nNewStartPos,
							map<int, int>* byteCodeLineChangeMap, bool bBeforeEnd = false);
	void modifyLookupSwitch(unsigned char** outputPos, unsigned opCode, unsigned char * byteCodeData,
		                    int& nPos, int& nNewPos, int nStartPos, int nNewStartPos,
							map<int, int>* byteCodeLineChangeMap, bool bBeforeEnd = false);
	void modifyTableSwitch(unsigned char** outputPos, unsigned opCode, unsigned char * byteCodeData,
		                   int& nPos, int& nNewPos, int nStartPos, int nNewStartPos,
						   map<int, int>* byteCodeLineChangeMap, bool bBeforeEnd = false);
	void getSwitchCodeAddedSize(int nPos, map<int,int>* byteCodeLineChangeMap, int nInsertLength);
	bool isReturnOperand(unsigned opCode);
	bool isJumpOperand(unsigned opCode);
	bool isSwitchOperand(unsigned opCode);
	bool isTypeChangeCode(unsigned opCode);
	bool isLdcCode(unsigned opCode);
	bool isGetFieldCode(unsigned opCode);
	bool isInvokeCode(unsigned opCode);
	//運算コード稼動か判断関数
    bool isMethodOpCode(unsigned opCode);
	unsigned readU1(unsigned char * byteCodeData, int pos);
	unsigned readU2(unsigned char * byteCodeData, int pos);
	unsigned readU4(unsigned char * byteCodeData, int pos);
	long readLongInfo(unsigned char * byteCodeData, int pos);
	int getOpCodeSkipLength(unsigned char * byteCodeData, int pos,unsigned opcode);
	int opcode_length(unsigned opcode);
	void insertCallbacksFunc(unsigned char** outputPos, int paramIndex_forMethod, int methodIndex_forMethod,
		                     int& nInsertLength);
	void insertLocalParamBeforeReturn(unsigned char** outputPos, unsigned opCode, int attributeMaxLocal,
		                              int &nNewPos);
	void getStoreAndCode(unsigned opCode, int attributeMaxLocal, unsigned& storeCode, unsigned& loadCode);
	bool copyTheLeftCode(unsigned char** outputPos, unsigned char * byteCodeData, 
						 int len, int nInsertLength,
						 map<int,int>* byteCodeLineChangeMap, int& nNewPos);
	bool findSwitchPos(unsigned char * byteCodeData, int len,
				  int nInsertLength, map<int,int>* byteCodeLineChangeMap);
	void copyByteCodeWithSwitch(unsigned char** outputPos, int nInsertLength,
		                        unsigned char * byteCodeData, int len,
								map<int,int>* byteCodeLineChangeMap, int& nNewPos);
//	void setReturnValueIntoLdcTable(unsigned char * byteCodeData, int nPos, unsigned opCode);
//	char* getReturnValueFromConstantPool(int nIndex);
//	char* getReturnValueFromConstantStringInfo(int nIndex);
//	char* getReturnValueFromConstantIntegerInfo(int nIndex);
//	char* getReturnValueFromConstantLongInfo(int nIndex);
//	char* getReturnValueFromConstantFloatInfo(int nIndex);
//	char* getReturnValueFromConstantDoubleInfo(int nIndex);
	int  findUtf8InfoConstantPos(char* searchValue);
    void addMethodReturnTypeToConstantPool(unsigned char*& outputPos, unsigned char* pConstantPoolCountPos, int& nConstantPoolCount);
	//Callbacksメソッド挿入したの各テーブルの新位置を取得する
    unsigned getTablePosAfterInsertCallbacks(unsigned oldPos, map<int, int>* byteCodeLineChangeMap,
		                                     int size, int nInsertLength);
//	map<int,int>* fieldTable;//クラスファイルのフィールドテーブル(constantPoolの位置→field一覧の位置)
//	map<int, string>* ldcValueTable;//ldc系コードに関連するconstPoolの値を格納するテーブル
	bool bHasGetField;//リターンコードの前にgetfieldがあるかどうかフラグ
	bool bHasLdc;//リターンコードの前にldcがあるかどうかフラグ
	bool bNeedAddLocal;//一時変数を追加したかどうかフラグ
	int nMaxLocal;//一時変数個数
	bool bLongOrDoubleReturn;//戻り値はlong或いはdoubleかどうか判断フラグ
	map<int, string>* addedConstantUtf8Info;//追加したconstantUtf8Info;
	//end of add by Qiu Song on 20090812 for メソッド監視

	void setExceptionTableMan(ExceptionTableMan* param){pExceptionTableMan = param;};
    class ExceptionTableMan* getExceptionTableMan(){return pExceptionTableMan;};
	char* getClassNameOFMyself(){return className;};

private:
	const unsigned char *classData;

	U4 *magic;
	U2 *minorVersion;
	U2 *majorVersion;
	U2 *constantPoolCount;
	CpInfo **constantPool;
	U2 *accessFlags;
	U2 *thisClass;
	U2 *superClass;
	U2 *interfacesCount;
	U1 *interfaces; //位置を保存するだけ
	U2 *fieldsCount;
	FieldInfo **fields;
	U2 *methodsCount;
	MethodInfo **methods;
	U2 *attributesCount;
	AttributeInfo **attributes;

	char* className;
	class ExceptionTableMan* pExceptionTableMan;
private:
	static void addUtf8CpoolEntry(unsigned char *&pos, const char *str);
	static void addClassInfoCpoolEntry(unsigned char *&pos, int nameIndex);
	static void addStringInfoCpoolEntry(unsigned char *&pos, int nameIndex);
	static void addNameAndTypeCpoolEntry(unsigned char *&pos, int nameIndex, int descrIndex);
	static void addMethodrefCpoolEntry(unsigned char *&pos, int classIndex, int nameTypeIndex);
	static void writeU1(unsigned char *&pos, U1 val);
	static void writeU2(unsigned char *&pos, U2 val);
	static void writeU4(unsigned char *&pos, U4 val);

	U1 * findInvokeSpecialPos(AttributeInfo *attrInfo);
	void getInvokeSpecialClassName(U1 *invokeSpecialPos, char *className);
	void initial(jint classDataLen, const unsigned char *classData);

};
END_NS

#endif  //_CLASSFILE_H
