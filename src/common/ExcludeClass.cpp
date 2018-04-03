// ExcludeClass.cpp: ExcludeClass クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////
#include "HelperFunc.h"
#include "ExcludeClass.h"
#include "Global.h"

USE_NS(NS_COMMON)

const char *ExcludeClass::TAG_EXCLUDEPLACE = "ExcludePlace";
//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

ExcludeClass::ExcludeClass(): excludeClass(NULL),bExcludeClassValid(true)
{
}

ExcludeClass::~ExcludeClass()
{
	if (excludeClass != NULL)
	{
		delete[] excludeClass;
		excludeClass = NULL;
	}
}

//get eclude class information
void ExcludeClass::init(DOMElement *node)
{
    //get attribute ExcludeClass
	DOMNamedNodeMap *attributes = node->getAttributes();
	char *value = HelperFunc::getAttrValueUtf8(attributes, ATTR_EXCLUDE_CLASS);
    //value will not be null because of schema check
	excludeClass = value;

	//add by Qiu Song on 20090821 for 監視中止タスク
	char* targetValue = HelperFunc::getAttrValueUtf8(attributes, ATTR_VALID);
	if(targetValue != NULL && strcmp(targetValue, "false") == 0)
	{
		bExcludeClassValid = false;
		return;
	}
	//end of add by Qiu Song on 20090821 for 監視中止タスク
	//Exclude Place関連の属性を取得
	XMLCh *xmlTemp;
	DOMNodeList *excludePlaceNodes = node->getElementsByTagName(
		xmlTemp = XMLString::transcode(TAG_EXCLUDEPLACE));
	XMLString::release(&xmlTemp);
    getPlaceInfo(excludePlaceNodes);
}

int ExcludeClass::operator==(const ExcludeClass &rhs) const
{
	//compare excludeClass class 
	bool equal =  strcmp(this->excludeClass, rhs.excludeClass) == 0;
	if(!equal)
	{
       return equal;
	}

	int placeNumber = this->placeVector->size();
	int rhsPlaceNumber = rhs.placeVector->size();
	if(placeNumber != rhsPlaceNumber)
	{
		return false;
	}
	int i;
	for(i = 0; i<placeNumber;i++)
	{
		char* placeClass = (char*)this->placeVector->at(i);
		char* rhsPlaceClass = (char*)rhs.placeVector->at(i);
		if(strcmp(placeClass,rhsPlaceClass) != 0)
		{
			return false;
		}

		char* placeMethod = (char*)this->methodNameVector->at(i);
        char* rhsPlaceMethod = (char*)rhs.methodNameVector->at(i);
		if(strcmp(placeMethod,rhsPlaceMethod) != 0)
		{
			return false;
		}

		char* placeMethodSig = (char*)this->methodSignatureVector->at(i);
		char* rhsPlaceMethodSig = (char*)rhs.methodSignatureVector->at(i);
		if(strcmp(placeMethodSig,rhsPlaceMethodSig) != 0)
		{
			return false;
		}
	}

    return true;
}

bool ExcludeClass::hasPlaceInfo()
{
    return (placeVector->size() > 0); 
}

bool ExcludeClass::shouldExclude(const char *classSig, 
		const char *methodName, const char *methodSig)
{
	int placeNumber = placeVector->size();
	if(placeNumber == 0)
	{
		//no place tag and this means at any place
		return true;
	}

	bool match = false;
	for(int i = 0; i< placeNumber;i++)
	{
		char* placeClass = placeVector->at(i);
		char* placeMethod = methodNameVector->at(i);
		char* placeMethodSig = methodSignatureVector->at(i);

		if (!(strcmp(placeClass, "") == 0 ||
			strncmp(placeClass, classSig, strlen(placeClass)) == 0))
			continue;

		if (!(strcmp(placeMethod, "") == 0 ||
			strncmp(placeMethod, methodName, strlen(placeMethod)) == 0))
			continue;

		//for method signature,you should write all of them
		if (!(strcmp(placeMethodSig, "") == 0 ||
			strcmp(placeMethodSig, methodSig) == 0))
			continue;
        
		match = true;
		break;
	}

	return match;
}

//you should call this function after config file is read
void ExcludeClass::logConfig()
{
	string buf;
	buf = "Exclude the Class: ";
	buf += excludeClass;
	LOG4CXX_INFO(Global::logger, buf.c_str());

	int placeNumber = placeVector->size();
	for(int i = 0; i< placeNumber;i++)
	{
		char* placeClass = placeVector->at(i);
		buf = "Package where the excluded class happened: ";
		buf += placeClass;
		LOG4CXX_INFO(Global::logger, buf.c_str());

		char* placeMethod = methodNameVector->at(i);
		buf = "Method Name  where the excluded class happened: ";
		buf += placeMethod;
		LOG4CXX_INFO(Global::logger, buf.c_str());

		char* placeMethodSig = methodSignatureVector->at(i);
		buf = "Method Signature where the excluded class happened: ";
		buf += placeMethodSig;
		LOG4CXX_INFO(Global::logger, buf.c_str());
	}

}
