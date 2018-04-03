#if !defined(_OBJECTAUTORELEASE_H)
#define _OBJECTAUTORELEASE_H

#include "Define.h"

BEGIN_NS(NS_COMMON)

class ObjectAutoRelease  
{
public:
	ObjectAutoRelease(void *object);
	ObjectAutoRelease(void *object, bool isArray);
	virtual ~ObjectAutoRelease();

private:
	void *object;
	bool isArray;
};
END_NS

#endif 

