#if !defined(_OUTPUTSETTING_H)
#define _OUTPUTSETTING_H

#include <xercesc/dom/DOM.hpp>
#include "Define.h"

#ifndef XERCESC20
XERCES_CPP_NAMESPACE_USE
#endif

BEGIN_NS(NS_COMMON)
class OutputSetting
{
public:
	OutputSetting(DOMElement *parent);
	virtual ~OutputSetting();

private:
	int stackTraceDepth;
	int attributeNestDepth;
	bool variable;
	bool argument;
	bool attribute;
	bool _public;
	bool _package;
	bool _protected;
	bool _private;
	bool dumpInstance;
	int  maxArrayElementForObject;
	int  maxArrayElementForPrimitive;
    bool _mail;
	bool _attachFile;
	int  saveDays;
	//modified by Qiu Song on 20090917 for ダンプスレッド状態指定
	bool dumpThisObject;
	char* dumpAllThreads;
	char* taskPrefix;
	//end of add by Qiu Song on 20090907 for ダンプスレッド状態指定
	int  threadPriority;//add by Qiu Song on 20091022 for スレッド優先度の指定

	static const char *ATTR_STACK_TRACE_DEPTH;
	static const char *ATTR_VARIABLE;
	static const char *ATTR_ARGUMENT;
	static const char *ATTR_ATTRIBUTE;
	static const char *ATTR_PUBLIC;
	static const char *ATTR_PACKAGE;
	static const char *ATTR_PROTECTED;
	static const char *ATTR_PRIVATE;
	static const char *ATTR_ATTRIBUTE_NEST_DEPTH;
    static const char *ATTR_MAX_ARRAY_OBJ_ELEMENT;
	static const char *ATTR_MAX_ARRAY_PRIMITIVE_ELEMENT;
    static const char *ATTR_DUMP_INSTANCE;
	static const char *ATTR_MAIL;
	static const char *ATTR_ATTACHFILE;
	static const char *ATTR_SAVEDAYS;
	static const char *ATTR_DUMPALLTHREADS;
	//add by Qiu Song on 20090929 for Thisオブジェクトダンプ有無
	static const char *ATTR_THIS;
	//end of add by Qiu Song on 20090929 for Thisオブジェクトダンプ有無
	//add by Qiu Song on 20090929 for Thisオブジェクトダンプ有無
	static const char *ATTR_THREAD_PRIORITY;
	//end of add by Qiu Song on 20090929 for Thisオブジェクトダンプ有無
public:
	int  getStackTraceDepth() {return stackTraceDepth;};
	int  getAttributeNestDepth() {return attributeNestDepth;};
	bool getVariable() {return variable;};
	bool getArgument() {return argument;};
	bool getAttribute() {return attribute;};
	bool getPublic() {return _public;};
	bool getPackage() {return _package;};
	bool getProtected() {return _protected;};
	bool getPrivate() {return _private;};
	bool getDumpInstance() {return dumpInstance;};
	//add by Qiu Song on 20090917 for ダンプスレッド状態指定
	char* getDumpAllThreads(){return dumpAllThreads;};
	void setDumpAllThreads(char* strBuf){ dumpAllThreads = strBuf; };
	//bool getDumpAllThreads() {return dumpAllThreads;};
	//end of add by Qiu Song on 20090917 for ダンプスレッド状態指定

	//add by Qiu Song on 20090929 for Thisオブジェクトダンプ有無
	bool getDumpThisObject(){return dumpThisObject;};
	void setDumpThisObject(bool bDump){dumpThisObject = bDump;};
	//end of add by Qiu Song on 20090929 for Thisオブジェクトダンプ有無
	//add by Qiu Song on 20090929 for 優先度ダンプ有無
	int getThreadPriority(){return threadPriority;};
	void setTaskPrefix(char* prefix){ taskPrefix = prefix;};
	char* getTaskPrefix(){return taskPrefix;};
	//end of add by Qiu Song on 20090929 for 優先度ダンプ有無
	int  getMaxArrayElementForObject() {return maxArrayElementForObject;};
	int  getMaxArrayElementForPrimitive(){return maxArrayElementForPrimitive;};
	bool getMail(){return _mail;};
	bool getAttachFile(){return _attachFile;};
	int  getSaveDays(){return saveDays;};
	int  operator==(const OutputSetting &rhs) const;
    void logConfig();
};
END_NS

#endif  //_OUTPUTSETTING_H
