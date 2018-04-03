#pragma once

#if !defined(_FRAME_H)
#define _FRAME_H

#include "jni.h"
#include "jvmti.h"
#include "xercesc/dom/DOM.hpp"
#include "DumpParam.h"

#ifndef XERCESC20
XERCES_CPP_NAMESPACE_USE
#endif

BEGIN_NS(NS_OUTPUT)
class Frame  
{
private:
	DumpParam *param;
public:
	Frame(DumpParam *param);
	virtual ~Frame();

private:
	void init();

private:
	static const char *ATTR_DEPTH;
	static const char *TAG_FRAME;
};
END_NS

#endif  //_FRAME_H