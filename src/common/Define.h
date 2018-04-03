
#if !defined(_DEFINE_H)

#define _DEFINE_H
/*
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
*/
#define CERT_FILE						"Excat_cert.pem"

//we will change the Ccat=2.0 to Excat=2.0 later
#define EXCATVERSION				    "Ccat=3.0"//modified by Qiu Song on 20091029 for Ver3.0�̃��C�Z���X�̑Ή�

/**Buffer�̍ő�̒���*/
//test
//#define MAX_BUF_LEN						256
#define MAX_BUF_LEN						8192

/**�R���t�B�b�O�E�t�@�C����*/
#define CONFIG_FILE_NAME				"DumpStack.config"

/**�N���X�E�t�B�[���h�̃X�^�b�e�B�N�C���q*/
#define ACC_STATIC						8

/**public�C���q*/
#define ACC_PUBLIC						1

/**private�C���q*/
#define ACC_PRIVATE						2

/**protected�C���q*/
#define ACC_PROTECTED					4

/**�C���X�^���X�̍ő�� */
#define MAX_INSTANCE_COUNT				32

/** �_���v�t�@�C���̏o�̓p�X�̍Œ���̃T�C�Y (100MB) */
#define MIN_DISK_SIZE  104857600

/**�o�C�g�R�[�h�̍ő�̃T�C�Y*/
#define MAX_BYTECODE_SIZE 10000

/**String�̍ő�T�C�Y**/
#define MAX_STRING_LENGTH 16383

//dump kind
//0:for exception,1:for method, 2:for thread(signal)
#define DUMP_KIND_EXCEPTION             0
#define DUMP_KIND_METHOD                1
#define DUMP_KIND_THREAD                2 
#define DUMP_KIND_EXCEPTION_ALL         3  //��O�������ɁA���ׂẴX���b�h���_���v
#define DUMP_KIND_METHOD_ALL            4  //�Ď��Ώۃ��\�b�h���Ă΂�鎞�ɁA���ׂẴX���b�h���_���v
#define DUMP_KIND_THROWABLE             5  //���ׂĂ̗�O���Ď�

//���O��O�t�@�C����
#define EXCLUDE_EXCEPTION_FILE_NAME "excludeExceptionFile.txt"

//�߂�l���i�[����ꎞ�ϐ�
#define METHOD_RETURN_VALUE "Excat_CallBack_ReturnValue"

//�߂�l�̃^�C�v
#define BIPUSH_RETURN 0
#define VAR_RETURN  1
#define CONST_RETURN 2

/**�p�����[�^�uattributes�v����uattrName�v�̑����̒l���擾*/
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

/**���O��ԁudumpstack�v*/
#define NS_DUMP_STACK					dumpstack

/**���O��ԁuclassfileoperation�v*/
#define NS_CLASS_FILE_OPERATION			classfileoperation

/**���O��ԁucommon�v*/
#define NS_COMMON						common

/**���O��ԁujniutility�v*/
#define NS_JNI_UTILITY					jniutility

/**���O��ԁuoutput�v*/
#define NS_OUTPUT						output

/**���O��ԁuantlrparse�v*/
#define NS_ANTLRPARSE					antlrparse

/**���O��ԁux�v���J�n*/
#define BEGIN_NS(x) \
	namespace NS_DUMP_STACK \
	{ \
		namespace x \
		{								

/**���O��Ԃ��I��*/
#define END_NS \
		} \
	}
				
/**���O��ԁux�v���g�p*/
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


