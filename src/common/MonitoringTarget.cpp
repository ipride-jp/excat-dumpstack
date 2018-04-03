#include "HelperFunc.h"
#include "MonitoringTarget.h"
#include "ExcludeClass.h"
#include "Global.h"


USE_NS(NS_COMMON)

const char *MonitoringTarget::TAG_PLACE = "Place";
const char *MonitoringTarget::TAG_THROWABLE = "Throwable";
const char *MonitoringTarget::TAG_FILTERS = "Filters";
const char *MonitoringTarget::TAG_FILTER = "Filter";
const char *MonitoringTarget::ATTR_PLACE_CLASS = "Class";
const char *MonitoringTarget::ATTR_PLACE_METHOD = "MethodName";
const char *MonitoringTarget::ATTR_PLACE_METHOD_SIG = "MethodSignature";
const char *MonitoringTarget::ATTR_THROWABLE_CLASS = "Class";
const char *MonitoringTarget::ATTR_EXCLUDE_CLASS = "ExcludeClass";
const char *MonitoringTarget::ATTR_VALID = "Valid";//add by Qiu Song on 20090821 for 監視中止タスク

MonitoringTarget::MonitoringTarget() : throwableClass(NULL)
{
	placeVector = new vector<char*>;
	methodNameVector = new vector<char*>;
	methodSignatureVector = new vector<char*>;
	excludeClassVector = new vector<ExcludeClass*>;

	//add by Qiu Song on 20090821 for 監視中止タスク
	bThrowableClassValid = true;
	//end of add by Qiu Song on 20090821 for 監視中止タスク
	//add by Qiu Song on 20091009 for 自動監視機能のパッケージ重複指定
	bAutoMonitorMode = false;
	//end of add by Qiu Song on 20091009 for 自動監視機能のパッケージ重複指定
}

MonitoringTarget::~MonitoringTarget()
{
	if (throwableClass != NULL)
	{
		delete throwableClass;
		throwableClass = NULL;
	}

	if (placeVector != NULL)
	{
		vector<char*>::iterator it;
		for(it = placeVector->begin(); it != placeVector->end(); it++ )
		{
			delete *it;
        }  
		placeVector->clear();
		delete placeVector;
		placeVector = NULL;
	}

	if (methodNameVector != NULL)
	{
		vector<char*>::iterator it;
		for(it = methodNameVector->begin(); it != methodNameVector->end(); it++ )
		{
			delete *it;
        }  
		methodNameVector->clear();
		delete methodNameVector;
	}

	if (methodSignatureVector != NULL)
	{
		vector<char*>::iterator it;
		for(it = methodSignatureVector->begin(); it != methodSignatureVector->end(); it++ )
		{
			delete *it;
        }  
		methodSignatureVector->clear();
		delete methodSignatureVector;
		methodSignatureVector = NULL;
	}

	if (excludeClassVector != NULL)
	{
		vector<ExcludeClass*>::iterator it;
		for(it = excludeClassVector->begin(); it != excludeClassVector->end(); it++ )
		{
			delete *it;
        }  
		excludeClassVector->clear();
		delete excludeClassVector;
		excludeClassVector = NULL;
	}
}


int MonitoringTarget::init(DOMElement *node)
{
	//Throwable関連の属性を取得
	XMLCh *xmlTemp;
	DOMNodeList *throwNodes = node->getElementsByTagName(
		xmlTemp = XMLString::transcode(TAG_THROWABLE));
	XMLString::release(&xmlTemp);
	if (throwNodes->getLength() == 1)
	{
		DOMNamedNodeMap *attributes = throwNodes->item(0)->getAttributes();
		//監視クラス名を取得する
		throwableClass = HelperFunc::getAttrValueUtf8(attributes, ATTR_THROWABLE_CLASS);

		//add by Qiu Song on 20091009 for 自動監視のパッケージ重複指定
		if( strcmp( throwableClass, "java.lang.Throwable") == 0)
		{
			bAutoMonitorMode = true;
		}
		//end of add by Qiu Song on 20091009 for 自動監視のパッケージ重複指定
		//add by Qiu Song on 20090821 for 監視中止タスク
		//監視クラス中止タスク(有効かどうか)を取得する
		char* targetValid = HelperFunc::getAttrValueUtf8(attributes, ATTR_VALID);
        if(targetValid != NULL && strcmp(targetValid, "false") == 0)
		{
			bThrowableClassValid = false;
			return 0;
		}
		//end of add by Qiu Song on 20090821 for 監視中止タスク
	}

	//Filters関連の属性を取得
	DOMNodeList *filtersNodes = node->getElementsByTagName(
		xmlTemp = XMLString::transcode(TAG_FILTERS));
	XMLString::release(&xmlTemp);
	int i = 0;
	if (filtersNodes->getLength() == 1)
	{

		DOMNodeList *filterNodes = node->getElementsByTagName(
		    xmlTemp = XMLString::transcode(TAG_FILTER));
	     XMLString::release(&xmlTemp);
         int filterNumber = filterNodes->getLength();
		 for(i = 0;i<filterNumber;i++)
		 {
			 DOMElement* excludeNode = (DOMElement*)(filterNodes->item(i));
             ExcludeClass* excludeClass = new ExcludeClass();
			 excludeClass->setAutoMonitorMode(bAutoMonitorMode);//add by Qiu Song on 20091010
             excludeClass->init(excludeNode);

			 //add by Qiu Song on 20090821 for 監視中止タスク
			 if(excludeClass->getExcludeClassValid() == false)
			 {
				 delete excludeClass;
				 continue;
			 }
			 //end of add by Qiu Song on 20090821 for 監視中止タスク
             //does this exclude class exists?
			 if(excludeClassExist(excludeClass->getExcludeClassName()))
			 {
				 Global::logger->logError("Duplicated monitor class name:%s",
					 excludeClass->getExcludeClassName());
				 delete excludeClass;
				 return -1;
			 }
		
			//insert into the proper place
			vector<ExcludeClass*>::iterator p;
			for (p = excludeClassVector->begin(); p != excludeClassVector->end(); ++p) 
			{
				if (*excludeClass < **p) 
					break;
			}
			excludeClassVector->insert(p, excludeClass);
		 }
		
	}//of if (filtersNodes->getLength() == 1)

    //CatchPlace関連の属性を取得
	DOMNodeList *catchPlaceNodes = node->getElementsByTagName(
		xmlTemp = XMLString::transcode(TAG_PLACE));
	XMLString::release(&xmlTemp);

    getPlaceInfo(catchPlaceNodes);
	return 0;
}

void  MonitoringTarget::getPlaceInfo(DOMNodeList *nodeList)
{

	int placeNumber = nodeList->getLength();
	for(int i = 0; i < placeNumber;i++)
	{
		//CatchPlace Class
		DOMNamedNodeMap *attributes = nodeList->item(i)->getAttributes();
		
		//add by Qiu Song on 20090821 for 監視中止タスク
		char* targetValid = HelperFunc::getAttrValueUtf8(attributes, ATTR_VALID);
		if(targetValid != NULL && strcmp(targetValid, "false") == 0)
		{
			continue;
		}
		//end of add by Qiu Song on 20090821 for 監視中止タスク
		char *value = HelperFunc::getAttrValueUtf8(attributes, ATTR_PLACE_CLASS);
		if (value != NULL)
		{
			//add by Qiu Song on 20091007 for 自動監視機能に同じパッケージを指定する場合の対応処理
			//if( strcmp( getThrowableClass(), "java.lang.Throwable") == 0 )
			if( this->getAutoMonitorMode() == true )
			{
				int placeNumber = placeVector->size();
				if(placeNumber > 0)
				{
					for(int j = 0; j < placeNumber;j++)
					{
						char* placeClass = placeVector->at(j);
						if ( strcmp(placeClass, value) == 0 )
						{
							continue;
						}
					}
				}
			}
			//end of add by Qiu Song on 20091007 for 自動監視機能に同じパッケージを指定する場合の対応処理
            placeVector->push_back(value);
		}

		//Place Method
		value = HelperFunc::getAttrValueUtf8(attributes, ATTR_PLACE_METHOD);
		if (value != NULL)
		{
			methodNameVector->push_back(value);
		}

		//CatchPlace MethodSignature
		value = HelperFunc::getAttrValueUtf8(attributes, ATTR_PLACE_METHOD_SIG);
		if (value != NULL)
		{
			methodSignatureVector->push_back(value);
		}
	}
}

//does the name exist in the exclude class vector?
bool MonitoringTarget::excludeClassExist(const char* name)
{
	vector<ExcludeClass*>::iterator p;
	for (p = excludeClassVector->begin(); p != excludeClassVector->end(); ++p) 
	{
		if (strcmp(name,(*p)->getExcludeClassName()) == 0) 
		{
			return true;
		}
	}

	return false;
}

bool MonitoringTarget::shouldDump(const char *throwableClassName, const char *classSig, 
						   const char *methodName, const char *methodSig)
{
	if (strcmp(throwableClass, throwableClassName) != 0) 
	{
		return false;
	}
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
void MonitoringTarget::logConfig()
{
	string buf;
	buf = "Monitor the Class: ";
	buf += throwableClass;
	LOG4CXX_INFO(Global::logger, buf.c_str());

	int placeNumber = placeVector->size();
	for(int i = 0; i< placeNumber;i++)
	{
		char* placeClass = placeVector->at(i);
		buf = "Package where the monitored class happened: ";
		buf += placeClass;
		LOG4CXX_INFO(Global::logger, buf.c_str());

		char* placeMethod = methodNameVector->at(i);
		buf = "Method Name  where the monitored class happened: ";
		buf += placeMethod;
		LOG4CXX_INFO(Global::logger, buf.c_str());

		char* placeMethodSig = methodSignatureVector->at(i);
		buf = "Method Signature where the monitored class happened: ";
		buf += placeMethodSig;
		LOG4CXX_INFO(Global::logger, buf.c_str());
	}

	if (excludeClassVector != NULL)
	{
		vector<ExcludeClass*>::iterator it;
		for(it = excludeClassVector->begin(); it != excludeClassVector->end(); it++ )
		{
		    (*it)->logConfig();
        }  
	}
}

int MonitoringTarget::operator==(const MonitoringTarget &rhs) const
{
	//compare throwable class 
	bool equal =  strcmp(this->throwableClass, rhs.throwableClass) == 0;
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

	if(this->excludeClassVector->size() != rhs.excludeClassVector->size())
	{
		return false;
	}
	
    //compare excluded classes
	for(i = 0; i < this->excludeClassVector->size();i++ )
	{
		ExcludeClass* first = (ExcludeClass* )this->excludeClassVector->at(i);
        ExcludeClass* second = (ExcludeClass* )rhs.excludeClassVector->at(i);
		if(*first ==*second)
		{ continue;
		}
		else
		{
			return false;
		}
    } 

    return true;

}

//Throwableを監視する時に、発生したExceptionをcatchしたクラス名が、
//監視するパッケージ名に属するかどうかをチェックする
//@param classSig　発生したExceptionをcatchしたクラス
bool MonitoringTarget::isMonitorPackage(const char *classSig){

	int placeNumber = placeVector->size();
	if(placeNumber == 0)
	{
		//no place tag 
		return false;
	}

	bool match = false;
	for(int i = 0; i< placeNumber;i++)
	{
		char* placeClass = placeVector->at(i);
		if(strlen(placeClass)!=0){
			if (strncmp(placeClass, classSig, strlen(placeClass)) == 0){
				 match = true;
				 break;
			}

		}
    }
	return match;
}
