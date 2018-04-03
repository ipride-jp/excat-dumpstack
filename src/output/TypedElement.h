#if !defined(_TYPEDELEMENT_H)
#define _TYPEDELEMENT_H

#include "jni.h"
#include "jvmti.h"
#include "ObjectData.h"
#include "ObjectPool.h"
#include "StackTrace.h"


BEGIN_NS(NS_OUTPUT)


class TypedElement  
{
public:
	TypedElement(TypedElementParam *param, const char *tagName);
	virtual ~TypedElement();

private:
	TypedElementParam *param;
	const char*tagName;
	static const char *ATTR_DEFINED_TYPE;
	static const char *ATTR_REAL_TYPE;
	static const char *ATTR_NAME;
	static const char *ATTR_VALUE;
	static const char *ATTR_ARRINDEX;
	static const char *ATTR_VALID;
	static const char *ATTR_OBJECT_REF;

public:
	static const char *TAG_VARIABLE;
	static const char *TAG_ATTR;
	static const char *TAG_THIS;
	static const char *TAG_ITEM;
	static const char *TAG_ARGUMENT;

private:
	void init();

};
END_NS

#endif  //_TYPEDELEMENT_H
