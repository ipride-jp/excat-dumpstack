// ExceptionNameMan.h: ExceptionNameMan クラスのインターフェイス
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
発生した例外クラスから、Throwableまでのスーパークラス名を取得して、
自身からThrowableまでの例外クラスがCatchされるかどうか判断する。
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
	//Catchメッソッドを見つけるまで、監視対象パッケージに到達することを表すフラグ
    bool reachMonitorPackage;
public:
	ExceptionNameMan(jvmtiEnv *_jvmti,JNIEnv* _jni,jthread _thread,
		jclass _exceptionClazz,const char* _exceptionClassSig,const char* _newExceptionClassSig,jlocation _location);
	virtual ~ExceptionNameMan();

	//該当Exceptionの情報をダンプすべきかどうかを判断する。
	bool shouldDump(char* catchClassSig);

    void setExceptionTableMan(ExceptionTableMan* param){pExceptionTableMan = param;};
private:
	//catch methodの検索
	jmethodID seachCatchMethod(char* catchClassSig);

    //Catchメソッドであるかあるいは、監視対象パッケージまで到達したかどうか判断
    bool IsCathcMethodOrReachTarget(jmethodID methodId,jlocation _location, char* catchClassSig);
};

END_NS

#endif // !defined(AFX_EXCEPTIONNAMEMAN_H__EC14EE1B_F087_4C64_B109_4CB39BAEB830__INCLUDED_)
