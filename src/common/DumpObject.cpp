#include "../common/HelperFunc.h"
#include "Global.h"
#include "DumpObject.h"
#include <string.h>

USE_NS(NS_COMMON)


const char *DumpObject::TAG_FIELD = "Field";
const char *DumpObject::ATTR_CLASS = "Class";
const char *DumpObject::ATTR_NAME = "Name";
const char *DumpObject::ATTR_VALID = "Valid";

DumpObject::DumpObject():objectClassName(NULL),valid(true)
{
	fieldVector = new  vector<char*>;
}

DumpObject::~DumpObject()
{
	if(objectClassName  != NULL)
	{
		delete objectClassName;
		objectClassName = NULL;
	}
	if (fieldVector != NULL)
	{
		vector<char*>::iterator it;
		for(it = fieldVector->begin(); it != fieldVector->end(); it++ )
		{
			delete *it;
        }  
		fieldVector->clear();
		delete fieldVector;
		fieldVector = NULL;
	}
}

int DumpObject::init(DOMElement *node)
{
	//ObjectElementŠÖ˜A‚Ì‘®«‚ðŽæ“¾
	DOMNamedNodeMap *attributes = node->getAttributes();

	char *value;
	GET_ATTR_VALUE(attributes, ATTR_VALID, value);
    if(value != NULL)
	{
		char* validString = HelperFunc::strdup(value);
		XMLString::release(&value);

		if(strcmp(validString,"true") != 0)
		{
			valid = false;
			delete validString;
			return 0;
		}else
		{
			valid = true;
            delete validString;
        }
	}

	value = HelperFunc::getAttrValueUtf8(attributes, ATTR_CLASS);
	if (value != NULL)
	{
		objectClassName = value;
	}

    //get fields 
	XMLCh *xmlTemp;
	DOMNodeList *fieldsNodes = node->getElementsByTagName(
	    xmlTemp = XMLString::transcode(TAG_FIELD));
	XMLString::release(&xmlTemp);
	int fieldsNum = fieldsNodes->getLength();
	for(int i=0; i < fieldsNum;i++)
	{
       DOMNamedNodeMap *attributesField = fieldsNodes->item(i)->getAttributes();
	   char *valueField;
	   GET_ATTR_VALUE(attributesField, ATTR_VALID, valueField);
       if(valueField != NULL)
	   {
		    char* validString = new char[strlen(valueField) + 1];
		    strcpy(validString, valueField);
			XMLString::release(&valueField);
			if(strcmp(validString,"true") != 0)
			{
				delete validString;
				continue;
			}else
			{
                delete validString;
            }
	   }

	   valueField = HelperFunc::getAttrValueUtf8(attributesField, ATTR_NAME);
	   if(valueField != NULL)
	   {
		    char* fieldName = valueField;
	        //insert into the proper place
		    vector<char*>::iterator p;
			string buf;
		    for (p = fieldVector->begin(); p != fieldVector->end(); ++p) 
			{
				if(strcmp((const char*)fieldName,(const char*)(*p)) == 0)
				{
					buf = "Duplicated field:";
                    buf += fieldName;
					buf += " for class:";
					buf += objectClassName;
					LOG4CXX_ERROR(Global::logger, buf.c_str())
					delete fieldName;
					return -1;
				}
		   		if(strcmp((const char*)fieldName,(const char*)(*p)) < 0)
				{
					break;
                }
			}
            fieldVector->insert(p, fieldName);
	   }
	}

	return 0;
}

//you should call this function after config file is read
void DumpObject::logConfig()
{
	string buf;
	buf = "Dump Template for Class: ";
    buf += objectClassName;
	LOG4CXX_INFO(Global::logger, buf.c_str());
	//info
	if(fieldVector != NULL)
	{
		vector<char*>::iterator it;
		for(it = fieldVector->begin(); it != fieldVector->end(); it++ )
		{
			buf = "    dump field:";
			buf += (char*)(*it);
		    LOG4CXX_INFO(Global::logger, buf.c_str());
		}  
	}
}

bool DumpObject::shouldDump(char* fieldName)
{
	bool found = false;
	vector<char*>::iterator it;
	for(it = fieldVector->begin(); it != fieldVector->end(); it++ )
	{
		if(strcmp(fieldName,*it) == 0)
		{
			found = true;
			break;
		}
    }  

	return found;
}

int DumpObject::operator<(const DumpObject &rhs) const
{
    return strcmp(this->objectClassName, rhs.objectClassName) < 0;
}

int DumpObject::operator==(const DumpObject &rhs) const
{
	//compare  class name 
	bool equal =  strcmp(this->objectClassName, rhs.objectClassName) == 0;
	if(!equal)
	{
       return equal;
	}
	if(this->fieldVector->size() != rhs.fieldVector->size() )
	{
       return false;
	}
    //compare fields
	for(int i = 0; i < this->fieldVector->size();i++ )
	{
		char* first = (char* )this->fieldVector->at(i);
        char* second = (char* )rhs.fieldVector->at(i);
		if(strcmp(first,second)==0)
		{ 
			continue;
		}
		else
		{
			return false;
		}
    } 
    return true;

}
