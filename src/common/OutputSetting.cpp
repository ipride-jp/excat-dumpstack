#include "HelperFunc.h"
#include "OutputSetting.h"
#include "Define.h"
#include "Global.h"

//�����ő�̓W�J��
#define MAX_ATTRIBUTE_DEPTH 64

USE_NS(NS_COMMON)

const char *OutputSetting::ATTR_STACK_TRACE_DEPTH = "StackTraceDepth";
const char *OutputSetting::ATTR_ATTRIBUTE_NEST_DEPTH = "AttributeNestDepth";
const char *OutputSetting::ATTR_VARIABLE = "Variable";
const char *OutputSetting::ATTR_ARGUMENT = "Argument";
const char *OutputSetting::ATTR_ATTRIBUTE = "Attribute";
const char *OutputSetting::ATTR_PUBLIC = "Public";
const char *OutputSetting::ATTR_PACKAGE = "Package";
const char *OutputSetting::ATTR_PROTECTED = "Protected";
const char *OutputSetting::ATTR_PRIVATE = "Private";
const char *OutputSetting::ATTR_DUMP_INSTANCE = "DumpInstance";
const char *OutputSetting::ATTR_MAX_ARRAY_OBJ_ELEMENT = "MaxArrayElementForObject";
const char *OutputSetting::ATTR_MAX_ARRAY_PRIMITIVE_ELEMENT = "MaxArrayElementForPrimitive";
const char *OutputSetting::ATTR_MAIL = "Mail";
const char *OutputSetting::ATTR_ATTACHFILE = "AttachFile";
const char *OutputSetting::ATTR_SAVEDAYS = "SaveDays";
const char *OutputSetting::ATTR_DUMPALLTHREADS = "DumpAllThreads";

//add by Qiu Song on 20090929 for This�I�u�W�F�N�g�_���v�L��
const char *OutputSetting::ATTR_THIS = "This";
//end of add by Qiu Song on 20090929 for This�I�u�W�F�N�g�_���v�L��

//add by Qiu Song on 20091022 for �X���b�h�D��x�̎w��
const char *OutputSetting::ATTR_THREAD_PRIORITY = "ThreadPriority";
//end of add by Qiu Song on 20091022 for �X���b�h�D��x�̎w��

OutputSetting::OutputSetting(DOMElement *node) :  stackTraceDepth(15),
	variable(true), argument(true), attribute(true), _public(true), _package(false),
	_protected(false), _private(false), dumpInstance(false), attributeNestDepth(3),
	maxArrayElementForObject(100),maxArrayElementForPrimitive(100),
	_mail(false),_attachFile(false),saveDays(0),dumpThisObject(false),dumpAllThreads(NULL), threadPriority(0),taskPrefix(NULL)//modified by Qiu Song
{
	DOMNamedNodeMap *attributes = node->getAttributes();
	
	//FilePath�����̒l���擾
	char *value;

	//StackTraceDepth�����̒l���擾
	value = HelperFunc::getAttrValueUtf8(attributes, ATTR_STACK_TRACE_DEPTH);
	if (value != NULL)
	{
		stackTraceDepth = strtol(value, NULL, 10);
        if(stackTraceDepth <= 0) 
		{
			stackTraceDepth = 15;
		}
		if(Global::isTrial){
			//���p�łł���΁A50�܂�
			if(stackTraceDepth > 50){
				stackTraceDepth = 50;
			}
		}

		delete[] value;
		value = NULL;
	}

	//AttributeNestDepth�����̒l���擾
	value = HelperFunc::getAttrValueUtf8(attributes, ATTR_ATTRIBUTE_NEST_DEPTH);
	if (value != NULL)
	{
		attributeNestDepth = strtol(value, NULL, 10);
        if(attributeNestDepth < 0) 
		{
			attributeNestDepth = 0;
		}else   if(attributeNestDepth > MAX_ATTRIBUTE_DEPTH) 
		{
			attributeNestDepth = MAX_ATTRIBUTE_DEPTH;
		}
		if(Global::isTrial){
			//���p�łł���΁A3�܂�
			if(attributeNestDepth > 3){
				attributeNestDepth = 3;
			}
		}
        delete[] value;
		value = NULL;
	}

	//variable�����̒l���擾
	value = HelperFunc::getAttrValueUtf8(attributes, ATTR_VARIABLE);
	if (value != NULL)
	{
		variable = strcmp(value, "true") == 0 ? true : false;
		delete[] value;
		value = NULL;
	}

	//argument�����̒l���擾
	value = HelperFunc::getAttrValueUtf8(attributes, ATTR_ARGUMENT);
	if (value != NULL)
	{
		argument = strcmp(value, "true") == 0 ? true : false;
		delete[] value;
		value = NULL;
	}

	//attribute�����̒l���擾
	value = HelperFunc::getAttrValueUtf8(attributes, ATTR_ATTRIBUTE);
	if (value != NULL)
	{
		attribute = strcmp(value, "true") == 0 ? true : false;
		delete[] value;
		value = NULL;
	}

	//public�����̒l���擾
	value = HelperFunc::getAttrValueUtf8(attributes, ATTR_PUBLIC);
	if (value != NULL)
	{
		_public = strcmp(value, "true") == 0 ? true : false;
		delete[] value;
		value = NULL;
	}

	//package�����̒l���擾
	value = HelperFunc::getAttrValueUtf8(attributes, ATTR_PACKAGE);
	if (value != NULL)
	{
		_package = strcmp(value, "true") == 0 ? true : false;
		delete[] value;
		value = NULL;
	}

	//protected�����̒l���擾
	value = HelperFunc::getAttrValueUtf8(attributes, ATTR_PROTECTED);
	if (value != NULL)
	{
		_protected = strcmp(value, "true") == 0 ? true : false;
		delete[] value;
		value = NULL;
	}

	//private�����̒l���擾
	value = HelperFunc::getAttrValueUtf8(attributes, ATTR_PRIVATE);
	if (value != NULL)
	{
		_private = strcmp(value, "true") == 0 ? true : false;
		delete[] value;
		value = NULL;
	}

	//MaxArrayElementForObject�����̒l���擾
	value = HelperFunc::getAttrValueUtf8(attributes, ATTR_MAX_ARRAY_OBJ_ELEMENT);
	if (value != NULL)
	{
		maxArrayElementForObject = strtol(value, NULL, 10);
        if(maxArrayElementForObject <= 0) 
		{
			maxArrayElementForObject = 100;
		}
		delete[] value;
		value = NULL;
	}

	//MaxArrayElementForPrimitive�����̒l���擾
	value = HelperFunc::getAttrValueUtf8(attributes, ATTR_MAX_ARRAY_PRIMITIVE_ELEMENT);
	if (value != NULL)
	{
		maxArrayElementForPrimitive = strtol(value, NULL, 10);
        if(maxArrayElementForPrimitive <=0) 
		{
			maxArrayElementForPrimitive = 100;
		}
		delete[] value;
		value = NULL;
	}

	//dumpInstance�����̒l���擾
	value = HelperFunc::getAttrValueUtf8(attributes, ATTR_DUMP_INSTANCE);
	if (value != NULL)
	{
		dumpInstance = strcmp(value, "true") == 0 ? true : false;
		delete[] value;
		value = NULL;
	}

	//Mail�����̒l���擾
	value = HelperFunc::getAttrValueUtf8(attributes, ATTR_MAIL);
	if (value != NULL)
	{
		_mail = strcmp(value, "true") == 0 ? true : false;
		delete[] value;
		value = NULL;
	}

	//AttachFile�����̒l���擾
	value = HelperFunc::getAttrValueUtf8(attributes, ATTR_ATTACHFILE);
	if (value != NULL)
	{
		_attachFile = strcmp(value, "true") == 0 ? true : false;
		delete[] value;
		value = NULL;
	}

	//SaveDays�����̒l���擾
	value = HelperFunc::getAttrValueUtf8(attributes, ATTR_SAVEDAYS);
	if (value != NULL)
	{
		saveDays = strtol(value, NULL, 10);
        if(saveDays < 0) 
		{
			saveDays = 0;
		}
		else if (saveDays > 365)
		{
			char* buf = new char[256];
			sprintf(buf, "SaveDays [%d] is too large. Change to 365.", saveDays);
			LOG4CXX_WARN(Global::logger, buf);
			saveDays = 365;
		}
		delete[] value;
		value = NULL;
	}

	//DumpAllThreads�����̒l���擾
	value = HelperFunc::getAttrValueUtf8(attributes, ATTR_DUMPALLTHREADS);
	if (value != NULL)
	{
		//modified by Qiu Song on 20090917 for �_���v�X���b�h��Ԏw��
		//dumpAllThreads = strcmp(value, "true") == 0 ? true : false;
		char* strDumpAllThread = new char[MAX_BUF_LEN];
		memset(strDumpAllThread, 0, MAX_BUF_LEN);
		strcpy(strDumpAllThread, value);
		strDumpAllThread[strlen(value)];
		dumpAllThreads = strDumpAllThread;
		//end of modified by Qiu Song on 20090917 for �_���v�X���b�h��Ԏw��
		delete[] value;
		value = NULL;
	}

	//add by Qiu Song on 20090929 for This�I�u�W�F�N�g�̃_���v
	value = HelperFunc::getAttrValueUtf8(attributes, ATTR_THIS);
    if (value != NULL)
	{
		dumpThisObject = strcmp(value, "true") == 0 ? true : false;
		delete[] value;
		value = NULL;
	}
	//end of add by Qiu Song on 20090929 for This�I�u�W�F�N�g�̃_���v

	//add by Qiu Song on 20091022 for �D��x�̎w��
	value = HelperFunc::getAttrValueUtf8(attributes, ATTR_THREAD_PRIORITY);
    if (value != NULL)
	{
		threadPriority = atoi(value);
		delete[] value;
		value = NULL;
	}
	//end of add by Qiu Song on 20091022 for �D��x�̎w��
}

OutputSetting::~OutputSetting()
{
	//add by Qiu Song on 20090917 for �_���v�X���b�h��Ԏw��
    if(dumpAllThreads != NULL)
	{
		delete dumpAllThreads;
		dumpAllThreads = NULL;
	}

	if(taskPrefix != NULL)
	{
		delete taskPrefix;
		taskPrefix = NULL;
	}
	//end of add by Qiu Song on 20090917 for �_���v�X���b�h��Ԏw��
}

void OutputSetting::logConfig()
{
	char buf[MAX_BUF_LEN];
	sprintf(buf, "DumpData Setting--StackTraceDepth: %d", stackTraceDepth);
	LOG4CXX_INFO(Global::logger, buf);

	sprintf(buf, "DumpData Setting--AttributeNestDepth: %d", attributeNestDepth);
	LOG4CXX_INFO(Global::logger, buf);

	sprintf(buf, "DumpData Setting--Variable: %s", variable?"true":"false");
	LOG4CXX_INFO(Global::logger, buf);

	sprintf(buf, "DumpData Setting--Argument: %s", argument?"true":"false");
	LOG4CXX_INFO(Global::logger, buf);

	sprintf(buf, "DumpData Setting--Attribute: %s", attribute?"true":"false");
	LOG4CXX_INFO(Global::logger, buf);

	sprintf(buf, "DumpData Setting--Public: %s", _public?"true":"false");
	LOG4CXX_INFO(Global::logger, buf);

	sprintf(buf, "DumpData Setting--Package: %s", _package?"true":"false");
	LOG4CXX_INFO(Global::logger, buf);

	sprintf(buf, "DumpData Setting--Protected: %s", _protected?"true":"false");
	LOG4CXX_INFO(Global::logger, buf);

	sprintf(buf, "DumpData Setting--Private: %s", _private?"true":"false");
	LOG4CXX_INFO(Global::logger, buf);

	sprintf(buf, "DumpData Setting--DumpInstance: %s", dumpInstance?"true":"false");
	LOG4CXX_INFO(Global::logger, buf);

	sprintf(buf, "DumpData Setting--Max array element for object: %d", maxArrayElementForObject);
	LOG4CXX_INFO(Global::logger, buf);

	sprintf(buf, "DumpData Setting--Max array element for primitive: %d", maxArrayElementForPrimitive);
	LOG4CXX_INFO(Global::logger, buf);

	sprintf(buf, "DumpData Setting--Mail: %s", _mail?"true":"false");
	LOG4CXX_INFO(Global::logger, buf);

	sprintf(buf, "DumpData Setting--AttachFile: %s", _attachFile?"true":"false");
	LOG4CXX_INFO(Global::logger, buf);

	sprintf(buf, "DumpData Setting--SaveDays: %d", saveDays);
	LOG4CXX_INFO(Global::logger, buf);

	sprintf(buf, "DumpData Setting--DumpAllThreads: %s", dumpAllThreads);
	LOG4CXX_INFO(Global::logger, buf);

	//add by Qiu Song on 20090929 for This�I�u�W�F�N�g�_���v
	sprintf(buf, "DumpData Setting--This Object Dump: %s", dumpThisObject?"true":"false");
	LOG4CXX_INFO(Global::logger, buf);
	//end of add by Qiu Song on 20090929 for This�I�u�W�F�N�g�_���v
}

int OutputSetting::operator==(const OutputSetting &rhs) const
{
	return  this->stackTraceDepth == rhs.stackTraceDepth && 
		   this->attributeNestDepth == rhs.attributeNestDepth && 
		   this->variable == rhs.variable &&
		   this->argument == rhs.argument &&
		   this->attribute == rhs.attribute && 
		   this->_public == rhs._public &&
		   this->_package == rhs._package &&
		   this->_protected == rhs._protected && 
		   this->_private == rhs._private &&
		   this->dumpInstance == rhs.dumpInstance &&
		   this->maxArrayElementForPrimitive == rhs.maxArrayElementForPrimitive &&
		   this->maxArrayElementForObject == rhs.maxArrayElementForObject &&
		   this->_mail == rhs._mail &&
		   this->_attachFile == rhs._attachFile &&
		   this->saveDays == rhs.saveDays &&
		   strcmp(dumpAllThreads, rhs.dumpAllThreads) == 0 && 
		   this->dumpThisObject == rhs.dumpThisObject;
}
