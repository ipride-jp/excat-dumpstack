#ifndef INC_CcatTypeValue_hpp_
#define INC_CcatTypeValue_hpp_

#include <stdlib.h>
#include <string>
#include <jni.h>
#include <jvmti.h>
struct _jvmtiEnv;

#include "../common/Define.h"

using namespace std;
BEGIN_NS(NS_ANTLRPARSE)

#define MAX_MSG_LEN 128

typedef struct ParserParamTag
{
	JNIEnv *jni;
	jvmtiEnv *jvmti;
	jthread thread;
	jint depth;
    jvmtiFrameInfo* frameInfo;
} ParserParam;

class CcatTypeValue
{
public:
	typedef enum
	{
	   CTYPE_NONE = 0,          //値なし
	   CTYPE_BOOL,              //boolean
	   CTYPE_NUMBER,            //long
	   CTYPE_FLOAT,             //float
	   CTYPE_DOUBLE,            //double
	   CTYPE_CHAR,              //char
	   CTYPE_STRING,            //string
	   CTYPE_DATE_STRING,       //DateLiteral 
	   //all of below is object type
	   CTYPE_OBJECT_NULL,       //null
	   CTYPE_OBJECT_STRING,     //java.lang.String
	   CTYPE_OBJECT_CHARACTER,  //java.lang.Character
	   CTYPE_OBJECT_BYTE,       //java.lang.Byte
	   CTYPE_OBJECT_SHORT,      //java.lang.Short
	   CTYPE_OBJECT_INTEGER,    //java.lang.Integer
	   CTYPE_OBJECT_LONG,       //java.lang.Long
	   CTYPE_OBJECT_FLOAT,      //java.lang.Float
	   CTYPE_OBJECT_DOUBLE,     //java.lang.Double
	   CTYPE_OBJECT_BOOLEAN,    //java.lang.Boolean
	   CTYPE_OBJECT_DATE,       //java.util.Date
	   CTYPE_OBJECT_OTHER,     //object different above
	   //all of below is object array
	   CTYPE_ARRAY_CHARACTER,  //char[]
	   CTYPE_ARRAY_BYTE,       //byte[]
	   CTYPE_ARRAY_SHORT,      //short[]
	   CTYPE_ARRAY_INTEGER,    //int[]
	   CTYPE_ARRAY_LONG,       //long[]
	   CTYPE_ARRAY_FLOAT,      //float[]
	   CTYPE_ARRAY_DOUBLE,     //double[]
	   CTYPE_ARRAY_BOOL,       //bool[]
	   CTYPE_ARRAY_OBJECT,      //配列オブジェクト
	} CTYPE;

	CTYPE      type;   
    bool       boolValue;
    __int64    numberValue;
    float      floatValue;
    double     doubleValue;
    char*      utf8CharValue;
    string     stringValue;
    jobject    objectValue;

	bool       hasError;
	char*      errorMsg;

public:
	CcatTypeValue();
	~CcatTypeValue();
	CcatTypeValue(const CcatTypeValue& copy);
    CcatTypeValue& operator=(const CcatTypeValue& copy);

	void  setErrorMsg(const char* msg);
	char* getErrorMsg(){return errorMsg;};

    CcatTypeValue operator||(const CcatTypeValue&) const;
    CcatTypeValue operator&&(const CcatTypeValue&) const;
    CcatTypeValue operator==(const CcatTypeValue&) const;
    CcatTypeValue operator!=(const CcatTypeValue&) const;
    CcatTypeValue operator!() const;
    CcatTypeValue operator>(const CcatTypeValue&) const;
    CcatTypeValue operator>=(const CcatTypeValue&) const;
    CcatTypeValue operator<(const CcatTypeValue&) const;
    CcatTypeValue operator<=(const CcatTypeValue&) const;
    CcatTypeValue operator+(const CcatTypeValue&) const;
    CcatTypeValue operator-(const CcatTypeValue&) const;
    CcatTypeValue operator-() const;
    CcatTypeValue operator+() const;
    CcatTypeValue operator*(const CcatTypeValue&) const;
    CcatTypeValue operator/(const CcatTypeValue&) const;
    CcatTypeValue operator%(const CcatTypeValue&) const;
    void getArrayElement(CcatTypeValue& index);
    void getObjectAttribute(const char* p);
    void getAttribute(const char* p);
    void getParameter(const char* p);
	void getObjectValue();

private:
    CcatTypeValue relationValue(CcatTypeValue&, bool justCompareEquale, 
								bool canEqual, char* operatorStr) const;
    void   checkSansuOperands(CcatTypeValue& op2, char* operatorStr);
	void   getDateStringFormat(char* dateFormat);
	int    compareDateString(string date1, string date2, char* dateFormat);
	void   setObjectType();
	void   getByteObjectValue();
	void   getIntObjectValue();
	void   getShortObjectValue();
	void   getLongObjectValue();
	void   getFloatObjectValue();
	void   getDoubleObjectValue();
	void   getBooleanObjectValue();
	void   getCharObjectValue();
	void   getStringObjectValue();
	void   getDateObjectValue(char* dateFormat);
	static int getParamSlotIndex(int paraIndex, char* varTypes, int paramSlotNum);
    void   getObjectAttribute(jclass clazz, const char* p, bool isStaticMethod);
	void   printValue(string& valueStr);  //for debug
};


END_NS
#endif /*INC_CcatTypeValue_hpp_*/
