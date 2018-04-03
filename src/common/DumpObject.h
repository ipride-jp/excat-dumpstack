#if !defined(_DUMPOBJECT_H)
#define _DUMPOBJECT_H

#include <vector>
#include <xercesc/dom/DOM.hpp>
#include "Define.h"

using namespace std;
#ifndef XERCESC20
XERCES_CPP_NAMESPACE_USE
#endif

BEGIN_NS(NS_COMMON)
/**
 *ダンプするオブジェクトのフィールドを指定する 
 */
class DumpObject
{
	public:
		DumpObject();
        virtual ~DumpObject();
		int operator==(const DumpObject &rhs) const;
		int operator<(const DumpObject &rhs) const;
		int init(DOMElement *node);
    private:
		char* objectClassName;
		vector<char*> *fieldVector;
        bool valid;
  	    static const char *TAG_OBJECTELEMENT;
	    static const char *TAG_FIELD;
	    static const char *ATTR_CLASS;
	    static const char *ATTR_NAME;
		static const char *ATTR_VALID;
    public:
		char* getObjectClassName(){ return objectClassName;};
		bool shouldDump(char* fieldName);
		bool isValid(){return valid;};
	    void logConfig();
};

END_NS
#endif
