#pragma warning(disable : 4503)
#pragma warning(disable : 4786) //�x��C4786���֎~

#if !defined(_HELPERFUNC_H)
#define _HELPERFUNC_H

#include <jvmti.h>

#include <xercesc/dom/DOM.hpp>
#include "Define.h"

#ifndef XERCESC20
XERCES_CPP_NAMESPACE_USE
#endif

BEGIN_NS(NS_COMMON)


/**
 * ���̃N���X�ł́A�l�X�ȃw���p�[�֐���񋟂���B
 */
class HelperFunc  
{
private:
	/**
	 * �R���X�g���N�^[�֎~]
	 */
	HelperFunc() {};

	/**
	 * �f�X�g���N�^[�֎~]
	 */
	virtual ~HelperFunc() {};

	/**
	 * primitive�^�C�v��JVM�����̌`�����畁�ʂ̌`���ɕϊ�
	 * 
	 * @param internalType JVM�����`����primitive�^�C�v
	 * @return ���ʌ`����primitive�^�C�v
	 */
	static char *getPrimitiveType(const char *internalType);

	/**
	 * �I�u�W�F�N�g�̃^�C�v��JVM�����̌`�����畁�ʂ̌`���ɕϊ�
	 * 
	 * @param internalType JVM�����`���̃I�u�W�F�N�g�E�^�C�v
	 * @return ���ʌ`���̃I�u�W�F�N�g�E�^�C�v
	 */
	static char *getObjectType(const char *internalType);

	/**
	 * �z��^�C�v��JVM�����̌`�����畁�ʂ̌`���ɕϊ�
	 * 
	 * @param internalType JVM�����`���̔z��^�C�v
	 * @return ���ʌ`���̔z��^�C�v
	 */
	static char *getArrayType(const char *internalType);

	/**
	 * Java�̊�{�^���ǂ����𔻒f���� 
	 */
	static bool isBasicType(const char descriptor);
public:

	/**
	 * �p�����[�^�uerr�v���`�F�b�N���A���ꂪ�G���[�ł���ꍇ�A���O�ɏo��
	 *
	 * @param jvmti jvmti���������I�u�W�F�N�g
	 * @param err �`�F�b�N�ΏۂƂȂ�jvmtiError�^�C�v�̃G���[
	 * @param str �uerr�v���G���[�ł���ꍇ�A�o�͂��������b�Z�[�W
	 * @return <ul>
	 *			<li>true: �G���[����</li>
	 *			<li>false: �G���[�Ȃ�</li>
	 *		   </ul>
	 */
	static bool validateJvmtiError(jvmtiEnv *jvmti, jvmtiError err, const char *str);

	/**
	 * DOM�G�������g�Ƀp�����[�^�uname�v�Ɓuvalue�v�Ɏ�����鑮����ǉ�
	 *
	 * @param elem �ǉ���ƂȂ�DOM�G�������g
	 * @param name ������
	 * @param value �����̒l
	 */
	static void addAttrToDOMElement(DOMElement *elem, const char *name, const char *value);

	/**
	 * String�^�C�v�̃I�u�W�F�N�g�̒l���擾
	 * 
	 * @param jni JNI���������I�u�W�F�N�g
	 * @param object �����ΏۂƂȂ�I�u�W�F�N�g
	 * @param value String�^�C�v�̃I�u�W�F�N�g�̒l�B���̃p�����[�^�����̊֐��O�Ő��������B
	 * @param len �p�����[�^�uvalue�v�̒���
	 */
	static void getStringObjectValue(JNIEnv *jni, jobject object, char* value, int len);

	static void getJStringValue(JNIEnv *jni, jstring strValue, char *value, int len);
	static char* getJStringValue(JNIEnv *jni, jstring strValue);
	/**
	 * �n���ꂽ���\�b�h�̃V�O�l�`���ɂ���ē��Y���\�b�h�̃p�����[�^�̐����擾
	 * 
	 * @param methodSig ���\�b�h�̃V�O�l�`��
	 * @return ���\�b�h�E�p�����[�^�̐�
	 */
	static int getMethodParamNum(const char *methodSig);

    static void getMethodParamType(char * methodSig,int paramNum,bool isStaticMethod,
									char* varTypes);
	/**
	 * �N���X�̃V�O�l�`����JVM�����p�̌`�����畁�ʂ̌`���ɕϊ�
	 * 
	 * @param internalType JVM�����`���̃N���X�E�V�O�l�`��
	 * @return ���ʌ`���̃N���X�E�V�O�l�`��
	 */
	static char *convertClassSig(const char *internalType);

	/**
	 * ���\�b�h�̃V�O�l�`����JVM�����p�̌`�����畁�ʂ̌`���ɕϊ�
	 * 
	 * @param methodSig JVM�����`���̃��\�b�h�E�V�O�l�`��
	 * @return ���ʌ`���̃��\�b�h�E�V�O�l�`��
	 */
	static char *convertMethodSig(const char *methodSig);

	/**
	 * �p�����[�^entryTable��location�ɑΉ�����s�ԍ�������
	 * 
	 * @param entryTable �s�ԍ������܂ޔz��
	 * @param entryCount �s�ԍ����z��̃G���g���E�T�C�Y
	 * @param location �����ΏۂƂȂ�s�̈ʒu
	 * @return �������ʂƂȂ�s�ԍ�
	 */
	static int getLineNumberByLocation(jvmtiLineNumberEntry *entryTable, 
		jint entryCount, jlocation location);

	/**
	 * �p�����[�^attributes�ɑ�������attrName�ɑΉ����鑮���l���擾
	 * 
	 * @return attrName�ɑΉ����鑮���l�iUTF-8�R�[�h�j
	 */
	static char* getAttrValueUtf8(DOMNamedNodeMap *attributes, const char* attrName);

	/**
	 * �w�肵���f�B���N�g���������݂��邩�𔻒f����
	 */
	static bool isValidDir(char* dirName);
	static bool isValidFile(const char* fileName);
    static bool makeDirectory(char *dirName);
	static int utf16be_to_utf8_sub( char *pUtf8, const unsigned short *pUcs2, int nUcsNum, bool bCountOnly);
    static char *utf16be_to_utf8(const jchar utf16char, int *nBytesOut);
	static void getLongTypeValue(jlong value,char*buf);
	static char* strdup(const char* data);
    static char* replaceChar(const char* data,char oldChar,char newChar);
	static long  getFileSize(char* fileFullPath);
	static bool  renameFile(char* oldName,char* newName);
	static void  mySleep(int seconds);
	static bool  containInvalidFileNameChar(char* name);
	static void  convertInvalidFileNameChar(char* name);
    static char* utf8_to_native(const char *pUtf8Str);
	static bool  isBigEndian();
	static bool  hasEnoughFreeDiskSpace();
	static bool  removeFile(const char* fileName);
	static unsigned long  getCurrentThreadId();

	//add by Qiu Song on 20090819 for Excat Version 3.0
	//�X���b�h���(int��������)���擾����
	static char* getThreadStatusString(jint threadStatus);
    //�X���b�h�҂����R���擾����
	static char* getThreadWaitingReason(jint threadStatus);
	//�J�����g�X���b�h�͎w�肳�ꂽ�^�C�v���ǂ������f����
	static bool shouldDumpCurrentThread(jvmtiEnv* jvmti, jthread curThread, char* threadStatus, int threadPriority);
	
	//�J�����g�X���b�h�̏�Ԃ͎w�肳�ꂽ�ƈ�v���邩�ǂ���
	static bool isCurrentThreadStatusMatch(jvmtiEnv* jvmti, jthread curThread, char* threadStatus);

	//�J�����g�X���b�h�̗D��x�͎w�肳�ꂽ�ƈ�v���邩�ǂ���
	static bool isCurrentThreadPriorityMatch(jvmtiEnv* jvmti, jthread curThread, int threadPriority);

	//�w��^�C�v�̃I�u�W�F�N�g�_���v�ғ������f����
	static bool shouldDumpCurrentObject(jvmtiEnv* jvmti, JNIEnv *jni, jobject curObject);
	//�N���X�͎w�肳�ꂽ�_���v�^�C�v���ǂ������f����
	static bool isFilterObjType(char *javaClassName);
	//���\�b�hsig���烊�^�[���^�C�v���擾����
	static char* getReturnTypeFromMethodSig(const char *methodSig);
	//���O��O�ꗗ�t�@�C���̃p�X���擾����
	static char* getExcludeExceptionListFilePath();
	//�unull�v��string��߂�
	static char* getNullString();
	//�w�蕶����Ɂu*�v�����邩�ǂ���
	static bool hasWildCode(const char* stringBuf);
	//�����񂪎w�肳�ꂽ�̂ƃ}�b�`���ǂ������f����
	static bool doesStringMatch(const char* curMethodName, const char* monitorMethodName);
	//�����񂪎w�肳�ꂽ�̂ƃ}�b�`���ǂ������f����(wild code���܂߂�)
	static bool doesStringMatchWithWildCode(const char* curMethodName, const char* monitorMethodName);
	//wild code�O�ƌ�̕�������擾����
	static void getStringBeforeAndAfterWildCode(const char* stringBuf, char* strBeforeWildCode, char* strAfterWildCode);
	//�w��ʒu�̑O�ƌ�̕�������擾����
	static void getStringBeforeAndAfterPosition(const char* stringBuf, int nPos, char* stringBufLeft, char* stringBufRight);
	//�����񂪎w�肳�ꂽ�̂ƃ}�b�`���ǂ������f����(�擪������wild code�̏ꍇ)
	static bool doesStringMatchWithFirstWildCode(const char* stringBuf, const char* matchString, char* stringBufRight);
	//�����񂪎w�肳�ꂽ�̂ƃ}�b�`���ǂ������f����(�擪������wild code�ȊO�̏ꍇ)
	static bool doesStringMatchWithFirstNotWildCode(const char* stringBuf, const char* matchString,
													char* strBeforeWildCode, char* strAfterWildCode,
													char* stringBufLeft, char* stringBufRight);
	//wild code�w��͗L�����ǂ���
	static bool isWildCodeCorrect(const char*);

	//boolean�̖߂�l��true�܂���false�ɕύX
	static void convertBoolValue(char* type, char* value);

	//char�̖߂�l��\���ł���悤�ɕύX
	static void convertCharValue(char* type, char* value);
	
	//jobject�̃N���X�����擾����
	static char* getClassNameFromJobject(jvmtiEnv* jvmti, JNIEnv *jni, jobject curObject);

	//���\�b�h�̃N���X�����擾����
	static char* getMethodDeclaringClassName(jvmtiEnv* jvmti, JNIEnv *jni, jmethodID methodId);

	//�N���X������p�b�P�[�W�����擾����
	static char* getPackageNameFromClassName(char* className);

	//OS��MAC�A�h���X�̃`�F�b�N�֐�
	static bool CheckMacAddress(char* mac);

	//�e�t�H���_�����擾����
	static char* getPreForldName(char* strPathName);

	//�w�蕶����ɁA�Ō�̎w�蕶����T��
	static int findLastCharOf(char* strPathName, char strChar);

	//�t�H���_���폜����֐�
    static int removeDir(char* dirPath);

	//�w��t�H���_�܂ŁA�󂭃t�H���_���폜����֐�
    static int removeDirUntil(char* dirPathStart, char* dirPathEnd);

	//�_���v�t�@�C�����폜����
    static void removeDumpFile(char* filePath);

#ifdef _WINDOWS
	//Windows�V�X�e����MAC�A�h���X�̃`�F�b�N�֐�
	static bool CheckMacAddressForWindows(char* mac);
#endif

#ifdef _LINUX
	//Linux�V�X�e����MAC�A�h���X���擾����֐�
	static bool CheckMacAddressForLinux(char* mac);
#endif
	//end of add by Qiu Song on 20090819 for Excat Version 3.0

//add by Qiu Song on 20091120 for Solaris��HP-UX��Mac�A�h���X���擾����֐�
#ifdef _EXCAT_SOLARIS
	//Linux�V�X�e����MAC�A�h���X���擾����֐�
	static bool CheckMacAddressForSolaris(char* mac);
#endif

#ifdef _EXCAT_HPUX
	//Linux�V�X�e����MAC�A�h���X���擾����֐�
	static bool CheckMacAddressForHPUX(char* mac);
#endif
//end of //add by Qiu Song on 20091120 for Solaris��HP-UX��Mac�A�h���X���擾����֐�
private:
	static int utf8_to_utf16be_sub(unsigned short *pUcs2, const char *pUtf8,int nUtf8Num, 
									bool bCountOnly, bool bBigEndian);
    static unsigned short* utf8_to_utf16be(const char *pUtf8Str, int *nNumOut, bool bBigEndian);
};
END_NS

#endif  //_HELPERFUNC_H
