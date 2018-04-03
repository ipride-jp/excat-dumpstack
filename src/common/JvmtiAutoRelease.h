#if !defined(_JVMTIAUTORELEASE_H)
#define _JVMTIAUTORELEASE_H

#include "Define.h"

BEGIN_NS(NS_COMMON)
class JvmtiAutoRelease  
{
public:
	JvmtiAutoRelease(void *object);
	virtual ~JvmtiAutoRelease();

private:
	void *object;
};
END_NS
#endif
