// ExcludeClass.h: ExcludeClass �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EXCLUDECLASS_H__869C08DF_B3DC_4ABD_BC38_800E3A0F6AEF__INCLUDED_)
#define AFX_EXCLUDECLASS_H__869C08DF_B3DC_4ABD_BC38_800E3A0F6AEF__INCLUDED_

#include "MonitoringTarget.h"

using namespace std;
#ifndef XERCESC20
XERCES_CPP_NAMESPACE_USE
#endif

BEGIN_NS(NS_COMMON)

class ExcludeClass  : public MonitoringTarget 
{
public:
	ExcludeClass();
	virtual ~ExcludeClass();
	int operator==(const ExcludeClass &rhs) const;
	int operator<(const ExcludeClass &rhs) const;
	void init(DOMElement *node);
	char* getExcludeClassName(){return excludeClass;};
	void logConfig();
	bool hasPlaceInfo();

	bool shouldExclude(const char *classSig, 
		const char *methodName, const char *methodSig);
	//add by Qiu Song on 20090821 for �Ď����~�^�X�N
	bool getExcludeClassValid(){return bExcludeClassValid;};
	//end of add by Qiu Song on 20090821 for �Ď����~�^�X�N
private:
	char *excludeClass;

    static const char *TAG_EXCLUDEPLACE;

	//add by Qiu Song on 20090821 for �Ď����~�^�X�N
	bool bExcludeClassValid;
	//end of add by Qiu Song on 20090821 for �Ď����~�^�X�N
};

inline int ExcludeClass::operator<(const ExcludeClass &rhs) const
{
	//compare throwable class
	int result = strcmp(this->excludeClass, rhs.excludeClass);
	return result < 0;
};

END_NS

#endif // !defined(AFX_EXCLUDECLASS_H__869C08DF_B3DC_4ABD_BC38_800E3A0F6AEF__INCLUDED_)
