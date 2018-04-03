#if !defined(_JVMUTILFUNC_H)
#define _JVMUTILFUNC_H

#include <jvmti.h>
#include "Define.h"

BEGIN_NS(NS_COMMON)

/**
 * このクラスでは、様々なヘルパー関数を提供する。
 */
class JvmUtilFunc  
{
private:
	/**
	 * コンストラクタ[禁止]
	 */
	JvmUtilFunc() {};

	/**
	 * デストラクタ[禁止]
	 */
	virtual ~JvmUtilFunc() {};
public:
    static jvmtiError myGetLocalObject(jvmtiEnv *jvmti,jthread thread,
            jint depth,jint slot,jobject* value_ptr);
    static jvmtiError myGetLocalInt(jvmtiEnv *jvmti,jthread thread,
            jint depth, jint slot,jint* value_ptr);
    static jvmtiError myGetLocalDouble(jvmtiEnv *jvmti,jthread thread,
            jint depth,jint slot,jdouble* value_ptr);
    static jvmtiError myGetLocalFloat(jvmtiEnv *jvmti,jthread thread,
		    jint depth,jint slot,jfloat* value_ptr);
    static jvmtiError myGetLocalLong(jvmtiEnv *jvmti,jthread thread,
            jint depth,jint slot,jlong* value_ptr);


	static bool  isPrimitiveType(const char *type);
	static char* getPrimitiveLocalVariableValue(jvmtiEnv *jvmti,jthread thread, 
												jint depth, jint slot, char type,
												jvmtiError* err);

};
END_NS

#endif  //_JVMUTILFUNC_H
