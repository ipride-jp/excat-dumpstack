#include "JvmtiAutoRelease.h"
#include "jvmti.h"
#include "Global.h"

USE_NS(NS_COMMON)

JvmtiAutoRelease::JvmtiAutoRelease(void *object)
{
	this->object = object;
}

JvmtiAutoRelease::~JvmtiAutoRelease()
{
	if (object != NULL)
		Global::jvmti->Deallocate((unsigned char *)object);
}
