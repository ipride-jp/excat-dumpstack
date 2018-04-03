#if !defined(_FIELDINFO_H)
#define _FIELDINFO_H

#include "BaseInfo.h"

BEGIN_NS(NS_CLASS_FILE_OPERATION)
class AttributeInfo;

class FieldInfo : public BaseInfo
{
public:
	FieldInfo(U1 *pos);
	~FieldInfo();

private:
	U2 *accessFlags;
	U2 *nameIndex;
	U2 *descriptorIndex;
	U2 *attributesCount;
	AttributeInfo **attributes;
};

typedef FieldInfo* FieldInfoPtr;
END_NS

#endif //_FIELDINFO_H
