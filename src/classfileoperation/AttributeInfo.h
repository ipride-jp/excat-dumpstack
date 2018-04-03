#if !defined(_ATTRIBUTEINFO_H)
#define _ATTRIBUTEINFO_H

#include "BaseInfo.h"

BEGIN_NS(NS_CLASS_FILE_OPERATION)
class AttributeInfo : public BaseInfo
{
public:
	AttributeInfo(U1 *pos) ;
	~AttributeInfo() {};

public:
	U2 *attributeNameIndex;
	U4 *attributeLength;
	U1 *info; //à íuèÓïÒÇï€ë∂ÇæÇØ
};

typedef AttributeInfo* AttributeInfoPtr;
END_NS

#endif  //_ATTRIBUTEINFO_H
