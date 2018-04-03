#if !defined(_BASEINFO_H)
#define _BASEINFO_H

#include "../common/Define.h"

BEGIN_NS(NS_CLASS_FILE_OPERATION)
typedef unsigned char U1;
typedef unsigned short U2;
#ifdef _BIT64
typedef unsigned int U4;
#else
typedef unsigned long U4;
#endif
class BaseInfo
{
public:
	BaseInfo(void);
	virtual ~BaseInfo(void);

protected:
	int size;

public:
	virtual int getSize() {return size;};
};
END_NS

#endif  //_BASEINFO_H
