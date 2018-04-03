#include "ObjectAutoRelease.h"

USE_NS(NS_COMMON)

ObjectAutoRelease::ObjectAutoRelease(void *object)
{
	this->object = object;
	this->isArray = false;
}

ObjectAutoRelease::ObjectAutoRelease(void *object, bool isArray)
{
	this->object = object;
	this->isArray = isArray;

}

ObjectAutoRelease::~ObjectAutoRelease()
{
	if (object != 0)
	{
		if (isArray)
		{
             delete [] object;
		}
		else
		{
		    delete object;
		}
		object = 0;
	}
}
