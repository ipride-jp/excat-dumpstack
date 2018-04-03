#include <jni.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include "JvmUtilFunc.h"
#include "HelperFunc.h"
#include "Global.h"
#include "Define.h"

USE_NS(NS_COMMON)

//常用のタイプかどうか判断する(boolean,char,int,byte,short,long,float,double)
bool JvmUtilFunc::isPrimitiveType(const char *type)
{
	if (type == NULL)
		return false;

	const char *primitive = "ZCIBSJFD";
	if (strlen(type) == 1 && strstr(primitive, type) != NULL) 
		return true;

	return false;
}

jvmtiError JvmUtilFunc::myGetLocalObject(jvmtiEnv *jvmti,jthread thread,
            jint depth,jint slot,jobject* value_ptr){

    jvmtiError err;

	err = jvmti->GetLocalObject(thread,depth,slot,value_ptr);

	return err;
}

jvmtiError JvmUtilFunc::myGetLocalInt(jvmtiEnv *jvmti,jthread thread,
            jint depth,jint slot,jint* value_ptr){

    jvmtiError err;
	err = jvmti->GetLocalInt(thread,depth,slot,value_ptr);
	return err;
}


jvmtiError JvmUtilFunc::myGetLocalLong(jvmtiEnv *jvmti,jthread thread,
            jint depth,jint slot,jlong* value_ptr){

    jvmtiError err;
	err = jvmti->GetLocalLong(thread,depth,slot,value_ptr);
	return err;
}

jvmtiError JvmUtilFunc::myGetLocalFloat(jvmtiEnv *jvmti,jthread thread,
            jint depth,jint slot,jfloat* value_ptr){

    jvmtiError err;
	err = jvmti->GetLocalFloat(thread,depth,slot,value_ptr);

	return err;
}

jvmtiError JvmUtilFunc::myGetLocalDouble(jvmtiEnv *jvmti,jthread thread,
            jint depth,jint slot,jdouble* value_ptr){

    jvmtiError err;
 	err = jvmti->GetLocalDouble(thread,depth,slot,value_ptr);
	return err;
}

char* JvmUtilFunc::getPrimitiveLocalVariableValue(jvmtiEnv *jvmti,jthread thread, jint depth, 
												  jint slot, char type, jvmtiError* pErrorCode)
{
	char buf[MAX_BUF_LEN];
	memset(buf, 0, MAX_BUF_LEN);
	if (type == 'Z')  //boolean
	{
		jint value;
		*pErrorCode = myGetLocalInt(jvmti,thread, depth, slot, &value);
		sprintf(buf, "%s", value == 0 ? "false" : "true");
	}
	else if (type == 'C') //char
	{
		jint value;
		*pErrorCode = myGetLocalInt(jvmti,thread, depth, slot, &value);
		sprintf(buf, "%04x", value);
	}
	else if ((type == 'I') //int
		|| (type == 'B') //byte
		|| (type == 'S')) //short
	{
		jint value;
		*pErrorCode = myGetLocalInt(jvmti,thread, depth, slot, &value);
		sprintf(buf, "%d", value);
	} 
	else if (type == 'J') //long
	{
		jlong value;
		*pErrorCode = myGetLocalLong(jvmti,thread, depth, slot, &value);
		HelperFunc::getLongTypeValue(value,buf);
	}
	else if (type == 'F') //float
	{
		jfloat value;
		*pErrorCode = myGetLocalFloat(jvmti,thread, depth, slot, &value);
		sprintf(buf, "%e", value);
	}
	else if (type == 'D') //double
	{
		jdouble value;
		*pErrorCode = myGetLocalDouble(jvmti,thread, depth, slot, &value);
		sprintf(buf, "%e", value);
	}
	char *result = new char[strlen(buf) + 1];
	strcpy(result, buf);

	return result;
}
