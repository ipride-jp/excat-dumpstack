#pragma warning(disable : 4503)
#pragma warning(disable : 4786)

// ExceptionTableMan.cpp: ExceptionTableMan クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////
#include <map>
#include <vector>
#include <string>

#include "../common/Define.h"
#include "ExceptionTableMan.h"

USE_NS(NS_JNI_UTILITY)

//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

ExceptionTableMan::ExceptionTableMan()
{
	exceptionTabMap = new map<string,vector<ExceptionInfo*>*>();
}


ExceptionTableMan::~ExceptionTableMan()
{
     for (ExceptionTabMap::const_iterator iter = exceptionTabMap->begin(); iter != exceptionTabMap->end(); ++iter)
	 {
         vector<ExceptionInfo*>* pExpVect = iter->second;
		 //clear this vector
		 vector<ExceptionInfo*>::iterator it;
	 	 for(it = pExpVect->begin(); it != pExpVect->end(); it++ )
		 {
			delete *it;
         }  
		 pExpVect->clear();
		 delete pExpVect;
     }
     
	 exceptionTabMap->clear();
     delete exceptionTabMap;
}

//exception tableの登録
void ExceptionTableMan::registerExceptionTable(int exceptionNum,ExceptionInfo** eceptions,char* className,
		char* methodName,char* methodSignature)
{
    if(exceptionNum <= 0)
	{
		return;
	}

	string key = className;
    key += ':';
    key += methodName;
	key += ':';
	key += methodSignature;

    vector<ExceptionInfo*>* pVector = new vector<ExceptionInfo*>();
	for(int i= 0; i < exceptionNum;i++)
	{
        ExceptionInfo* pException = new ExceptionInfo(eceptions[i]);
        pVector->push_back(pException);
	}

	exceptionTabMap->insert(pair<string,vector<ExceptionInfo*>*>(key, pVector));
}

//important:
//classNameの前後にLと;があります
bool ExceptionTableMan::catchAndHandleException(
		char* className,char* methodName,char* methodSignature,
		int location,vector<string*>* names)
{
	string temp = className;
	//remove L;
    string key = temp.substr(1,strlen(className)-2);
    key += ':';
    key += methodName;
	key += ':';
	key += methodSignature;

    map<string,vector<ExceptionInfo*>*>::const_iterator iter  = exceptionTabMap->find(key);
	if(iter == exceptionTabMap->end()){
		return false;
	}
	vector<ExceptionInfo*>* pExceptionInfoVect = iter->second;

	vector<string*>::iterator itv;
	for(itv = names->begin(); itv != names->end(); itv++ )
	{
		string* excepName = *itv;
		string excepNameTemo = excepName->substr(1,excepName->size()-2);//remove L;

        vector<ExceptionInfo*>::iterator ite;
		for(ite = pExceptionInfoVect->begin(); ite != pExceptionInfoVect->end();ite++){
            ExceptionInfo* exceptionInfo = *ite;  
			if(exceptionInfo->isHandledException(excepNameTemo.c_str(),location)){
				return true;
			}
		}
    }  

	return false;

}