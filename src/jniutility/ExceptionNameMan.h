// ExceptionNameMan.h: ExceptionNameMan �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EXCEPTIONNAMEMAN_H__EC14EE1B_F087_4C64_B109_4CB39BAEB830__INCLUDED_)
#define AFX_EXCEPTIONNAMEMAN_H__EC14EE1B_F087_4C64_B109_4CB39BAEB830__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


using namespace std;

BEGIN_NS(NS_JNI_UTILITY)

/**
����������O�N���X����AThrowable�܂ł̃X�[�p�[�N���X�����擾���āA
���g����Throwable�܂ł̗�O�N���X��Catch����邩�ǂ������f����B
*/
class ExceptionNameMan  
{
private:
	//expction name and super class's name
	vector<string*>* names;
    jvmtiEnv *jvmti;
	JNIEnv* jni;
    jthread thread;
    jclass exceptionClazz;
    const char* exceptionClassSig;
	const char* newExceptionClassSig;
	jlocation location;
    bool hasError;
	class ExceptionTableMan* pExceptionTableMan;
	//Catch���b�\�b�h��������܂ŁA�Ď��Ώۃp�b�P�[�W�ɓ��B���邱�Ƃ�\���t���O
    bool reachMonitorPackage;
public:
	ExceptionNameMan(jvmtiEnv *_jvmti,JNIEnv* _jni,jthread _thread,
		jclass _exceptionClazz,const char* _exceptionClassSig,const char* _newExceptionClassSig,jlocation _location);
	virtual ~ExceptionNameMan();

	//�Y��Exception�̏����_���v���ׂ����ǂ����𔻒f����B
	bool shouldDump(char* catchClassSig);

    void setExceptionTableMan(ExceptionTableMan* param){pExceptionTableMan = param;};
private:
	//catch method�̌���
	jmethodID seachCatchMethod(char* catchClassSig);

    //Catch���\�b�h�ł��邩���邢�́A�Ď��Ώۃp�b�P�[�W�܂œ��B�������ǂ������f
    bool IsCathcMethodOrReachTarget(jmethodID methodId,jlocation _location, char* catchClassSig);
};

END_NS

#endif // !defined(AFX_EXCEPTIONNAMEMAN_H__EC14EE1B_F087_4C64_B109_4CB39BAEB830__INCLUDED_)
