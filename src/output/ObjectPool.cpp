#include "ObjectData.h"
#include "StackTrace.h"
#include "ObjectPool.h"
#include "SuperClass.h"
#include "Attribute.h"

#include "../common/Global.h"
#include "../common/Define.h"
#include "../common/JniLocalRefAutoRelease.h"
#include "../common/JvmtiAutoRelease.h"
#include "../common/HelperFunc.h"
#include "TypedElement.h"
#include <time.h>

using namespace std;
#ifndef XERCESC20
XERCES_CPP_NAMESPACE_USE
#endif
USE_NS(NS_OUTPUT)
USE_NS(NS_COMMON)

const char *ObjectPool::TAG_OBJECT_POOL = "ObjectPool";
const char *ObjectPool::TAG_OBJECT = "Object";
const char *ObjectPool::ATTR_ID = "Id";
const char *ObjectPool::ATTR_TYPE = "Type";
const char *ObjectPool::ATTR_VALUE = "Value";

const char *ObjectPool::CLASS_STRING = "Ljava/lang/String;";

ObjectPool::ObjectPool(ObjectPoolParam *param)
{
	this->param = param;
    objNumber = 0;
    objectMap = new map<jlong, int>();
	//add by Qiu Song on 20090911 for 指定タイプオブジェクトのダンプ
	bNeedFilterObject = true;
	expendingObjectId = -1;
	//end of add by Qiu Song on 20090911 for 指定タイプオブジェクトのダンプ
}

ObjectPool::~ObjectPool()
{
	if(objectMap != NULL)
	{
       objectMap->clear();
	   delete objectMap;
	   objectMap = NULL;
	}
	if(Global::getIsIBMJvm())
	{
		if(objNumber <= 0){
			return;
		}
		//clear tag
		jvmtiError err;
		jint tag_count;
		jlong* tags;
		jint count_ptr;
		jobject* object_result_ptr;
    
		tag_count = objNumber;
		tags = new jlong[objNumber];
		for(jlong i = 0;i < objNumber;i++)
		{
			tags[i] = i + 1;
		}
		err = param->jvmti->GetObjectsWithTags(tag_count,tags,&count_ptr,
					  &object_result_ptr, NULL);

		if(err == JVMTI_ERROR_NONE)
		{
		   for(jint i = 0; i < count_ptr;i++)
		   {
				jobject obj = object_result_ptr[i];
				param->jvmti->SetTag(obj,0);
				param->jni->DeleteLocalRef(obj);
		   }
		   param->jvmti->Deallocate((unsigned char *)object_result_ptr);
		}else
		{
			Global::logger->logError("failed to call GetObjectsWithTags in ObjectPool::~ObjectPool(),error cd =%d.",err);
		}
   
		delete[] tags;
	}
}

int ObjectPool::addObject(jobject object)
{
	jvmtiError err;
	objNumber++;
	int id = objNumber - 1;
    jlong objHashCode = 0;//add by Qiu Song on 20091006 for HashCodeの追加
	if(Global::getIsIBMJvm())
	{
		//get tag
		jlong tag = 0;
		param->jvmti->GetTag(object,&tag);
		if(tag == 0)
		{
			tag = objNumber;
			param->jvmti->SetTag(object,tag);
		}
		objectMap->insert(pair<jlong,int>(tag,id));
		objHashCode = tag;//add by Qiu Song on 20091006 for HashCodeの追加
	}
	else
	{
		jint hashCode =0;
		err = param->jvmti->GetObjectHashCode(object,&hashCode);
		if(err != JVMTI_ERROR_NONE)
		{
			Global::logger->logError("failed to call GetObjectHashCode in ObjectPool::addObject,error cd =%d.",err);
			return id;//not exit
		}
		objectMap->insert(pair<jlong,int>(hashCode,id));
		objHashCode = hashCode;//add by Qiu Song on 20091006 for HashCodeの追加
	}

	ObjectData* objData = new ObjectData();
	param->parent->addObjectData(objData);
	
    objData->setId(id);
	//add by Qiu Song on 20091006 for HashCodeの追加
	objData->setHashCode(objHashCode);
	//end of add by Qiu Song on 20091006 for HashCodeの追加

	//get object signature
	jclass clazz = param->jni->GetObjectClass(object);
	if(clazz == NULL)
    {
		Global::logger->logError("failed to call GetObjectClass in ObjectPool::addObject.");
		objData->setType("Unknown class");
		objData->setExpanded(true);
		return id;//not exit
	}
    AUTO_REL_JNI_LOCAL_REF(param->jni, clazz);	

	//get class status
	jint status;
	err = param->jvmti->GetClassStatus(clazz,&status);
	if(err != JVMTI_ERROR_NONE)
	{
		Global::logger->logError("failed to call GetClassStatus in ObjectPool::addObject,error cd =%d.",err);
		objData->setType("Unknown class");
		objData->setExpanded(true);
		return id;//not exit
	}

    //IBM JDKの場合、statusが判断不要
	if(!Global::getIsIBMJvm())
	{	
		if(status == 0)
		{
			//unsafe class 
			objData->setType("Unsafe class");
			objData->setExpanded(true);
			return id;
		}
	}
    
	char *sig = NULL;
	err = param->jvmti->GetClassSignature(clazz, &sig, NULL);
	if(err != JVMTI_ERROR_NONE)
	{
		Global::logger->logError("failed to call GetClassSignature in ObjectPool::addObject,error cd =%d.",err);
		objData->setType("Unknown class");
		objData->setExpanded(true);
		return id;//not exit
	}
	AUTO_REL_JVMTI_OBJECT(sig);

	//get object value
	char buf[MAX_BUF_LEN*4];
	memset(buf, 0, MAX_BUF_LEN*4);	
	char *value = NULL;
	if(strcmp(sig, CLASS_STRING) == 0)
	{
	
		HelperFunc::getStringObjectValue(param->jni, object, buf, sizeof(buf));
		value = buf;
	}

	//add attribute type
	char *javaClassName = HelperFunc::convertClassSig(sig);
	objData->setType(javaClassName);
    delete javaClassName;

	//add attribute value
	if (value != NULL)
	{
		objData->setValue(value);
		objData->setExpanded(true);
    }else
	{
		objData->setValue(NULL);
        if(*sig == '[')
		{
			//配列?
			objData->setIsArrayObject(true);
			jsize size = param->jni->GetArrayLength((jarray)object);
			objData->setItemNumber(size);
			if(size == 0)
			{
                objData->setExpanded(true);
			}
        }
	}
	return id;
}

int ObjectPool::getDumpArraySize(int arraySize,bool isObject)
{
	OutputSetting *setting = Global::getConfig()->getOutputSetting();
	int dumpSize = 0;
	if(isObject)
	{
		dumpSize = setting->getMaxArrayElementForObject();
	}else
	{
        dumpSize = setting->getMaxArrayElementForPrimitive();
	}

	return dumpSize < arraySize?dumpSize:arraySize;
}

void ObjectPool::addArrayElements(TypedElementParam *paramArr,StackTrace *ancestor)
{
	paramArr->objectRef = -1;
	char *type = paramArr->objDefinedType;
	jsize size = paramArr->jni->GetArrayLength((jarray)paramArr->object);
    bool expandedOnce = paramArr->parent->getExpandedOnce();

	if (strcmp(type, "Z") == 0) //boolean
	{
		if(!expandedOnce)
		{
			jboolean* elems = paramArr->jni->GetBooleanArrayElements(
				(jbooleanArray)paramArr->object, 0);
			if(elems == NULL)
			{
				Global::logger->logError("failed to call GetBooleanArrayElements in ObjectPool::addArrayElements.");
				return;
			}
			jbooleanArray objectBak = (jbooleanArray)paramArr->object;
			int dumpSize = getDumpArraySize(size,false);
			for (int index = 0; index < dumpSize; index++)
			{
				jboolean* value = elems + index;

				paramArr->object = NULL;
				paramArr->objValue = (char*)(*value == 0 ? "false" : "true");
				paramArr->arrIndex = index;
				TypedElement typedElem(paramArr, TypedElement::TAG_ITEM);
			}

			paramArr->jni->ReleaseBooleanArrayElements(objectBak, elems, 0);
		}

	}
	else if (strcmp(type, "B") == 0)  //byte
	{
		if(!expandedOnce)
		{
			jbyte* elems = paramArr->jni->GetByteArrayElements(
				(jbyteArray)paramArr->object, 0);
			if(elems == NULL)
			{
				Global::logger->logError("failed to call GetByteArrayElements in ObjectPool::addArrayElements.");
				return;
			}
			jbyteArray objectBak = (jbyteArray)paramArr->object;
			int dumpSize = getDumpArraySize(size,false);
			for (int index = 0; index < dumpSize; index++)
			{
				jbyte* value = elems + index;

				paramArr->object = NULL;

				char buf[MAX_BUF_LEN];
				memset(buf, 0, MAX_BUF_LEN);
				sprintf(buf, "%d", *value);
				paramArr->objValue = buf;
				paramArr->arrIndex = index;			
				TypedElement typedElem(paramArr, TypedElement::TAG_ITEM);
			}
			paramArr->jni->ReleaseByteArrayElements(objectBak, elems, 0);

		}

	}
	else if (strcmp(type, "C") == 0) //char
	{
		if(!expandedOnce)
		{
			jchar* elems = paramArr->jni->GetCharArrayElements(
				(jcharArray)paramArr->object, 0);
			if(elems == NULL)
			{
				Global::logger->logError("failed to call GetCharArrayElements in ObjectPool::addArrayElements.");
				return;
			}
			jcharArray objectBak = (jcharArray)paramArr->object;
			int dumpSize = getDumpArraySize(size,false);
			for (int index = 0; index < dumpSize; index++)
			{
				jchar *value = elems + index;

				paramArr->object = NULL;

				char buf[MAX_BUF_LEN];
				memset(buf, 0, MAX_BUF_LEN);
				sprintf(buf, "%04x", *value);
				
				paramArr->objValue = buf;
				paramArr->arrIndex = index;			
				TypedElement typedElem(paramArr, TypedElement::TAG_ITEM);
			}
			paramArr->jni->ReleaseCharArrayElements(objectBak, elems, 0);
	
		}

	}
	else if (strcmp(type, "D") == 0) //double
	{
		if(!expandedOnce)
		{
			jdouble* elems = paramArr->jni->GetDoubleArrayElements(
				(jdoubleArray)paramArr->object, 0);
			if(elems == NULL)
			{
				Global::logger->logError("failed to call GetDoubleArrayElements in ObjectPool::addArrayElements.");
				return;
			}
			jdoubleArray objectBak = (jdoubleArray)paramArr->object;
			int dumpSize = getDumpArraySize(size,false);
			for (int index = 0; index < dumpSize; index++)
			{
				jdouble* value = elems + index;
				paramArr->object = NULL;

				char buf[MAX_BUF_LEN];
				memset(buf, 0, MAX_BUF_LEN);
				sprintf(buf, "%e", *value);
				paramArr->objValue = buf;
				paramArr->arrIndex = index;			
				TypedElement typedElem(paramArr, TypedElement::TAG_ITEM);
			}
			paramArr->jni->ReleaseDoubleArrayElements(objectBak, elems, 0);
		}
	}
	else if (strcmp(type, "F") == 0) //float
	{
		if(!expandedOnce)
		{
			jfloat* elems = paramArr->jni->GetFloatArrayElements(
				(jfloatArray)paramArr->object, 0);
			if(elems == NULL)
			{
				Global::logger->logError("failed to call GetFloatArrayElements in ObjectPool::addArrayElements.");
				return;
			}
			jfloatArray objectBak = (jfloatArray)paramArr->object;
			int dumpSize = getDumpArraySize(size,false);
			for (int index = 0; index < dumpSize; index++)
			{
				jfloat* value = elems + index;
				paramArr->object = NULL;

				char buf[MAX_BUF_LEN];
				memset(buf, 0, MAX_BUF_LEN);
				sprintf(buf, "%e", *value);
				paramArr->objValue = buf;
				paramArr->arrIndex = index;			
				TypedElement typedElem(paramArr, TypedElement::TAG_ITEM);
			}
			paramArr->jni->ReleaseFloatArrayElements(objectBak, elems, 0);
			//paramArr->jni->DeleteLocalRef(objectBak);
		}
	}
	else if (strcmp(type, "I") == 0) //int
	{
		if(!expandedOnce)
		{
			jint* elems = paramArr->jni->GetIntArrayElements(
				(jintArray)paramArr->object, 0);
			if(elems == NULL)
			{
				Global::logger->logError("failed to call GetIntArrayElements in ObjectPool::addArrayElements.");
				return;
			}
			jintArray objectBak = (jintArray)paramArr->object;
			int dumpSize = getDumpArraySize(size,false);
			for (int index = 0; index < dumpSize; index++)
			{
				jint* value = elems + index;
				paramArr->object = NULL;

				char buf[MAX_BUF_LEN];
				memset(buf, 0, MAX_BUF_LEN);
				sprintf(buf, "%d", *value);
				paramArr->objValue = buf;
				paramArr->arrIndex = index;			
				TypedElement typedElem(paramArr, TypedElement::TAG_ITEM);
			}
			paramArr->jni->ReleaseIntArrayElements(objectBak, elems, 0);
			//paramArr->jni->DeleteLocalRef(objectBak);
		}
	}
	else if (strcmp(type, "S") == 0) //short
	{
		if(!expandedOnce)
		{
			jshort* elems = paramArr->jni->GetShortArrayElements(
				(jshortArray)paramArr->object, 0);
			if(elems == NULL)
			{
				Global::logger->logError("failed to call GetShortArrayElements in ObjectPool::addArrayElements.");
				return;
			}
			jshortArray objectBak = (jshortArray)paramArr->object;
			int dumpSize = getDumpArraySize(size,false);
			for (int index = 0; index < dumpSize; index++)
			{
				jshort* value = elems + index;
				paramArr->object = NULL;

				char buf[MAX_BUF_LEN];
				memset(buf, 0, MAX_BUF_LEN);
				sprintf(buf, "%d", *value);
				paramArr->objValue = buf;
				paramArr->arrIndex = index;			
				TypedElement typedElem(paramArr, TypedElement::TAG_ITEM);
			}
			paramArr->jni->ReleaseShortArrayElements(objectBak, elems, 0);
		
		}

	}
	else if (strcmp(type, "J") == 0) //long
	{
		if(!expandedOnce)
		{
			jlong* elems = paramArr->jni->GetLongArrayElements(
				(jlongArray)paramArr->object, 0);
			if(elems == NULL)
			{
				Global::logger->logError("failed to call GetLongArrayElements in ObjectPool::addArrayElements.");
				return;
			}
			jlongArray objectBak = (jlongArray)paramArr->object;
			int dumpSize = getDumpArraySize(size,false);
			for (int index = 0; index < dumpSize; index++)
			{
				jlong* value = elems + index;
				paramArr->object = NULL;

				char buf[MAX_BUF_LEN];
				memset(buf, 0, MAX_BUF_LEN);
				HelperFunc::getLongTypeValue(*value,buf);
				paramArr->objValue = buf;
				paramArr->arrIndex = index;			
				TypedElement typedElem(paramArr, TypedElement::TAG_ITEM);
			}
			paramArr->jni->ReleaseLongArrayElements(objectBak, elems, 0);
		}

	}
	else
	{
		jobject objectBak = (jobject)paramArr->object;
		int dumpSize = getDumpArraySize(size,true);
		for (int index = 0; index < dumpSize; index++)
		{
			paramArr->objectRef = -1;
			jobject objSub = paramArr->jni->GetObjectArrayElement((jobjectArray)objectBak, 
				index);
            paramArr->object = objSub;
			AUTO_REL_JNI_LOCAL_REF(param->jni, objSub);

			if (objSub != NULL)
			{
				
				int objectRef = this->getObjectId(objSub);
			    ObjectData* objData = NULL;
				if(objectRef < 0)
				{
					objectRef = this->addObject(objSub);
					objData = ancestor->getObjectData(objectRef);
					//ダンプするデータ量を増えた
					ancestor->addDumpDataSize(objData->getDumpSize());
				}else
				{
					objData = ancestor->getObjectData(objectRef);
				}
                paramArr->objectRef = objectRef;
  
				//展開可能の場合、オブジェクトを展開する
				if(!objData->getExpanded())
				{
				    this->expandObject(objSub,objData,ancestor,
						Global::curAttrDepth);
				}

			}
			if(!expandedOnce)
            {
				paramArr->objValue = NULL;
				paramArr->arrIndex = index;
				
				TypedElement typedElem(paramArr, TypedElement::TAG_ITEM);
			}
		}
	}
}
void ObjectPool::expandObject(jobject object,ObjectData* objData,StackTrace *ancestor,
							  int currentLayer)
{
	//we want to know this object in which layer
    int layerNumber = objData->getLayersNumber();
    if(layerNumber >= 0)
	{
		if(layerNumber <= currentLayer)
		{
			return;
		}else
		{
            layerNumber = currentLayer;
		}
	}else
	{
        layerNumber = currentLayer;
	}

	objData->setLayersNumber(layerNumber);

	//子属性を展開する
 	//get object signature
	jvmtiError err;
	jclass clazz = param->jni->GetObjectClass(object);
	if(clazz == NULL)
    {
		Global::logger->logError("failed to call GetObjectClass in ObjectPool::expandObject.");
		return;//not exit
	}

    AUTO_REL_JNI_LOCAL_REF(param->jni, clazz);
	char *sig = NULL;
	err = param->jvmti->GetClassSignature(clazz, &sig, NULL);

	if(err != JVMTI_ERROR_NONE)
	{
		Global::logger->logError("failed to call GetClassSignature in ObjectPool::expandObject,error cd =%d.",err);
		return ;//not exit
	}
	AUTO_REL_JVMTI_OBJECT(sig);


	//add by Qiu Song on 20090903 for 指定タイプのオブジェクトが値のみダンプされる
	int nPos = objectMap->size();
	int nObjNumberBak = objNumber;
	if(bNeedFilterObject == true)
	{
		char* objValue = objData->getValue();
		if(HelperFunc::isFilterObjType(objData->getType()) || (objValue != NULL && strcmp(objValue, "null") == 0) )
		{
		    bNeedFilterObject = false;
			expendingObjectId = objData->getId();
		}
	}
	//end of add by Qiu Song on 20090903 for 指定タイプのオブジェクトが値のみダンプされる

	if(*sig == '[')
	{
		//配列
		jsize size = objData->getItemNumber();
        if(size > 0) 
		{
			TypedElementParam* paramArr = new TypedElementParam();
			paramArr->jni = param->jni;
			paramArr->jvmti = param->jvmti;
			paramArr->ancestor = param->parent;
			paramArr->object = object;
			paramArr->objectRef = -1;
			paramArr->objName = "";
			paramArr->objValue = NULL;
			paramArr->parent = objData;
			paramArr->valid = true;
			paramArr->objDefinedType = sig + 1;
			paramArr->pool = this;
			addArrayElements(paramArr,param->parent);
			delete paramArr;
			objData->setExpandedOnce(true);
		}
	}
	else
	{
		//add object's attributes
		AttributeParam* paramAttr = new AttributeParam();
		paramAttr->jni = param->jni;
		paramAttr->jvmti = param->jvmti;
		paramAttr->ancestor = param->parent;
		paramAttr->parent = objData;
		paramAttr->clazz = clazz;
		paramAttr->object = object;
		paramAttr->pool = this;
		paramAttr->className = objData->getType();
		Attribute attr(paramAttr);

	   if(attr.getExpandedOnce())
		{
            objData->setExpandedOnce(true);
		}
		delete paramAttr;

		//get super class fields
		jclass superClazz = param->jni->GetSuperclass(clazz);
		AUTO_REL_JNI_LOCAL_REF(param->jni, superClazz);
		if (superClazz != NULL)
		{
			SuperClassParam* paramSC = new SuperClassParam();
			paramSC->jni = param->jni;
			paramSC->jvmti = param->jvmti;
			paramSC->ancestor = param->parent;
			paramSC->thread = param->thread;
			paramSC->object = object;
			paramSC->clazz = superClazz;
			paramSC->parent = objData;
			paramSC->pool = this;
			SuperClass superClass(paramSC);
			delete paramSC;
		}
    }
	//add by Qiu Song on 20090903 for 指定タイプのオブジェクト値のみダンプ
	if(bNeedFilterObject == false && expendingObjectId == objData->getId())
	{
		bNeedFilterObject = true;
	}
	if(bNeedFilterObject == true)
	{
		filterUselessObject(ancestor, objData, sig, nPos, nObjNumberBak);
	}
	//end of add by Qiu Song on 20090903 for 指定タイプのオブジェクト値のみダンプ
}

int ObjectPool::getObjectId(jobject object)
{
	if(Global::getIsIBMJvm())
	{
		jlong tag =0;
		jvmtiError err;
		err = param->jvmti->GetTag(object,&tag);
		if(err != JVMTI_ERROR_NONE)
		{
			Global::logger->logError("failed to call GetTag in ObjectPool::getObjectId,error cd =%d.",err);
			return -1;//not exit
		}
		if(tag == 0)
		{
			return -1;
		}
		map<jlong, int>::const_iterator p; //反復子
		p = objectMap->find(tag);
		if(p != objectMap->end())
		{
			return p->second;
			
		}else
		{
			return -1;
		}
	}
	else
	{
		jint hashCode =0;
		jvmtiError err;
		err = param->jvmti->GetObjectHashCode(object,&hashCode);
		if(err != JVMTI_ERROR_NONE)
		{
			Global::logger->logError("failed to call GetObjectHashCode in ObjectPool::addObject,error cd =%d.",err);
			return -1;//not exit
		}
		map<jlong, int>::const_iterator p; //反復子

		p = objectMap->find(hashCode);
		if(p != objectMap->end())
		{
			return p->second;
			
		}else
		{
			return -1;
		}
	}
}

//add by Qiu Song on 20090903 for 指定タイプのオブジェクトを値のみダンプ
void ObjectPool::filterUselessObject(StackTrace* ancestor, ObjectData* objData, char* sig, int nPos, int nObjNumberBak)
{
	char* objValue = objData->getValue();
	if(HelperFunc::isFilterObjType(objData->getType()) || (objValue != NULL && strcmp(objValue, "null") == 0))
	{
		setDumpObjValue(objData, ancestor);
		if(*sig != '[')
		{
			//無駄なattributeを削除する
			objData->removeAllAttribute();
		}

		//無駄なオブジェクトを削除する
		removeObjectMapFromPos(nPos);
		ancestor->removeObjectFromPos(nPos);
		objNumber = nObjNumberBak;
		objData->setExpandedOnce(false);
		objData->setSuperClass(NULL);
	}
}

void ObjectPool::setDumpObjValue(ObjectData* objData, StackTrace* ancestor)
{

	char* objType = objData->getType();

	if(strcmp(objType,"java.math.BigInteger") == 0)
	{
		//値を取得する
		setBigIntegerValue(objData,ancestor);
	}
    else if(strcmp(objType,"java.math.BigDecimal") == 0)
	{
		//値を取得する
		setBigDecimalValue(objData,ancestor);
	}
	else if(strcmp(objType,"java.lang.StringBuffer") == 0 || strcmp(objType,"java.lang.StringBuilder") == 0)
	{
		//値を取得する
		setStringBufferValue(objData,ancestor);
	}
 	else if(strcmp(objType,"java.util.Date") == 0 || strcmp(objType,"java.sql.Timestamp") == 0 ||
		    strcmp(objType,"java.sql.Time") == 0 )
	{
		setDateValue(objData,ancestor);
	}
	else if(strcmp(objType,"java.lang.Integer") == 0   || strcmp(objType,"java.lang.Byte") == 0  ||
			strcmp(objType,"java.lang.Double") == 0    || strcmp(objType,"java.lang.Float") == 0 ||
			strcmp(objType,"java.lang.Long") == 0      || strcmp(objType,"java.lang.Short") == 0 ||
			strcmp(objType,"java.lang.Character") == 0 || strcmp(objType,"java.lang.Boolean") == 0)

	{
		//値を取得する
		setCommonObjValue(objData,ancestor);
	}
	else if(objType[strlen(objType) - 1] == ']')
	{
		//値を取得する
		setArrayObjValue(objData,ancestor);
	}
}

void ObjectPool::setBigIntegerValue(ObjectData* objData, StackTrace* ancestor)
{
	TypeElementData* sigNumObj = objData->getAttributeByName("signum");
	char* sigNum = NULL;
	if(sigNumObj == NULL)
	{
		return;
	}
	else
	{
		sigNum = sigNumObj->getValue();
		if(sigNum == NULL)
		{
			return;
		}
		else if(strcmp(sigNum, "0") == 0)
		{
			objData->setValue(sigNum);
		}
	}
	TypeElementData* valueObj = objData->getAttributeByName("mag");
	if(valueObj == NULL)
	{
		return;
	}
	ObjectData* refData = NULL;
	int nRefId = valueObj->getRefId();
	refData = ancestor->getObjectData(nRefId);
	if(refData != NULL)
	{
		char objValueTemp[MAX_BUF_LEN];
		memset(objValueTemp, 0, MAX_BUF_LEN);
		valueObj = refData->getAttribute(0);
		if(valueObj == NULL)
		{
			return;
		}
		char* atrValueTemp = valueObj->getValue();
		if(atrValueTemp != NULL)
		{
			if(strcmp(sigNum, "-1") == 0)
			{
				objValueTemp[0] = '-';
			}
			strcat(objValueTemp,atrValueTemp);
			objValueTemp[strlen(atrValueTemp)+1] = '\0';
			objData->setValue(objValueTemp);
		}
	}
}

void ObjectPool::setBigDecimalValue(ObjectData* objData, StackTrace *ancestor)
{
	TypeElementData* scaleObj = objData->getAttributeByName("scale");
	TypeElementData* valueObj = objData->getAttributeByName("intCompact");
	if(scaleObj == NULL || valueObj == NULL)
	{
		return;
	}
	int nScale = atoi(scaleObj->getValue());
    char* attrValue = valueObj->getValue();

	char objValueTemp[MAX_BUF_LEN];
	memset(objValueTemp, 0, MAX_BUF_LEN);
	int nValueLength = strlen(attrValue);
    if(nScale >= nValueLength)
	{
		sprintf(objValueTemp, "0.%s",attrValue);
		objValueTemp[nValueLength+3] = '\0';
		objData->setValue(objValueTemp);
	}
	else
	{
		char objScaleValueTemp[MAX_BUF_LEN];
	    memset(objScaleValueTemp, 0, MAX_BUF_LEN);
		strncpy(objValueTemp, attrValue, nValueLength - nScale);
		if(nScale > 0)
		{
			strncpy(objScaleValueTemp, attrValue + (nValueLength - nScale), nScale);
			strcat(objValueTemp,".");
			strcat(objValueTemp,objScaleValueTemp);
		}
		objData->setValue(objValueTemp);
	}
}

void ObjectPool::setStringBufferValue(ObjectData* objData, StackTrace *ancestor)
{

	TypeElementData* valueObj = objData->getSuperClass()->getAttributeByName("value");
	if(valueObj == NULL)
	{
		return;
	}
	ObjectData* refData = NULL;
	int nRefId = valueObj->getRefId();
	refData = ancestor->getObjectData(nRefId);

	if(refData == NULL)
	{
		return;
	}
	char objValueTemp[MAX_STRING_LENGTH];
	memset(objValueTemp, 0, MAX_STRING_LENGTH);

	vector<TypeElementData*>* attrs = refData->getSonVector();
	if(attrs == NULL)
	{
		return;
	}

	vector<TypeElementData*>::iterator iter;
	for(iter = attrs->begin(); iter != attrs->end(); iter++)
	{
		TypeElementData * typeElementData = (TypeElementData *)(*iter);
		char* elementValue = typeElementData->getValue();
		if(strcmp(elementValue, "0000") == 0)
		{
			break;
		}
		strcat(objValueTemp,elementValue);
	}

	objValueTemp[strlen(objValueTemp) + 1] = '\0';
	objData->setValue(objValueTemp);
}

void ObjectPool::setDateValue(ObjectData* objData, StackTrace *ancestor)
{
	TypeElementData* valueObj = NULL;
	if(strcmp(objData->getType(), "java.sql.Timestamp") == 0 || strcmp(objData->getType(), "java.sql.Time") == 0)
	{
		if(objData->getSuperClass() == NULL)
		{
			return;
		}
		valueObj = objData->getSuperClass()->getAttributeByName("fastTime");
	}
	else
	{
		valueObj = objData->getAttributeByName("fastTime");
	}
	if(valueObj == NULL)
	{
		return;
	}

	char timeValueTemp[MAX_BUF_LEN];
	memset(timeValueTemp, 0, MAX_BUF_LEN);
	char* atrValueTemp = valueObj->getValue();
	if(atrValueTemp == NULL)
	{
		return;
	}

	time_t time1 = 0;
	if(strlen(atrValueTemp) > 3)
	{
		memcpy(timeValueTemp, atrValueTemp, strlen(atrValueTemp) -3);
		time1 = atol(timeValueTemp);
	}

	char objValueTemp[MAX_BUF_LEN];
	memset(objValueTemp, 0, MAX_BUF_LEN);


	if(localtime(&time1) == NULL)
	{
		return;
	}
	if(strcmp(objData->getType(), "java.sql.Time") == 0)
	{
		strftime(objValueTemp,9, "%H:%M:%S", localtime(&time1));
	}
	else
	{
		strftime(objValueTemp,63, "%Y/%m/%d %H:%M:%S", localtime(&time1));
	}
	objValueTemp[strlen(objValueTemp)+1] = '\0';
 	objData->setValue(objValueTemp);
}

void ObjectPool::setCommonObjValue(ObjectData* objData, StackTrace *ancestor)
{
	TypeElementData* valueObj = objData->getAttributeByName("value");
	if(valueObj == NULL)
	{
		return;
	}
	char objValueTemp[MAX_BUF_LEN];
	memset(objValueTemp, 0, MAX_BUF_LEN);
	char* atrValueTemp = valueObj->getValue();
	strcpy(objValueTemp,atrValueTemp);
	objValueTemp[strlen(atrValueTemp)+1] = '\0';
	objData->setValue(objValueTemp);
}

void ObjectPool::setArrayObjValue(ObjectData* objData, StackTrace *ancestor)
{
	vector<TypeElementData*>* attrs = objData->getSonVector();
	if(attrs == NULL)
	{
		return;
	}
	vector<TypeElementData*>::iterator iter;
	for(iter = attrs->begin(); iter != attrs->end(); iter++)
	{
		TypeElementData * typeElementData = (TypeElementData *)(*iter);
		if(typeElementData != NULL)
		{
			int nRefId = typeElementData->getRefId();
			if(nRefId > 0)
			{
				ObjectData* refData = ancestor->getObjectData(nRefId);
				if(refData !=NULL)
				{
					char objValueTemp[MAX_STRING_LENGTH];
				    memset(objValueTemp, 0, MAX_STRING_LENGTH);
					setDumpObjValue(refData, ancestor);
				    char* atrValueTemp = refData->getValue();
					if(atrValueTemp != NULL)
					{
						int nMaxLength = strlen(atrValueTemp) > MAX_STRING_LENGTH-1 ? MAX_STRING_LENGTH -1 : strlen(atrValueTemp);
						//memcpy(objValueTemp, atrValueTemp, strlen(atrValueTemp));
						//objValueTemp[strlen(atrValueTemp)+1] = '\0';
						
						memcpy(objValueTemp, atrValueTemp, nMaxLength);
						objValueTemp[nMaxLength] = '\0';
						typeElementData->setValue(objValueTemp);
					}
					else
					{
						typeElementData->setValue(NULL);
					}
				}
				typeElementData->setObjectRef(-1);
			}
		}
	}
}

void ObjectPool::removeObjectMapFromPos(int nPos)
{
	if(objectMap == NULL)
	{
		return;
	}
	map<jlong,int>::iterator itMap = objectMap->begin();
	for ( ; itMap != objectMap->end(); ) 
	{
		if(itMap->second >= nPos)
		{
			objectMap->erase( itMap++ );
		}
		else
		{
			itMap++;
		}
	}
}
//end of add by Qiu Song on 20090903 for 指定タイプのオブジェクトを値のみダンプ