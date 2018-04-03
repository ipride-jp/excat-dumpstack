#include "Instance.h"
#include "../common/Global.h"
#include <vector>

USE_NS(NS_OUTPUT)
USE_NS(NS_COMMON)

using namespace std;

Instance::Instance(InstanceParam *param)
{
	this->param = param;
	init();
}

Instance::~Instance()
{
}

void Instance::init()
{
	if (param->objRefVec == NULL)
		return;

	int instanceCount = 0;

	vector<long>* objectRefV = new vector<long>;
	vector<long>* objectSizeV = new vector<long>;
	
	vector<jweak>::iterator it = param->objRefVec->begin();
	for (;it != param->objRefVec->end();it++)
	{
		//modified by Qiu Song on 2009.08.04 for インスタンスダンプ制限の削除
		if (param->maxInstanceCount != 0 && instanceCount >= param->maxInstanceCount)
		{
			break;
		}
        //end of modified by Qiu Song on 2009.08.04 for インスタンスダンプ制限の削除

		if (param->jni->IsSameObject(*it, NULL) == JNI_TRUE)
		{
			continue;
		}

		int refId = param->pool->getObjectId(*it);
		ObjectData* objData = NULL;
		if(refId < 0)
		{
			refId = param->pool->addObject(*it);
			objData = param->parent->getObjectData(refId);
			//ダンプするデータ量を増えた
			param->parent->addDumpDataSize(objData->getDumpSize());
			
		}else
		{
			objData = param->parent->getObjectData(refId);
		}
		//展開可能の場合、オブジェクトを展開する
		if(!objData->getExpanded())
		{
			param->pool->expandObject(*it,objData,param->parent,0);
		}
		jlong objectSize = 0;
		jvmtiError errcode = param->jvmti->GetObjectSize(*it, &objectSize);
		if (errcode != JVMTI_ERROR_NONE)
		{
			Global::logger->logError("Can't get object size from jvmti. %d", errcode);
			objectSize = -1;
		}
		objectRefV->push_back(refId);
		objectSizeV->push_back(objectSize);
		instanceCount++;
	}
	if (instanceCount == 0)
	{
		delete objectRefV;
		delete objectSizeV;
	}
	else
	{
		param->instanceData->setObjectRef(objectRefV);
		param->instanceData->setObjectSize(objectSizeV);
	}
}

