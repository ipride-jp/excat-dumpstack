// ExceptionTableMan.h: ExceptionTableMan クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EXCEPTIONTABLEMAN_H__095FFBAD_F5EE_4B65_BDE1_637181B91BB3__INCLUDED_)
#define AFX_EXCEPTIONTABLEMAN_H__095FFBAD_F5EE_4B65_BDE1_637181B91BB3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../classfileoperation/ExceptionInfo.h"

using namespace std;
USE_NS(NS_CLASS_FILE_OPERATION)

BEGIN_NS(NS_JNI_UTILITY)

class ExceptionTableMan  
{
private:
	map<string,vector<ExceptionInfo*>*> *exceptionTabMap;
public:
	ExceptionTableMan();
	virtual ~ExceptionTableMan();

	void registerExceptionTable(int exceptionNum,ExceptionInfo** eceptions,char* className,
		char* methodName,char* methodSignature);
	bool catchAndHandleException(char* className,char* methodName,char* methodSignature,
		int location,vector<string*>* names);
};

typedef map<string,vector<ExceptionInfo*>*> ExceptionTabMap;
END_NS

#endif // !defined(AFX_EXCEPTIONTABLEMAN_H__095FFBAD_F5EE_4B65_BDE1_637181B91BB3__INCLUDED_)
