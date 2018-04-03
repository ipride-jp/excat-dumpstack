#if !defined(_THISOBJECT_H)
#define _THISOBJECT_H

#include "jni.h"
#include "jvmti.h"
#include "xercesc/dom/DOM.hpp"
#include "DumpParam.h"

#ifndef XERCESC20
XERCES_CPP_NAMESPACE_USE
#endif

BEGIN_NS(NS_OUTPUT)
class ThisObject  
{
public:
	ThisObject(DumpParam *param);
	virtual ~ThisObject();

private:
	void init();

	DumpParam *param;
	static const char *TAG_THIS;
	static const char *ATTR_DEFINED_TYPE;
	static const char *ATTR_REAL_TYPE;
};
END_NS


#endif  //_THISOBJECT_H
