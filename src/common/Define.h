
#if !defined(_DEFINE_H)

#define _DEFINE_H
/*
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
*/
#define CERT_FILE						"Excat_cert.pem"

//we will change the Ccat=2.0 to Excat=2.0 later
#define EXCATVERSION				    "Ccat=3.0"//modified by Qiu Song on 20091029 for Ver3.0のライセンスの対応

/**Bufferの最大の長さ*/
//test
//#define MAX_BUF_LEN						256
#define MAX_BUF_LEN						8192

/**コンフィッグ・ファイル名*/
#define CONFIG_FILE_NAME				"DumpStack.config"

/**クラス・フィールドのスタッティク修飾子*/
#define ACC_STATIC						8

/**public修飾子*/
#define ACC_PUBLIC						1

/**private修飾子*/
#define ACC_PRIVATE						2

/**protected修飾子*/
#define ACC_PROTECTED					4

/**インスタンスの最大個数 */
#define MAX_INSTANCE_COUNT				32

/** ダンプファイルの出力パスの最低限のサイズ (100MB) */
#define MIN_DISK_SIZE  104857600

/**バイトコードの最大のサイズ*/
#define MAX_BYTECODE_SIZE 10000

/**Stringの最大サイズ**/
#define MAX_STRING_LENGTH 16383

//dump kind
//0:for exception,1:for method, 2:for thread(signal)
#define DUMP_KIND_EXCEPTION             0
#define DUMP_KIND_METHOD                1
#define DUMP_KIND_THREAD                2 
#define DUMP_KIND_EXCEPTION_ALL         3  //例外発生時に、すべてのスレッドをダンプ
#define DUMP_KIND_METHOD_ALL            4  //監視対象メソッドが呼ばれる時に、すべてのスレッドをダンプ
#define DUMP_KIND_THROWABLE             5  //すべての例外を監視

//除外例外ファイル名
#define EXCLUDE_EXCEPTION_FILE_NAME "excludeExceptionFile.txt"

//戻り値を格納する一時変数
#define METHOD_RETURN_VALUE "Excat_CallBack_ReturnValue"

//戻り値のタイプ
#define BIPUSH_RETURN 0
#define VAR_RETURN  1
#define CONST_RETURN 2

/**パラメータ「attributes」から「attrName」の属性の値を取得*/
#define GET_ATTR_VALUE(attributes, attrName, value) \
	{ \
		XMLCh buf[MAX_BUF_LEN]; \
		XMLString::transcode(attrName , buf, MAX_BUF_LEN - 1); \
		DOMNode *node = attributes->getNamedItem(buf); \
		value = NULL; \
		if (node != NULL) \
		{ \
			value = XMLString::transcode(node->getNodeValue()); \
		} \
	}															

/**名前空間「dumpstack」*/
#define NS_DUMP_STACK					dumpstack

/**名前空間「classfileoperation」*/
#define NS_CLASS_FILE_OPERATION			classfileoperation

/**名前空間「common」*/
#define NS_COMMON						common

/**名前空間「jniutility」*/
#define NS_JNI_UTILITY					jniutility

/**名前空間「output」*/
#define NS_OUTPUT						output

/**名前空間「antlrparse」*/
#define NS_ANTLRPARSE					antlrparse

/**名前空間「x」が開始*/
#define BEGIN_NS(x) \
	namespace NS_DUMP_STACK \
	{ \
		namespace x \
		{								

/**名前空間が終了*/
#define END_NS \
		} \
	}
				
/**名前空間「x」を使用*/
#define USE_NS(x)						using namespace NS_DUMP_STACK::x;

#define AUTO_REL_JVMTI_OBJECT(x)		JvmtiAutoRelease jvmtiWrapper##x = JvmtiAutoRelease((void *)x)

#define AUTO_REL_JNI_LOCAL_REF(x, y)	JniLocalRefAutoRelease jniLocalRefWrapper##y = JniLocalRefAutoRelease(x, (jobject)y)

#define AUTO_REL_OBJECT(x)				ObjectAutoRelease objAutoRelWrapper##x = ObjectAutoRelease((void *)x)

#define AUTO_REL_OBJECT_ARR(x)			ObjectAutoRelease objAutoRelWrapper##x = ObjectAutoRelease((void *)x, true)


#define EXCAT_HOME  "EXCAT_HOME"
#define DUMPSTACK_DIR   "DumpStack"
#ifdef _LINUX
#define FILE_SPERATOR "/"
#define FILE_SPERATOR_C '/'
#else
#define FILE_SPERATOR "\\"
#define FILE_SPERATOR_C '\\'
#endif

#define CALLBACK_CLASS_NAME  "Callbacks"
#define CALLBACK_METHOD_FOR_METHOD       "callback_for_method"
#define CALLBACK_METHOD_FOR_METHOD_SIG   "(Ljava/lang/String;)V"
#define CALLBACK_METHOD_FOR_INSTANCE     "callback_for_instance"
#define CALLBACK_METHOD_FOR_INSTANCE_SIG "(Ljava/lang/Object;Ljava/lang/String;Ljava/lang/String;)V"

#ifdef _LINUX
#define SNPRINTF snprintf
#else
#define SNPRINTF _snprintf
#endif

#ifdef _LINUX
#define __int64 long long 
#endif

#endif  //_DEFINE_H


