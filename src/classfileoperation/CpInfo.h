#if !defined(_CPINFO_H)
#define _CPINFO_H

#include "BaseInfo.h"

BEGIN_NS(NS_CLASS_FILE_OPERATION)
class CpInfo : public BaseInfo
{
public:
	CpInfo(U1 *pos);
	virtual ~CpInfo() {};

	void getUtf8Info(char *buf);
	void getUtf8InfoNeedMemory(char **buf);		
	bool isLargeNumericType(){return largeNumericType;};

public:
	U1 *tag;
	U1 *info; //ˆÊ’u‚ð•Û‘¶‚·‚é‚¾‚¯

private:
	bool largeNumericType;
};

typedef CpInfo*	CpInfoPtr;
END_NS

#endif  //_CPINFO_H
