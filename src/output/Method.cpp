#include "Method.h"
#include "../common/Define.h"
#include "../common/HelperFunc.h"
#include "TypedElement.h"
#include "../common/Global.h"
#include "../common/JvmtiAutoRelease.h"
#include "../common/JvmUtilFunc.h"
#include "../common/JniLocalRefAutoRelease.h"
#include "../common/ObjectAutoRelease.h"
#include "../classfileoperation/OpCodeScan.h"
#include "../classfileoperation/ClassFileConstant.h"
#include "../jniutility/AgentCallbackHandler.h"
#include <vector>
#include <algorithm>

USE_NS(NS_OUTPUT)
USE_NS(NS_COMMON)
USE_NS(NS_CLASS_FILE_OPERATION)
USE_NS(NS_JNI_UTILITY)

#define POINTER 

using namespace std;

const char *Method::TAG_METHOD = "Method";
const char *Method::ATTR_NAME = "Name";
const char *Method::ATTR_SIG = "Signature";
const char *Method::ATTR_DECLARING_CLASS = "DeclaringClass";
const char *Method::ATTR_LOCATION = "Location";
const char *Method::ATTR_LINE_NUM = "LineNumber";

bool EntryComp(const jvmtiLocalVariableEntry * x, 
	const jvmtiLocalVariableEntry * y) 
{
	return x->slot < y->slot;
}

Method::Method(MethodParam *param,bool bCurDumpMethod)
{
	this->param = param;
    table = NULL;
	//add by Qiu Song on 20090817 for メソッド監視
	m_objDefinedType = NULL;
	m_objValue = NULL;
	m_objName = NULL;
	m_objectRef = -1;
	bIsCurrentDumpMethod = bCurDumpMethod;
	//end of add by Qiu Song on 20090817 for メソッド監視
	init();
}

Method::~Method()
{
}

void Method::ReleaseVariableEntryTable(jvmtiLocalVariableEntry *table, int count)
{
	if (table == NULL)
		return;

	for (int index = 0; index < count; index++)
	{
		jvmtiLocalVariableEntry *entry = table + index;
		
		char *name = entry->name;
		char *sig = entry->signature;
		char *genSig = entry->generic_signature;
		
		AUTO_REL_JVMTI_OBJECT(name);
		AUTO_REL_JVMTI_OBJECT(sig);
		AUTO_REL_JVMTI_OBJECT(genSig);
	}				
}

//メソッド情報を取得する関数
void Method::init()
{
	string logBuf;
	char digitBuf[MAX_BUF_LEN];

    MethodData* methodData = param->methodData;
	//set class info and source file name
	char* classSig = NULL;
    if(!setDeclareClassInfo(methodData, &classSig))
	{
		return;
	}
    AUTO_REL_OBJECT(classSig);

	//set method name
	jmethodID methodId = param->frameInfo->method;
	char *methodName = NULL, *methodSig = NULL;
	jvmtiError err;
	err = param->jvmti->GetMethodName(param->frameInfo->method, &methodName,
		&methodSig, NULL);
	if (err != JVMTI_ERROR_NONE)
	{
		if(Global::logger->isDebugEnabled())
            Global::logger->logDebug("Failed to call GetMethodName,error cd=%d",err);
		return;
	}
	AUTO_REL_JVMTI_OBJECT(methodName);
    AUTO_REL_JVMTI_OBJECT(methodSig);
	setMethodName(methodData, methodName, methodSig);
	
	//set method line number
    setLineNumber(methodData);
    methodData->setLocation(param->frameInfo->location);
    
	//add by Qiu Song on 20090828 for メソッド監視
	if(param->frameInfo->location > 2)
	{
		param->parent->setDumpPosition("Finish");
	}
	else
	{
		param->parent->setDumpPosition("Start");
	}
	//end of add by Qiu Song on 20090828 for メソッド監視

	//add by Qiu Song on 20091118 for バグ:518
	/*bool bShouldDumpCurrentMethod = false;
	if(bIsCurrentDumpMethod == true)
	{
		char *newMethodSig = HelperFunc::convertMethodSig(methodSig);
		MethodDumpInfo* pDumpInfo = 
			AgentCallbackHandler::getMethodDumpInfo(param->parent->getParamClassURL(), methodName, newMethodSig);
		if (NULL != pDumpInfo && false != pDumpInfo->bValid)
		{
			bShouldDumpCurrentMethod = true;
		}
		if(newMethodSig != NULL)
		{
			delete newMethodSig;
			newMethodSig = NULL;
		}
	}*/
	//end of add by Qiu Song on 20091118 for バグ:518
	//set access flags
	jint modifiers = setAccessFlags(methodData);
    //ACC_STATIC  0x0008  Is static. 
	bool isStaticMethod = false;
	if(modifiers & 0x0008)
	{
		isStaticMethod = true;
	}

	//ACC_NATIVE  0x0100 s native  
	if(modifiers & 0x0100)
	{
		return;
	}

	param->parent->addDumpDataSize(methodData->getDumpSize());
    if(Global::getIsJrockitJvm() && (false == AgentCallbackHandler::isClassLoaderClass(classSig)))
	{
		if(isNextFrameLoadClass())
			return;
	}
	
	jint count = 0;   //local variable number
	//get local variable table
	vector<jvmtiLocalVariableEntry *> entryVec;
    bool bHasVarTable = checkVariableTable(&entryVec);
    AUTO_REL_JVMTI_OBJECT(table);

	char* varTypes = NULL;  //use to get var type when there is no local variable table
	if(bHasVarTable)
	{
        count = entryVec.size();
	}else
	{
		count = getLocalVarNumberWithoutDebug(isStaticMethod,&varTypes);
        if(count == 0)
			return;
   	}

	//get the number of method's parameters
	jint paramSlotNum = 0;
	jint slotNum = 0;
	if(bHasVarTable)
	{
		paramSlotNum = HelperFunc::getMethodParamNum(methodSig);
	}else
	{
		//get the slot number where params occupy
		paramSlotNum = getArgumentSizeWithotDebug(isStaticMethod);
		if (paramSlotNum == -1)  return;
		if(!isStaticMethod)
		{
			slotNum = paramSlotNum + 1;
		}else
		{
			slotNum = paramSlotNum;
		}   	
	}
    
	if(Global::logger->isDebugEnabled())
	{
		sprintf(digitBuf, "method paramSlotNum numer =%d",paramSlotNum);
		LOG4CXX_DEBUG(Global::logger, digitBuf);
	}

	if(!bHasVarTable && paramSlotNum > 0)
	{
		//引数情報のタイプは、signatureによる取得(補足)
        HelperFunc::getMethodParamType(methodSig,paramSlotNum,isStaticMethod,varTypes);
	}

	//analyze byte code to get local variable type
	bool needAnalyzeByteCode = false;
	unsigned byteCodeCurrent = 0;
	int invalidSlot = -1;

	//count:VarTableに格納変数の個数 slotNum：関数のパラメータの桁数(例えば、doubleの場合、2になります)
	if(!bHasVarTable && count > slotNum || 
		Global::getIsJrockitJvm())
	{
		//jrockitの場合、バイトコードを分析する必要がある。
        needAnalyzeByteCode = true;
	}

	if(needAnalyzeByteCode)
	{
		jint bytecode_count;  //byte code size
		unsigned char* bytecodes_ptr;  //point to byte code array

	   if(!HelperFunc::validateJvmtiError(param->jvmti,
			param->jvmti->GetBytecodes(methodId,&bytecode_count,&bytecodes_ptr),"GetBytecodes"))
		{
	        logBuf = "can't get byte code from jvmti for method: ";
			logBuf += methodName;
		    LOG4CXX_WARN(Global::logger, logBuf.c_str());
			entryVec.clear();
			if(varTypes != NULL)
			{
				delete varTypes;
				varTypes = NULL;
			}
			return;
        }
		if(bytecode_count > 0)
		{
            //create clas OpCodeScan
            OpCodeScan opCode(bytecode_count,bytecodes_ptr);
			if(!bHasVarTable && count > slotNum)
            {   //Debug情報がない場合、バイトコードを分析する
				opCode.getVarTypes(count,varTypes,param->frameInfo->location,slotNum);
			}
			if(Global::getIsJrockitJvm())
			{
                byteCodeCurrent = opCode.getCodeAtLocation(param->frameInfo->location);
				invalidSlot = opCode.chkInvalidSlotForJrockit(param->frameInfo->location);
				if(Global::logger->isDebugEnabled())
				{
#ifdef _LINUX
					Global::logger->logDebug("current byte code=%d,location=%lld,invalidSlot=%d",
					     byteCodeCurrent,param->frameInfo->location,invalidSlot);
#else
					Global::logger->logDebug("current byte code=%d,location=%I64d,invalidSlot=%d",
						byteCodeCurrent,param->frameInfo->location,invalidSlot);
#endif
				}

			}
		    //release the memory for  bytecodes_ptr
		    param->jvmti->Deallocate((unsigned char*)bytecodes_ptr);
		}
	}//end of 「if(needAnalyzeByteCode)」

	if(Global::getIsJrockitJvm())
	{
        if(param->frameInfo->location < 0)
		{
			if(bHasVarTable)
			{
				this->ReleaseVariableEntryTable(table, count);
			}
			entryVec.clear();
			if(varTypes != NULL)
			{
				delete varTypes;
				varTypes = NULL;
			}
			return;
		}
		if(param->frameInfo->location == 0)
		{
			//location 0でinvokestatic以外、例外発生する可能性がない
			if(byteCodeCurrent != opc_invokestatic)
			{
				if(bHasVarTable)
				{
					this->ReleaseVariableEntryTable(table, count);
				}
				entryVec.clear();
				if(varTypes != NULL)
				{
					delete varTypes;
					varTypes = NULL;
				}
				return;
			}
		}else
		{
			//newを実行する場合、当フレームがVMモードにあって、Slotを取得できない
			if(byteCodeCurrent == opc_new)
			{
				this->ReleaseVariableEntryTable(table, count);
				entryVec.clear();
				if(varTypes != NULL)
				{
					delete varTypes;
					varTypes = NULL;
				}
				return;
			}
		}
	}//end of 「if(Global::getIsJrockitJvm())」

    int index = 0;
	int hasThis = checkHasThis(&entryVec,isStaticMethod,bHasVarTable);

	bool shouldskip = false; //long,double type take two slots
	bool execeedDataLimit = false;
	char dummyName[8];

	int loopMax = 0;
	if(bHasVarTable)
	{
		loopMax = count;
	}else
	{
		//jrockit jvmの時、ローカル変数を取得すると、JVMが落ちることがある
		if(Global::getIsJrockitJvm())
		{
			loopMax = paramSlotNum;
			if(!isStaticMethod)
			{
				loopMax++;//to include this
			}
		}else
		{
            loopMax = count;
		}
	}

	//Var Tableの各パラメータの値を取得する
	for (index = 0; index < loopMax; index++)
	{
		if(!bHasVarTable && shouldskip)
		{ 
			shouldskip = false;
			continue;
         }

		jvmtiLocalVariableEntry *entry = NULL;
	    int startPos = 0;
		int endPos = 0;

		TypedElementParam typedParam;
		typedParam.jni = param->jni;
		typedParam.jvmti = param->jvmti;
		typedParam.parent = methodData;
		typedParam.ancestor = param->parent;
		typedParam.valid = true;
		typedParam.objectRef = -1;
		typedParam.arrIndex = -1;
		typedParam.objValue = NULL;
		typedParam.object = NULL;
		typedParam.pool = param->pool;
        typedParam.objDefinedType = NULL;

		Global::curAttrDepth = 0;
		int slotNumber;
		if(bHasVarTable)
		{   
			entry = entryVec[index];
			typedParam.objDefinedType = entry->signature;
			typedParam.objName = entry->name;
			typedParam.unsure = false;
            startPos = entry->start_location;
		    endPos = startPos + entry->length;
			slotNumber = entry->slot;
		}else
		{
			typedParam.objDefinedType = NULL;
			typedParam.unsure = true;

			sprintf(dummyName,"%%%d",index);
			typedParam.objName = dummyName;
			startPos = 0;
            endPos = 65535; //max integer
			slotNumber = index;
		}

		if(Global::logger->isDebugEnabled())
		{
			if(bHasVarTable){
				Global::logger->logDebug("dump variable %s,slot=%d",typedParam.objName,
					entry->slot);
			}else{
				Global::logger->logDebug("dump variable %s,index=%d",typedParam.objName,
					index);
            } 
		}

		//該当変数が有効かどうか判断する
        if(param->frameInfo->location >= startPos && param->frameInfo->location <= endPos)
		{
			typedParam.valid = true;
		}else
        {	
			typedParam.valid = false;
		}

		if(!bHasVarTable)
		{
			//double,longの場合、2byteが必要です
			if(varTypes[index] == 'D' || varTypes[index] == 'J')
			{
				shouldskip = true;
			}

			if(invalidSlot == index)
			{
				continue;
			}
			jvmtiError err = JVMTI_ERROR_NONE;
			if(!getVariableInfo(&typedParam,methodId,index,varTypes[index], &err))
			{
				if (JVMTI_ERROR_INVALID_SLOT == err)
				{
					break;
				}
				else
				{
					continue; //can't get infomation for this variable
				}
			}
		}
		else
		{
			if(invalidSlot == entry->slot)
			{
				continue;
			}			
		}

		if(typedParam.valid )
		{
			bool isPrimitive = false;
			if(bHasVarTable)
			{
				if(JvmUtilFunc::isPrimitiveType(entry->signature))
				{
					isPrimitive = true;
				}
			}else
			{   //no var table
	            if(varTypes[index] == 'U')
				{
					isPrimitive = true; //type unknown
				}else if(JvmUtilFunc::isPrimitiveType(typedParam.objDefinedType))
				{ 
					isPrimitive = true;
				}

			}
			
			if (isPrimitive) 
			{
				jvmtiError err = JVMTI_ERROR_NONE;
				if(bHasVarTable)
				{
					typedParam.objValue = JvmUtilFunc::getPrimitiveLocalVariableValue(param->jvmti,
						param->thread,param->depth, entry->slot, typedParam.objDefinedType[0], &err);
					if (JVMTI_ERROR_INVALID_SLOT == err)
					{
						if (typedParam.objValue != NULL)
						{
							delete typedParam.objValue;
							typedParam.objValue = NULL;
						}
						break;
					}
				}
			}
			else
			{
				jobject object = NULL;
				jvmtiError err;
				if (bHasVarTable)
				{
					err = JvmUtilFunc::myGetLocalObject(param->jvmti,param->thread, param->depth, 
					      entry->slot, &object);
				}else
				{
					object = typedParam.object;
					err = JVMTI_ERROR_NONE;
				}
				AUTO_REL_JNI_LOCAL_REF(param->jni, object);
                
				if (err == JVMTI_ERROR_NONE)
				{
					try{
						if (object != NULL && typedParam.valid)
						{
							//add by Qiu Song on 20090819 for 指定タイプのオブジェクトダンプ
							/*if( param->parent->getDumpType() != 1 &&
								HelperFunc::shouldDumpCurrentObject(param->jvmti,param->jni,object) == false)
							{
								continue;
							}*/
							//end of add by Qiu Song on 20090819 for 指定タイプのオブジェクトダンプ
							//当オブジェクトがプールにない場合、プールに追加
							int refId = param->pool->getObjectId(object);
							ObjectData* objData = NULL;
							if(refId < 0)
							{
                                 typedParam.objectRef = param->pool->addObject(object);
								 refId = typedParam.objectRef;
								 objData = param->parent->getObjectData(refId);
                                 //ダンプするデータ量を増えた
						   	     param->parent->addDumpDataSize(objData->getDumpSize());
							}
							else
							{
								objData = param->parent->getObjectData(refId);
							}
                            typedParam.objectRef = refId;
							//展開可能の場合、オブジェクトを展開する
							if(!objData->getExpanded())
							{
							    param->pool->expandObject(object,objData,param->parent,0);
							}
						}
					}catch(ExceedMemLimitException* e)
					{
						//even data limit is execeeded,we need to hold the
						//relation betweend method and variabe
						//and the same exception will be thrown in TypeEelement()
                         execeedDataLimit = true;
						 delete e;
					}
				}//end of 「if (err == JVMTI_ERROR_NONE)」
				else //failed to get variable
				{
					if(Global::logger->isDebugEnabled())
                        Global::logger->logDebug("failed to call GetLocalObject in Method::init,error cd =%d,depth =%d,slot=%d,index=%d.",
						    err,param->depth,entry->slot,index);
					typedParam.valid = false;
					if (JVMTI_ERROR_INVALID_SLOT == err)
					{
						if (!bHasVarTable && typedParam.objDefinedType != NULL)
						{
							delete typedParam.objDefinedType;
							typedParam.objDefinedType = NULL;
						}
						if(typedParam.objValue != NULL)
						{
							delete typedParam.objValue;
							typedParam.objValue = NULL;
						}
						break;						
					}
				}
			}
		}//end of 「if(typedParam.valid )」 line:410
 
		//check if it is this pointer
		const char* tagName = TypedElement::TAG_VARIABLE;
		//add by Qiu Song on 20090929 for Thisオブジェクトのダンプ
		bool bDumpThisObject = Global::getConfig()->getOutputSetting()->getDumpThisObject();
		//end of add by Qiu Song on 20090929 for Thisオブジェクトのダンプ
		
		if (bHasVarTable)
		{
			if (entry->slot == 0)
			{
				if(hasThis)
				{
		            //add by Qiu Song on 20090929 for Thisオブジェクトのダンプ
					if(bDumpThisObject == false)
					{
						continue;
					}
		            //end of add by Qiu Song on 20090929 for Thisオブジェクトのダンプ
					tagName = TypedElement::TAG_THIS;
					TypedElement typedElem(&typedParam,tagName);
					if(typedParam.objValue != NULL)
					{
						delete typedParam.objValue;
						typedParam.objValue = NULL;
					}
					continue;
				}
			}
		}else
		{
			if(index ==0 && !isStaticMethod )
			{
		        //add by Qiu Song on 20090929 for Thisオブジェクトのダンプ
				if(bDumpThisObject == false)
				{
					continue;
				}
		        //end of add by Qiu Song on 20090929 for Thisオブジェクトのダンプ
				tagName = TypedElement::TAG_THIS;
                typedParam.objName = "this";
				TypedElement typedElem(&typedParam,tagName);
				if(typedParam.objValue != NULL)
				{
					delete typedParam.objValue;
					typedParam.objValue = NULL;
				}
				delete typedParam.objDefinedType;
				typedParam.objDefinedType = NULL;
				continue;
			}
		}

		//judge if it is method parameter
		if(bHasVarTable)
		{
			if (entry->slot < (paramSlotNum + hasThis))
			{
				tagName = TypedElement::TAG_ARGUMENT;
			}
		}else
		{
			if (index < (paramSlotNum + hasThis))
			{
				tagName = TypedElement::TAG_ARGUMENT;
			}
		}

		TypedElement typedElem(&typedParam,tagName);
		if(typedParam.objValue != NULL)
		{
			delete typedParam.objValue;
			typedParam.objValue = NULL;
		}
		
		if (!bHasVarTable && typedParam.objDefinedType != NULL)
		{
			delete typedParam.objDefinedType;
			typedParam.objDefinedType = NULL;
		}
		
	}//for (index = 0; index < loopMax; index++)
	
	//add by Qiu Song on 20090817 for メソッド監視
	if(bIsCurrentDumpMethod == true
		&& param->parent->getDumpType() == 1)
	{
		getMethodReturnValue();
	}
	//end of add by Qiu Song on 20090817 for メソッド監視

	//release the contents of object table
	if(bHasVarTable)
	{
		this->ReleaseVariableEntryTable(table, count);
		entryVec.clear();		
	}else
	{
		if(varTypes != NULL)
		{
			delete varTypes;
			varTypes = NULL;
		}
	}
}

/*
 * get variable information when there is no local variable table
 *
 */
bool Method::getVariableInfo(TypedElementParam *pTypedParam,jmethodID methodId,int slot,
							  char scanType, jvmtiError* err)
{
	jint value;
	*err = JVMTI_ERROR_NONE;

	char buf[MAX_BUF_LEN];
	//try object
	if(scanType == 'L')
	{
		jobject object = NULL;
		*err = JvmUtilFunc::myGetLocalObject(param->jvmti,param->thread, param->depth,slot,&object);
		if(*err==JVMTI_ERROR_NONE && object != NULL)
		{
			char *realType = NULL;
			jclass clazz = param->jni->GetObjectClass(object);
			AUTO_REL_JNI_LOCAL_REF(param->jni, clazz);
			if(clazz !=NULL)
			{
				*err = param->jvmti->GetClassSignature(clazz, &realType, NULL);
				AUTO_REL_JVMTI_OBJECT(realType);
				if(*err == JVMTI_ERROR_NONE)
				{
					char *objDefinedType = new char[strlen(realType) + 1];
					strcpy(objDefinedType,realType);
					pTypedParam->objDefinedType = objDefinedType;
					pTypedParam->object = object;
					return true;
				}else
				{
					Global::logger->logError("failed to call GetClassSignature in Method::getVariableInfo,error cd =%d,slot=%d.",*err,slot);
					return false;
				}

			}else
			{
				return false;
			}

		}else if(*err==JVMTI_ERROR_NONE && object == NULL)
		{
			char *objDefinedType = new char[20];
		    strcpy(objDefinedType,"Ljava/lang/Object;");
			pTypedParam->objDefinedType = objDefinedType;
			pTypedParam->object = object;
			return true;
		}else
		{
			return false;
		}
	}


	//try long
	if(scanType == 'J')
	{
		jlong longValue;
		*err = JvmUtilFunc::myGetLocalLong(param->jvmti,param->thread, param->depth, slot, &longValue);
		if(*err==JVMTI_ERROR_NONE)
		{//it's long 
		   char *objDefinedType = new char[2];
		   strcpy(objDefinedType,"J");
		   pTypedParam->objDefinedType = objDefinedType;

		   HelperFunc::getLongTypeValue(longValue,buf);
		   char *result = new char[strlen(buf) + 1];
		   strcpy(result, buf);
		   pTypedParam->objValue = result;
		   pTypedParam->unsure = false;
		   return true;
		}else
		{
			return false;
		}
	}

	//try int 
	if(scanType == 'I' || scanType == 'C' || scanType == 'Z' || 
		scanType == 'B' || scanType == 'S')
	{
		*err = JvmUtilFunc::myGetLocalInt(param->jvmti,param->thread, param->depth, slot, &value);
		if(*err==JVMTI_ERROR_NONE)
		{//it's integer ,byte ,short,boolean,char and we handle it as int
	       char *objDefinedType = new char[2];
		   strcpy(objDefinedType,"I");
		   objDefinedType[0] = scanType;
		   pTypedParam->objDefinedType = objDefinedType;

		   if(scanType == 'C')
		   {
			   sprintf(buf, "%04x", value);
		   }else
		   {
	           sprintf(buf, "%ld", value);
		   }
		   char *result = new char[strlen(buf) + 1];
		   strcpy(result, buf);
		   pTypedParam->objValue = result;
		   return true;
		}
		else
		{
			return false;
		}
	}
 
	//try float
	if(scanType == 'F')
	{
		jfloat floatValue;
		*err = JvmUtilFunc::myGetLocalFloat(param->jvmti,param->thread, param->depth, slot, &floatValue);
		if(*err==JVMTI_ERROR_NONE)
		{//it's float 
	       char *objDefinedType = new char[2];
		   strcpy(objDefinedType,"F");
		   pTypedParam->objDefinedType = objDefinedType;

		   sprintf(buf, "%e", floatValue);
		   char *result = new char[strlen(buf) + 1];
		   strcpy(result, buf);
		   pTypedParam->objValue = result;
		   pTypedParam->unsure = false;
		   return true;
		}else
		{
			return false;
		}
	}


	//try double
	if(scanType == 'D')
	{
		jdouble doubletValue;
		*err = JvmUtilFunc::myGetLocalDouble(param->jvmti,param->thread, param->depth, slot, &doubletValue);
		if(*err==JVMTI_ERROR_NONE)
		{//it's double 
	       char *objDefinedType = new char[2];
		   strcpy(objDefinedType,"D");
		   pTypedParam->objDefinedType = objDefinedType;

		   sprintf(buf, "%e", doubletValue);
		   char *result = new char[strlen(buf) + 1];
		   strcpy(result, buf);
		   pTypedParam->objValue = result;
		   pTypedParam->unsure = false;
		   return true;
		}else
		{
			return false;
		}

	}

    //unknownd
	if(scanType == 'U')
	{
	    char *objDefinedType = new char[2];
		strcpy(objDefinedType,"U");
		pTypedParam->objDefinedType = objDefinedType;

		char *result = new char[10];
		sprintf(result, "unknown");
		pTypedParam->objValue = result;
		return true;
	}

    return false;
}

void Method::setLineNumber(MethodData* methodData)
{
	jvmtiError err;

	//get method line number 
	int lineNumber = 0;
	jint entryCount = 0;
	jvmtiLineNumberEntry *entryTable = NULL;
	err = param->jvmti->GetLineNumberTable(param->frameInfo->method, 
		&entryCount, &entryTable);
	if (err == JVMTI_ERROR_NONE)
    {
		lineNumber = HelperFunc::getLineNumberByLocation(entryTable, entryCount, 
			param->frameInfo->location);
	}else
    {
		if(Global::logger->isDebugEnabled())
		    Global::logger->logDebug("Failed to call GetLineNumberTable,error cd=%d",err);
	}

	AUTO_REL_JVMTI_OBJECT(entryTable);
	methodData->setLineNumber(lineNumber);
}

jint Method::setAccessFlags(MethodData* methodData)
{
	jint modifiers = 0;
	jvmtiError err;

	err = param->jvmti->GetMethodModifiers(param->frameInfo->method, &modifiers);
    if (err == JVMTI_ERROR_NONE)
	{
		methodData->setModifiers(modifiers);
	}else
	{
		if(Global::logger->isDebugEnabled())
            Global::logger->logDebug("Failed to call GetMethodModifiers,error cd=%d",err);
	}

	return modifiers;
}

//check if the next frame is java.lang.ClassLoader.loadClass
bool Method::isNextFrameLoadClass()
{
	if(param->depth <=2)
		return false; //the frame at the bottom

	jmethodID nextMethod = (param->frameInfo - 1)->method;

	char *methodName = NULL;
	jvmtiError err;
	err = param->jvmti->GetMethodName(nextMethod, &methodName,
		NULL, NULL);
	if (err != JVMTI_ERROR_NONE)
	{
		if(Global::logger->isDebugEnabled())
            Global::logger->logDebug("Failed to call GetMethodName,error cd=%d",err);
		return false;
	}
	AUTO_REL_JVMTI_OBJECT(methodName);

    if(strcmp(methodName,"<clinit>") == 0)
		return true;
	
    if(strcmp(methodName,"loadClass") != 0)
		return false;

	jclass declaringClass = NULL;
	err = param->jvmti->GetMethodDeclaringClass(nextMethod, &declaringClass);
	if (err != JVMTI_ERROR_NONE)
	{
		if(Global::logger->isDebugEnabled())
		    Global::logger->logDebug("Failed to call GetMethodDeclaringClass,error cd=%d",err);
		return false;
    }  

	AUTO_REL_JNI_LOCAL_REF(param->jni, declaringClass);
	char *classSig = NULL;
	err = param->jvmti->GetClassSignature(declaringClass, &classSig, NULL);
	if (err != JVMTI_ERROR_NONE)
    {
		if(Global::logger->isDebugEnabled())
		    Global::logger->logDebug("Failed to call GetClassSignature,error cd=%d",err);
		return false;
    } 
	AUTO_REL_JVMTI_OBJECT(classSig);

	if(strcmp(classSig,"Ljava/lang/ClassLoader;") ==0)
		return true;

	return AgentCallbackHandler::isClassLoaderClass(classSig);
}

bool Method::setDeclareClassInfo(MethodData* methodData, char** pClassSig)
{
	jclass declaringClass = NULL;
	*pClassSig = NULL;
	jvmtiError err;
	err = param->jvmti->GetMethodDeclaringClass(param->frameInfo->method, &declaringClass);
	if (err != JVMTI_ERROR_NONE)
	{
		if(Global::logger->isDebugEnabled())
		    Global::logger->logDebug("Failed to call GetMethodDeclaringClass,error cd=%d",err);
		return false;
    }  

	AUTO_REL_JNI_LOCAL_REF(param->jni, declaringClass);

	char *classSig = NULL;
	err = param->jvmti->GetClassSignature(declaringClass, &classSig, NULL);
	if (err != JVMTI_ERROR_NONE)
    {
		if(Global::logger->isDebugEnabled())
		    Global::logger->logDebug("Failed to call GetClassSignature,error cd=%d",err);
		return false;
    } 
	AUTO_REL_JVMTI_OBJECT(classSig);

	char*	temp = HelperFunc::convertClassSig(classSig);
	methodData->setDeclaringClass(temp);
	if(Global::logger->isDebugEnabled())
	    Global::logger->logDebug("declaring class=%s",temp);

	delete temp;

    //get source file name
	char* sourceFileName = NULL;
	err = param->jvmti->GetSourceFileName(declaringClass,&sourceFileName);
	if (err == JVMTI_ERROR_NONE)
    {
		methodData->setSourceFile(sourceFileName);
		AUTO_REL_JVMTI_OBJECT(sourceFileName);
	}else
    {
		if(Global::logger->isDebugEnabled())
		    Global::logger->logDebug("Failed to call GetSourceFileName,error cd=%d",err);
	}
	*pClassSig = HelperFunc::strdup(classSig);	
	return true;
}

void Method::setMethodName(MethodData* methodData, char *methodName, char* methodSig)
{
	methodData->setName(methodName); 
	if(Global::logger->isDebugEnabled())
	{
		Global::logger->logDebug("methodName=%s",methodName);
	}

	char *temp;
    temp = HelperFunc::convertMethodSig(methodSig);
	methodData->setSignature(temp);
	if(Global::logger->isDebugEnabled())
	{
		Global::logger->logDebug("methodSignature=%s",temp);
	}
    delete temp;
}

//if has variable talbe return true,others return false;
bool Method::checkVariableTable(vector<jvmtiLocalVariableEntry *> *pEntryVec)
{
	jvmtiError err;
	bool bHasVarTable = true;
    jint count;
	err = param->jvmti->GetLocalVariableTable(param->frameInfo->method, &count, &table);
	if (err != JVMTI_ERROR_NONE)
	{
		if(Global::logger->isDebugEnabled())
		    Global::logger->logDebug("Failed to call GetLocalVariableTable,error code=%d",err);
		bHasVarTable = false;
    }
	if(bHasVarTable)
	{
		for (int index = 0; index < count; index++) 
		{
			pEntryVec->push_back(table + index);
		}

		sort(pEntryVec->begin(), pEntryVec->end(), EntryComp);
		if(Global::logger->isDebugEnabled())
		    Global::logger->logDebug("local variable entry =%d",count);
	}

	return bHasVarTable;
}

jint Method::getLocalVarNumberWithoutDebug(bool isStaticMethod,char** pVarTypes)
{
	jvmtiError err;
	jint count = 0;
	//get local variable number
	err = param->jvmti->GetMaxLocals(param->frameInfo->method, &count);
	if (err != JVMTI_ERROR_NONE)
	{
		if(Global::logger->isDebugEnabled())
		    Global::logger->logDebug("Failed to call GetMaxLocals,error code=%d",err);
		return 0;
	}

   	if(count > 0)
	{
		*pVarTypes = new  char[count + 1];
		memset(*pVarTypes,0,count+1);

		for(int i = 0;i < count;i++)
		{
			(*pVarTypes)[i] = 'U'; //unknown type
		}
 
		//add by jiang on 2007/1/28
		//the type of this shouldn't be unknown
		if(!isStaticMethod)
		{
			(*pVarTypes)[0]='L';
		}
	}else
	{
		return 0;
	}

	return count;
}

int Method::getArgumentSizeWithotDebug(bool isStaticMethod)
{
	jvmtiError err;
	int paramSlotNum = 0;
	jint slotNum;
	err = param->jvmti->GetArgumentsSize(param->frameInfo->method, &slotNum);
	//get the slot number where params occupy
	if (err != JVMTI_ERROR_NONE)
	{
		if(Global::logger->isDebugEnabled())
		    Global::logger->logDebug("Failed to call GetArgumentsSize,error code=%d",err);
		return -1;
	}

	if(!isStaticMethod)
	{
		paramSlotNum = slotNum - 1; //this is included in the param slot number
	}else
	{
        paramSlotNum = slotNum;
	}

	return paramSlotNum;
}

//ローカル変数テーブルに"this"があるかどうかチェックする関数
int Method::checkHasThis(vector<jvmtiLocalVariableEntry *> *pEntryVec,bool isStaticMethod,bool bHasVarTable)
{

	int hasThis = 0;
	if (bHasVarTable)
	{
		if (pEntryVec->size() > 0  && (*pEntryVec)[0]->slot == 0 
			   && strcmp("this", (*pEntryVec)[0]->name) == 0)
		{
			hasThis = 1;
		}
	}else
	{
		if(!isStaticMethod)
		{
			hasThis = 1;
		}
	}

	if(Global::logger->isDebugEnabled())
         Global::logger->logDebug("Has this object?%d",hasThis);
	return hasThis;
}

//add by Qiu Song on 20090817 for メソッド監視
//メソッドの戻り値を取得する関数
void Method::getMethodReturnValue()
{
	jint bytecode_count;  //byte code size
	unsigned char* bytecodes_ptr;  //point to byte code array
	jmethodID methodId = param->frameInfo->method;

   if(!HelperFunc::validateJvmtiError(param->jvmti,
		param->jvmti->GetBytecodes(methodId,&bytecode_count,&bytecodes_ptr),"GetBytecodes"))
	{
		LOG4CXX_WARN(Global::logger, "can't get byte code from jvmti for method");
		return;
    }
	if(bytecode_count > 0)
	{
        //create clas OpCodeScan
        OpCodeScan opCode(bytecode_count,bytecodes_ptr);
		unsigned preOpCode = 0;
		jint preOpCodePos = 0;

		//実行後のダンプかどうか判断する
		unsigned byteCodeCurrent = opCode.getCodeAtLocation(param->frameInfo->location);
		if(param->frameInfo->location <= 2 || byteCodeCurrent != opc_invokestatic)
		{
			//release the memory for  bytecodes_ptr
			param->jvmti->Deallocate((unsigned char*)bytecodes_ptr);
			return;
		}

		unsigned byteReturnCode = opCode.getCodeAtLocation(param->frameInfo->location + 3);
		if(byteReturnCode == opc_return || byteReturnCode == opc_athrow)
		{
			//release the memory for  bytecodes_ptr
			param->jvmti->Deallocate((unsigned char*)bytecodes_ptr);
			//returnと例外発生した場合、戻り値を取得しない
			return;
		}
		//callbacks関数前のopcodeを取得する
		if(opCode.getLastOpCodeBeforeCallbacks(param->frameInfo->location, preOpCode, preOpCodePos))
		{
			char valueType;
			int valueSlot;
			int nOpType = opCode.getReturnValueTypeAndSlot(preOpCode,preOpCodePos,valueType,valueSlot);

			//取得したvalueSlotの種別を判断する
			switch(nOpType)
			{
			//戻り値(opCodeはbipushの場合)
			case BIPUSH_RETURN:
				m_objValue = getReturnValueFromPushCode(preOpCode,valueSlot);
				break;
			//Var Table Index
			case VAR_RETURN:
                m_objValue = getReturnValueFromVarTable(valueType, valueSlot);
				setObjNameFromVarTable(param->frameInfo->location, valueSlot);
				break;
			//xconst_xの場合
			case CONST_RETURN:
				m_objValue = getReturnValueFromConstCode(preOpCode);
				break;
			default:
				break;
			}
			if(m_objDefinedType != NULL)
			{
				delete[] m_objDefinedType;
				m_objDefinedType = NULL;
			}
			m_objDefinedType = HelperFunc::getReturnTypeFromMethodSig(param->methodData->getSignature());
			HelperFunc::convertBoolValue(m_objDefinedType, 	m_objValue);
			HelperFunc::convertCharValue(m_objDefinedType, 	m_objValue);
			setReturnInfoToMethodData();
			releaseMemory();//add by Qiu Song on 20091127 for チケット:536
		}
	}
	//release the memory for  bytecodes_ptr
	param->jvmti->Deallocate((unsigned char*)bytecodes_ptr);
	return;
}

//Var Tableから戻り値を取得する
char* Method::getReturnValueFromVarTable(char valueType, int valueSlot)
{
	char* returnValue = NULL;
	switch(valueType)
	{
	case 'L':
		returnValue = getVarObjectValue(valueSlot);
		//オブジェクトを取得する
		break;
	case 'I':
		returnValue = getVarIntegerValue(valueSlot);
		break;
	case 'F':
		returnValue = getVarFloatValue(valueSlot);
		break;
	case 'D':
		returnValue = getVarDoubleValue(valueSlot);
		break;
	case 'J':
		returnValue = getVarLongValue(valueSlot);
		break;
	default:
		break;
	}

	return returnValue;
}

void Method::setObjNameFromVarTable(jlocation curPosition, int valueSlot)
{
	vector<jvmtiLocalVariableEntry *> entryVec;
    bool bHasVarTable = checkVariableTable(&entryVec);
	
	if(bHasVarTable && entryVec.size() > valueSlot)
	{
		for(int i = 1; i < entryVec.size() ; i++)
		{
			if(entryVec[i]->slot != valueSlot || curPosition < entryVec[i]->start_location || 
			   curPosition > (entryVec[i]->start_location + entryVec[i]->length))
			{
				continue;
			}
			if(m_objName != NULL)
			{
				delete[] m_objName;
				m_objName = NULL;
			}
			int nLength = strlen(entryVec[i]->name)+1;
			m_objName = new char[nLength];
			strcpy(m_objName, entryVec[i]->name);
			m_objName[nLength -1] = '\0';
			return;
		}
	}
}


//bipush/sipushから戻り値を取得する
char* Method::getReturnValueFromPushCode(unsigned opCode,int value)
{
	char buf[MAX_BUF_LEN];
	char *result;
	if(opCode == opc_bipush && value > 127)
	{
		sprintf(buf, "-%ld", (256 - value));
	}
	else if(opCode == opc_sipush && value > 32767)
	{
		sprintf(buf, "-%ld", (65536 - value));
	}
	else
	{
		sprintf(buf, "%ld", value);
	}
	result = new char[strlen(buf) + 1];
	strcpy(result, buf);
	return result;
}

//xconst_xコードから戻り値を取得する
char* Method::getReturnValueFromConstCode(unsigned opCode)
{
	char buf[MAX_BUF_LEN];
	char *result;

	switch(opCode)
	{
		case opc_aconst_null:
			sprintf(buf, "null");
			break;
		case opc_iconst_m1:
			sprintf(buf, "-1");
			break;
        case opc_iconst_0:
			sprintf(buf, "0");
			break;
		case opc_iconst_1:
			sprintf(buf, "1");
			break;
		case opc_iconst_2:
			sprintf(buf, "2");
			break;
		case opc_iconst_3:
			sprintf(buf, "3");
			break;
		case opc_iconst_4:
			sprintf(buf, "4");
			break;
		case opc_iconst_5:
			sprintf(buf, "5");
			break;
		case opc_lconst_0:
			HelperFunc::getLongTypeValue(0,buf);
			break;
		case opc_lconst_1:
			HelperFunc::getLongTypeValue(1,buf);
			break;
		case opc_fconst_0:
		case opc_dconst_0:
			sprintf(buf, "0.000000e+000");
			break;
		case opc_fconst_1:
		case opc_dconst_1:
			sprintf(buf, "1.000000e+000");
			break;
		case opc_fconst_2:
			sprintf(buf, "2.000000e+000");
			break;
	}
	result = new char[strlen(buf) + 1];
	strcpy(result, buf);
	return result;
}


char* Method::getVarObjectValue(int valueSlot)
{
	jobject object = NULL;
	ObjectData* objData = getVarObjectData(valueSlot, object);

	char* returnValue = NULL;
	if(objData != NULL)
	{
		//refIDと戻り値(あれば)を設定する
		m_objectRef = objData->getId();
		char* objDataReturnValue = objData->getValue();
		if(objDataReturnValue != NULL)
		{
			returnValue = new char[strlen(objDataReturnValue) + 1];
			strcpy(returnValue, objDataReturnValue);
		}
	}
	if(object == NULL)
	{
		if(returnValue != NULL)
		{
			delete returnValue;
		}
		returnValue = HelperFunc::getNullString();
	}
	return returnValue;
}

char* Method::getVarIntegerValue(int valueSlot)
{
	jvmtiError err = JVMTI_ERROR_NONE;
	char buf[MAX_BUF_LEN];
	jint value;
	err = JvmUtilFunc::myGetLocalInt(param->jvmti,param->thread, param->depth, valueSlot, &value);
	if(err == JVMTI_ERROR_NONE)
	{
		//it's integer ,byte ,short,boolean,char and we handle it as int
	   sprintf(buf, "%ld", value);
	   char *result = new char[strlen(buf) + 1];
	   strcpy(result, buf);
	   return result;
	}
	return NULL;
}

char* Method::getVarFloatValue(int valueSlot)
{
	jvmtiError err = JVMTI_ERROR_NONE;
	char buf[MAX_BUF_LEN];
	jfloat floatValue;
	err = JvmUtilFunc::myGetLocalFloat(param->jvmti,param->thread, param->depth, valueSlot, &floatValue);
	if(err == JVMTI_ERROR_NONE)
	{
		//it's float 
		sprintf(buf, "%e", floatValue);
		char *result = new char[strlen(buf) + 1];
		strcpy(result, buf);
		return result;
	}
	return NULL;
}

char* Method::getVarDoubleValue(int valueSlot)
{
	jvmtiError err = JVMTI_ERROR_NONE;
	char buf[MAX_BUF_LEN];
	jdouble doubleValue;
	err = JvmUtilFunc::myGetLocalDouble(param->jvmti,param->thread, param->depth, valueSlot, &doubleValue);
	if(err == JVMTI_ERROR_NONE)
	{
		//it's double 
		sprintf(buf, "%e", doubleValue);
		char *result = new char[strlen(buf) + 1];
		strcpy(result, buf);
		return result;
	}
	return NULL;
}

char* Method::getVarLongValue(int valueSlot)
{
	jvmtiError err = JVMTI_ERROR_NONE;
	char buf[MAX_BUF_LEN];
	jlong longValue;
	err = JvmUtilFunc::myGetLocalLong(param->jvmti,param->thread, param->depth, valueSlot, &longValue);
	if(err == JVMTI_ERROR_NONE)
	{
		//it's long 
		HelperFunc::getLongTypeValue(longValue,buf);
		char *result = new char[strlen(buf) + 1];
		strcpy(result, buf);
		return result;
	}
	return NULL;
}

ObjectData* Method::getVarObjectData(int valueSlot, jobject& object)
{
	jvmtiError err = JVMTI_ERROR_NONE;
	ObjectData* objData = NULL;
	int refId = -1;

	err = JvmUtilFunc::myGetLocalObject(param->jvmti,param->thread, param->depth,valueSlot,&object);
	//オブジェクトはNULLの場合
	if(err==JVMTI_ERROR_NONE && object != NULL)
	{
		char *realType = NULL;
		jclass clazz = param->jni->GetObjectClass(object);
		AUTO_REL_JNI_LOCAL_REF(param->jni, clazz);
		if(clazz !=NULL)
		{
			//オブジェクトタイプを取得する
			err = param->jvmti->GetClassSignature(clazz, &realType, NULL);
			AUTO_REL_JVMTI_OBJECT(realType);
		}
	}
	else
	{
		return NULL;
	}
	
	//当オブジェクトがプールにない場合、プールに追加
	refId = param->pool->getObjectId(object);
	if(refId < 0)
	{
         refId = param->pool->addObject(object);
		 objData = param->parent->getObjectData(refId);
         //ダンプするデータ量を増えた
		 param->parent->addDumpDataSize(objData->getDumpSize());
	}
	else
	{
		objData = param->parent->getObjectData(refId);
	}

	//展開可能の場合、オブジェクトを展開する
	if(!objData->getExpanded())
	{
		param->pool->expandObject(object,objData,param->parent,0);
	}

	return objData;
}

//取得した戻り値の関連情報をparam->methodDataに保存する
void Method::setReturnInfoToMethodData()
{
	param->methodData->setReturnType(m_objDefinedType);
	param->methodData->setReturnValue(m_objValue);
	param->methodData->setObjectRef(m_objectRef);
	param->methodData->setReturnName(m_objName);
}
//end of add by Qiu Song on 20090817 for メソッド監視

//add by Qiu Song on 20091127 for チケット:536
void Method::releaseMemory()
{
	if(m_objDefinedType != NULL)
	{
		delete m_objDefinedType;
		m_objDefinedType = NULL;
	}
	if(m_objValue != NULL)
	{
		delete m_objValue;
		m_objValue = NULL;
	}
	if(m_objName != NULL)
	{
		delete m_objName;
		m_objName = NULL;
	}
}
//end of add by Qiu Song on 20091127 for チケット:536