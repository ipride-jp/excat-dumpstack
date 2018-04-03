#include "ObjectData.h"
#include "TypedElement.h"
#include "../common/Global.h"
#include "../common/SystemInfo.h"
#include "../common/HelperFunc.h"
#include "../common/JniLocalRefAutoRelease.h"
#include "../common/JvmtiAutoRelease.h"
#include <locale.h>
#include <stdlib.h>
#include <wchar.h>

USE_NS(NS_OUTPUT)
USE_NS(NS_COMMON)

XMLData::XMLData()
{
}

XMLData::~XMLData()
{
}

//because we don't know the size of the bytes needed for the esacaped bytes
//we'll calculate the size needed
int XMLData::calEscapeXMLChar(const char* str)
{
    int i = 0;
	int len = strlen(str);
	int maxsize = len;

	while(i < len)
	{
		if(str[i] == '<' )
		{
            maxsize+=3; //replace with &lt;
      	}else
        if(str[i] == '>')
		{
			maxsize+=3; //replace with &gt;
    	}else
        if(str[i] == '&' )
		{
			bool isEscaped = false;
          
			if((i < len -3 && str[i+1] == 'l' && str[i+2] == 't' && str[i+3] == ';') || 
               (i < len -3 && str[i+1] == 'g' && str[i+2] == 't' && str[i+3] == ';') ||
               (i < len -5 && str[i+1] == 'q' && str[i+2] == 'u' && str[i+3] == 'o' && str[i+4] == 't' && str[i+5] == ';') ||
               (i < len -5 && str[i+1] == 'a' && str[i+2] == 'p' && str[i+3] == 'o' && str[i+4] == 's' && str[i+5] == ';') ||
               (i < len -4 && str[i+1] == 'a' && str[i+2] == 'm' && str[i+3] == 'p' && str[i+4] == ';'))
			{
                isEscaped = true;
			}
            if(!isEscaped)
			{
				maxsize+=4; //replace with &amp;
			}
        }else
        if(str[i] == '"')
		{
			maxsize+=5; //replace with &quot;
     
        }else
        if(str[i] == '\'')
		{
			maxsize+=5; //replace with &apos;
      	}
		else
		{
			//特殊文字
            if(str[i] >= 0 && str[i] <= 0xf)
            {
			    maxsize+=1; //replace with \0-f
         	}else if(str[i] >= 0x10 && str[i] <= 0x1f)
            {
				maxsize+=2; //replace with \10-1f
     		}else if(str[i] == 0x7f)
			{
				maxsize+=2; //replace with \7f
			}
		}
		i++;
	}

    return maxsize;
}

//because we don't know the size of the bytes needed for the esacaped bytes
//you should call calEscapeXMLChar first
void XMLData::escapeXMLChar(const char* str,char ** bufObj,int maxsize)
{
	char* buf = new char[maxsize + 1];

    int i = 0;
	int j = 0;
	int len = strlen(str);
	while(i < len)
	{
		if(str[i] == '<' )
		{
            buf[j++] = '&';
            buf[j++] = 'l';
			buf[j++] = 't';
			buf[j++] = ';';
		}else
        if(str[i] == '>' )
		{
            buf[j++] = '&';
            buf[j++] = 'g';
			buf[j++] = 't';
			buf[j++] = ';';
		}else
        if(str[i] == '&' )
		{
			bool isEscaped = false;
          
			if((i < len -3 && str[i+1] == 'l' && str[i+2] == 't' && str[i+3] == ';') || 
               (i < len -3 && str[i+1] == 'g' && str[i+2] == 't' && str[i+3] == ';') ||
               (i < len -5 && str[i+1] == 'q' && str[i+2] == 'u' && str[i+3] == 'o' && str[i+4] == 't' && str[i+5] == ';') ||
               (i < len -5 && str[i+1] == 'a' && str[i+2] == 'p' && str[i+3] == 'o' && str[i+4] == 's' && str[i+5] == ';') ||
               (i < len -4 && str[i+1] == 'a' && str[i+2] == 'm' && str[i+3] == 'p' && str[i+4] == ';'))
			{
                isEscaped = true;
			}
            if(!isEscaped)
			{
				buf[j++] = '&';
				buf[j++] = 'a';
				buf[j++] = 'm';
				buf[j++] = 'p';
				buf[j++] = ';';
			}
			else
			{
				buf[j++] = str[i];
			}

        }else
        if(str[i] == '"')
		{
            buf[j++] = '&';
            buf[j++] = 'q';
			buf[j++] = 'u';
			buf[j++] = 'o';
			buf[j++] = 't';
			buf[j++] = ';';
        }else
        if(str[i] == '\'' )
		{
            buf[j++] = '&';
            buf[j++] = 'a';
			buf[j++] = 'p';
			buf[j++] = 'o';
			buf[j++] = 's';
			buf[j++] = ';';

		}
		else
		{
	        if(str[i] >= 0 && str[i] <= 0x9)
            {
                  buf[j++] = '\\';
                  buf[j++] = str[i] - 0 + '0';  //\0-9
			}else if(str[i] >= 0xa && str[i] <= 0xf)
			{
                  buf[j++] = '\\';
                  buf[j++] = str[i] - 0xa + 'a';  //\0-9
			}
			else if(str[i] >= 0x10 && str[i] <= 0x19)
            {
                  buf[j++] = '\\';
                  buf[j++] = '1';
                  buf[j++] = str[i] - 0x10 + '0';  //\10-1f
			}else if(str[i] >= 0x1a && str[i] <= 0x1f)
			{
                  buf[j++] = '\\';
                  buf[j++] = '1';
                  buf[j++] = str[i] - 0x1a + 'a';  //\10-1f
			}
			else if(str[i] == 0x7f)
			{
                  buf[j++] = '\\';
                  buf[j++] = '7';
                  buf[j++] = 'f';  //\7f
			}else
            if(((unsigned char)str[i]) == 0xC0 && (i < len -1 && ((unsigned char)str[i+1]) == 0x80))
			{
				buf[j++] ='?';  //0xc080 is null 
			}else
            if(((unsigned char)str[i]) == 0x80 && (i > 0 && ((unsigned char)str[i-1]) == 0xC0))
			{
				buf[j++] ='?';
			}
			else
            {
				buf[j++] = str[i];
			}
		}
		
		i++;
	}

	buf[j] = 0;
	*bufObj = new char[j + 1];

	strcpy(*bufObj,buf);
	delete buf;
}

ObjectData::ObjectData()
{
    attributes = new vector<TypeElementData*>();

	super = NULL;
	id = -1;
	type = NULL;
	value = NULL;
	expanded = false;
	expandedOnce = false;
	layersNumber = -1;
	isArrayObject = false;
	itemNumber = -1;
}

ObjectData::~ObjectData()
{
   if(value != NULL) 
   {
	   delete[] value;
	   value = NULL;
   }

   if(type != NULL) 
   {
	   delete[] type;
	   type = NULL;
   }

   if(attributes != NULL)
   {
		vector<TypeElementData*>::iterator iter;
		for (iter = attributes->begin(); iter != attributes->end(); ++iter) 
		{
			TypeElementData * typeElementData = (TypeElementData *)(*iter);
			delete typeElementData;
		}
		attributes->clear();
	    delete attributes;
		attributes = NULL;
   }

   if(super != NULL)
   {
	   delete super;
	   super = NULL;
   }
}

long ObjectData::getDumpSize()
{
	long dumpSize  = 0;
    
	dumpSize += sizeof(id);
	if(type != NULL)
	{
		dumpSize += strlen(type);
	}
	if(value != NULL)
	{
		dumpSize += strlen(value);
	}

	if(itemNumber >=0)
	{
        dumpSize += sizeof(itemNumber);
	}
   	
	return dumpSize;
}


void ObjectData::setId(long id)
{
	this->id = id;
}

void ObjectData::setType(const char* typeParam)
{
	if(typeParam != NULL)
	{
		int charNumber = calEscapeXMLChar(typeParam);
		escapeXMLChar(typeParam,&type,charNumber);
	}
}

void ObjectData::setValue(const char* valueParam)
{
	if(valueParam != NULL)
	{
		int charNumber = calEscapeXMLChar(valueParam);
		escapeXMLChar(valueParam,&value,charNumber);
	}
}


void ObjectData::writeToFile(FILE *fp,OutputToFile* opf)
{
	 int bufsize = MAX_BUF_LEN;
	 if(type != NULL)
		 bufsize += strlen(type);
	 opf->setBufSize(bufsize);
	 //modified by Qiu Song on 20091006 for HashCodeの追加
#ifdef _LINUX
	 //sprintf(opf->getInBuffer(),"<Object Id=\"%ld\" Hash=\"%lld\" Type=\"%s\" ",id, hashCode, type);
	 sprintf(opf->getInBuffer(),"<Object Id=\"%lld\" Type=\"%s\" ", hashCode, type);
#else
	 //sprintf(opf->getInBuffer(),"<Object Id=\"%ld\" Hash=\"%I64d\" Type=\"%s\" ",id, hashCode, type);
	 sprintf(opf->getInBuffer(),"<Object Id=\"%I64d\" Type=\"%s\" ", hashCode, type); 
#endif
	 //sprintf(opf->getInBuffer(),"<Object Id=\"%ld\" Type=\"%s\" ",id,type); 
     //end of modified by Qiu Song on 20091006 for HashCodeの追加
	 opf->mt2u(fp);
   
	 if(value != NULL)
	 {
        opf->setBufSize(MAX_BUF_LEN + strlen(value));
        sprintf(opf->getInBuffer()," Value=\"%s\" ",value);
	    opf->mt2u(fp);    
	 }
     else
     if(strcmp(type,"java.lang.String") == 0)
	 {
		 opf->setBufSize(MAX_BUF_LEN + strlen(value));
		 sprintf(opf->getInBuffer()," Value=\"%s\" ",value);
		 opf->mt2u(fp);
	 }

	if(itemNumber >=0)
	{
		opf->setBufSize(MAX_BUF_LEN);
        sprintf(opf->getInBuffer()," ItemNumber=\"%d\"",getItemNumber());
		opf->mt2u(fp);
	}

	 bool hasSon = false;
	 if((attributes != NULL && attributes->size() > 0) || super != NULL)
	 {
		 hasSon = true;
	 }

	 if(!hasSon)
	 {
		 opf->setBufSize(MAX_BUF_LEN);
		 sprintf(opf->getInBuffer(),"/>\n");
		 opf->mt2u(fp);
		 return;

	 }

	 opf->setBufSize(MAX_BUF_LEN);
     sprintf(opf->getInBuffer()," >\n");
	 opf->mt2u(fp);

     //write attributes
	 vector<TypeElementData*>::iterator iter;
	 for (iter = attributes->begin(); iter != attributes->end(); ++iter) 
	 {
	  	TypeElementData * typeElementData = (TypeElementData *)(*iter);
		if(typeElementData != NULL)
		{
			typeElementData->writeToFile(fp, opf);
		}
	 }

	 //write super class
	 if(super != NULL)
	 {
		 super->writeToFile(fp,opf);
	 }

	 opf->setBufSize(MAX_BUF_LEN);
     sprintf(opf->getInBuffer(),"</Object>\n");
	 opf->mt2u(fp);
}

void ObjectData::addTypeElementData(TypeElementData *typeElementData)
{
	//add by Qiu Song on 20090914 for バグ：属性がnullの場合、例外が発生
	if(attributes == NULL)
	{
		attributes = new vector<TypeElementData*>();
	}
	//end of add by Qiu Song on 20090914 for バグ：属性がnullの場合、例外が発生
    attributes->push_back(typeElementData); 
}

void ObjectData::setSuperClass(SuperClassData *superParam)
{
	super = superParam;
}

//add by Qiu Song on 20090825 for メソッド監視
//指定位置の属性情報を取得する
TypeElementData* ObjectData::getAttribute(int nIndex)
{
	if(attributes == NULL || attributes->size() == 0 ||
	   attributes->size() < nIndex)
	{
		return NULL;
	}
	vector<TypeElementData*>::iterator iter = attributes->begin() + nIndex;
 	TypeElementData * typeElementData = (TypeElementData *)(*iter);
	return typeElementData;
}

TypeElementData* ObjectData::getAttributeByName(char* attrName)
{
	if(attributes == NULL || attributes->size() == 0)
	{
		return NULL;
	}
	vector<TypeElementData*>::iterator iter = attributes->begin();
	for(iter = attributes->begin(); iter != attributes->end(); iter++)
	{
		TypeElementData * typeElementData = (TypeElementData *)(*iter);
		if(strcmp(typeElementData->getObjName(), attrName) == 0)
		{
			return typeElementData;
		}
	}
	return NULL;
}

TypeElementData* ObjectData::getAttributeByIndex(int attrIndex)
{
	if(attributes == NULL || attributes->size() == 0)
	{
		return NULL;
	}
	vector<TypeElementData*>::iterator iter = attributes->begin();
	for(iter = attributes->begin(); iter != attributes->end(); iter++)
	{
		TypeElementData * typeElementData = (TypeElementData *)(*iter);
		if(typeElementData != NULL && typeElementData->getIndex() == attrIndex)
		{
			return typeElementData;
		}
	}
	return NULL;
}
//すべてのattributeを削除する
void ObjectData::removeAllAttribute()
{
	if(attributes != NULL)
	{
		vector<TypeElementData*>::iterator iter;
		for (iter = attributes->begin(); iter != attributes->end(); ++iter) 
		{
			TypeElementData * typeElementData = (TypeElementData *)(*iter);
			if(typeElementData != NULL)
			{
				delete typeElementData;
				typeElementData = NULL;
			}
			delete typeElementData;
		}
		attributes->clear();
		delete attributes;
		attributes = NULL;
	}
}
//end of add by Qiu Song on 20090825 for メソッド監視


TypeElementData::TypeElementData()
{
	objectRef = -1;
	index = -1;
	objDefinedType = NULL;
	objName = NULL;
	tagName = NULL;
	valid = false;
	value = NULL;
	modifiers = 0;

}

TypeElementData::~TypeElementData()
{
	if(objDefinedType != NULL)
	{
		delete[] objDefinedType;
		objDefinedType = NULL;
	}
	if(objName != NULL)
	{
		delete[] objName;
		objName = NULL;
	}
	if(tagName != NULL)
	{
		delete[] tagName;
		tagName = NULL;
	}
	if(value != NULL)
	{
		delete[] value;
		value = NULL;
	}
}


//override
long TypeElementData::getDumpSize()
{
	long dumpSize  = 0;
    
	if(tagName != NULL)
	{
		dumpSize += strlen(tagName);
	}
    if(objName != NULL)
	{
		dumpSize += strlen(objName);
	}
    if(objDefinedType != NULL)
	{
		dumpSize += strlen(objDefinedType);
	}
    if(value != NULL)
	{
		dumpSize += strlen(value);
	}

    dumpSize += sizeof(objectRef);
    dumpSize += sizeof(index);
	dumpSize += sizeof(valid);
	dumpSize += sizeof(unsure);
	dumpSize += sizeof(itemNumber);
    dumpSize += sizeof(modifiers);
	return dumpSize;
}

void TypeElementData::setTagName(const char* tagNameParam)
{
	if(tagNameParam != NULL)
	{ 
		int charNumber = calEscapeXMLChar(tagNameParam);
		escapeXMLChar(tagNameParam,&tagName,charNumber);
	}
} 

void TypeElementData::setObjName(const char* objNameParam)
{
	if(objNameParam != NULL)
	{
		int charNumber = calEscapeXMLChar(objNameParam);
        escapeXMLChar(objNameParam,&objName,charNumber);
	}
	
}

void TypeElementData::setObjDefinedType(const char* typeParam)
{
	if(typeParam != NULL)
	{
		int charNumber = calEscapeXMLChar(typeParam);
        escapeXMLChar(typeParam,&objDefinedType,charNumber);
	}
}

void TypeElementData::setValue(const char* valueParam)
{
	if(valueParam != NULL)
	{
		int charNumber = calEscapeXMLChar(valueParam);
		escapeXMLChar(valueParam,&value,charNumber);
	}
}

void TypeElementData::setObjectRef(long refid)
{
     objectRef = refid;
}

void TypeElementData::setIndex(int id)
{
	index = id;
}

//メソッド監視用の引数など情報をダンプする実装メソッド
void TypeElementData::writeToFile(FILE *fp,OutputToFile* opf)
{
	int bufsize = MAX_BUF_LEN;
	if(tagName != NULL)
		bufsize += strlen(tagName);
	if(objDefinedType != NULL)
		bufsize += strlen(objDefinedType);
	if(objName != NULL)
	{
		if(strcmp(objName, METHOD_RETURN_VALUE) == 0)
		{
			return;
		}
		bufsize += strlen(objName);
	}
	opf->setBufSize(bufsize);
	sprintf(opf->getInBuffer(),"<%s DefinedType=\"%s\" Name=\"%s\" ",
		tagName,objDefinedType,objName);
	opf->mt2u(fp);

	if(value != NULL)
	{
		opf->setBufSize(MAX_BUF_LEN + strlen(value));
		sprintf(opf->getInBuffer()," Value=\"%s\" ",value);
		opf->mt2u(fp);
	}
	if(strcmp(tagName,TypedElement::TAG_VARIABLE) == 0)
	{
         if(valid)
         {
			 opf->setBufSize(MAX_BUF_LEN );
			 sprintf(opf->getInBuffer()," Valid=\"true\"");
			 opf->mt2u(fp);
		 }
		 else
		 {
			 opf->setBufSize(MAX_BUF_LEN);
			 sprintf(opf->getInBuffer()," Valid=\"false\""); 
			 opf->mt2u(fp);
		 }
		 if(unsure)
		 {
			 opf->setBufSize(MAX_BUF_LEN);
             sprintf(opf->getInBuffer()," Unsure=\"true\"");
			 opf->mt2u(fp);
		 }
	}

	if(index >= 0)
	{
		opf->setBufSize(MAX_BUF_LEN);
		sprintf(opf->getInBuffer()," Index=\"%d\"",index);
		opf->mt2u(fp);

	}

	if(objectRef >= 0)
	{
		opf->setBufSize(MAX_BUF_LEN);
		//modified by Qiu Song on 20091016 for RefIDはHashに変更する
        //sprintf(opf->getInBuffer()," ObjectRef=\"%ld\"",objectRef);
		jlong objectHashCode = opf->getObjectHashCode(objectRef);
		if(objectHashCode != -1)
		{
#ifdef _LINUX
			sprintf(opf->getInBuffer()," ObjectRef=\"%lld\"",objectHashCode);
#else
			sprintf(opf->getInBuffer()," ObjectRef=\"%I64d\"",objectHashCode);
#endif
		}
		//end of modified by Qiu Song on 20091016 for RefIDはHashに変更する
		opf->mt2u(fp);
	}

	if(itemNumber >=0)
	{
		opf->setBufSize(MAX_BUF_LEN);
        sprintf(opf->getInBuffer()," ItemNumber=\"%d\"",itemNumber);
		opf->mt2u(fp);
	}

	if(modifiers >0)
	{
		opf->setBufSize(MAX_BUF_LEN);
        sprintf(opf->getInBuffer()," Modifiers=\"%d\"",modifiers);
		opf->mt2u(fp);
	}

	bool hasSon = false;
	if(attributes->size() > 0)
	{
		hasSon = true;
	}

	if(!hasSon)
	{
		opf->setBufSize(MAX_BUF_LEN);
		sprintf(opf->getInBuffer()," />\n"); 
		opf->mt2u(fp);
		return;
	}
	opf->setBufSize(MAX_BUF_LEN);
    sprintf(opf->getInBuffer()," >\n");
	opf->mt2u(fp);
    //write attributes
	vector<TypeElementData*>::iterator iter;
	for (iter = attributes->begin(); iter != attributes->end(); ++iter) 
	{
	 	TypeElementData * typeElementData = (TypeElementData *)(*iter);
		typeElementData->writeToFile(fp,opf);
	}

	 //end tag
	opf->setBufSize(MAX_BUF_LEN);
	sprintf(opf->getInBuffer(),"</%s>\n",tagName); 
	opf->mt2u(fp);
}

void TypeElementData::setValid(bool validParam)
{
	valid = validParam;
}

void TypeElementData::setModifiers(jint param)
{
	modifiers = param;
}

SuperClassData::SuperClassData()
{
	signature = NULL;
}

SuperClassData::~SuperClassData()
{
	if(signature != NULL)
	{
		delete[] signature;
		signature = NULL;
	}
}

//override
long SuperClassData::getDumpSize()
{
	long dumpSize  = 0;
    
	if(signature != NULL)
	{
		dumpSize += strlen(signature);
	}
	return dumpSize;
}

void SuperClassData::setSignature(const char* param)
{
	if(param != NULL)
	{
		int charNumber = calEscapeXMLChar(param);
		escapeXMLChar(param,&signature,charNumber);
	}
}

void SuperClassData::writeToFile(FILE *fp,OutputToFile* opf)
{
	int bufsize = MAX_BUF_LEN;
	if(signature != NULL)
       bufsize += strlen(signature);
	opf->setBufSize(bufsize);
    sprintf(opf->getInBuffer(),"<SuperClass Signature=\"%s\"",signature);
	opf->mt2u(fp);

	bool hasSon = false;

	if(attributes->size() > 0 || getSuperClass()!= NULL)
	{
		hasSon = true;
	}

	if(!hasSon)
	{
		opf->setBufSize(MAX_BUF_LEN);
		sprintf(opf->getInBuffer(),"/>");
		opf->mt2u(fp);
		return;
	}
	opf->setBufSize(MAX_BUF_LEN);
    sprintf(opf->getInBuffer()," >\n");
	opf->mt2u(fp);
    //write attributes
	vector<TypeElementData*>::iterator iter;
	for (iter = attributes->begin(); iter != attributes->end(); ++iter) 
	{
	  	TypeElementData * typeElementData = (TypeElementData *)(*iter);
		typeElementData->writeToFile(fp,opf);
	}

	if(getSuperClass() != NULL)
	{
		getSuperClass()->writeToFile(fp,opf);
	}

	//end the tag
	opf->setBufSize(MAX_BUF_LEN);
	sprintf(opf->getInBuffer(),"</SuperClass>");
	opf->mt2u(fp);
}

MethodData::MethodData()
{
	location = -1;
	lineNumber = -1;
	declaringClass = NULL;
    name = NULL;
    signature = NULL;
	sourceFile = NULL;
	modifiers = 0;
	//add by Qiu Song on 20090818 for メソッド監視
	objectRef = -1;
	objReturnName = NULL;
	objReturnType = NULL;
	objReturnValue = NULL;
	exceptionObjName = NULL;
	exceptionObjRef = -1;
	monitorObjName = NULL;
	monitorObjRef = -1;
	useMonitorThreadName = NULL;
	//end of add by Qiu Song on 20090818 for メソッド監視
}

MethodData::~MethodData()
{
	if(name != NULL)
	{
		delete[] name;
		name = NULL;
	}
	if(declaringClass != NULL)
	{
		delete[] declaringClass;
		declaringClass = NULL;
	}
	if(signature != NULL)
	{
		delete[] signature;
		signature = NULL;
	}

	if(sourceFile != NULL)
	{
		delete[] sourceFile;
		sourceFile = NULL;
	}
	//add by Qiu Song on 20090921 for メソッド監視
	if(objReturnName != NULL)
	{
		delete[] objReturnName;
		objReturnName = NULL;
	}
	if(objReturnType != NULL)
	{
		delete[] objReturnType;
		objReturnType = NULL;
	}
	if(objReturnValue != NULL)
	{
		delete[] objReturnValue;
		objReturnValue = NULL;
	}
	if(exceptionObjName == NULL)
	{
		delete[] exceptionObjName;
		exceptionObjName = NULL;
	}
	if(monitorObjName == NULL)
	{
		delete[] monitorObjName;
		monitorObjName = NULL;
	}
	if(useMonitorThreadName == NULL)
	{
		delete[] useMonitorThreadName;
		useMonitorThreadName = NULL;
	}
	//end of add by Qiu Song on 20090921 for メソッド監視
}

//override
long MethodData::getDumpSize()
{
	long dumpSize  = 0;
    
	if(name != NULL)
	{
		dumpSize += strlen(name);
	}
    if(signature != NULL)
	{
		dumpSize += strlen(signature);
	}
    if(declaringClass != NULL)
	{
		dumpSize += strlen(declaringClass);
	}
   
    if(sourceFile != NULL)
	{
		dumpSize += strlen(sourceFile);
	}	

    dumpSize += sizeof(location);
    dumpSize += sizeof(lineNumber);
    dumpSize += sizeof(modifiers);
	return dumpSize;
}

void MethodData::setName(const char* nameParam)
{
	if(nameParam != NULL)
	{
		int charNumber = calEscapeXMLChar(nameParam);
		escapeXMLChar(nameParam,&name,charNumber);
	}
     
}	

void MethodData::setSignature(const char* param)
{
	if(param != NULL)
	{
        int charNumber = calEscapeXMLChar(param);
		escapeXMLChar(param,&signature,charNumber);
	}
 }

void MethodData::setDeclaringClass(const char* param)
{
	if(param != NULL)
	{
		int charNumber = calEscapeXMLChar(param);
		escapeXMLChar(param,&declaringClass,charNumber);
	}
    
}

void MethodData::setSourceFile(const char* param)
{
	if(param != NULL)
	{
		int charNumber = calEscapeXMLChar(param);
		escapeXMLChar(param,&sourceFile,charNumber);
	}
}

void MethodData::setLocation(int param)
{
	location = param;
}

void MethodData::setLineNumber(int param)
{
    lineNumber = param;
}

void MethodData::setModifiers(jint param){
	
	modifiers = param;
}

void MethodData::writeToFile(FILE *fp,OutputToFile* opf)
{
	int bufsize = MAX_BUF_LEN;
	if(declaringClass != NULL)
        bufsize += strlen(declaringClass);
	if(name != NULL)
        bufsize += strlen(name);
	if(signature != NULL)
        bufsize += strlen(signature);

	opf->setBufSize(bufsize);
	sprintf(opf->getInBuffer(),"<Method DeclaringClass=\"%s\" Name=\"%s\" Signature=\"%s\"",
		declaringClass,name,signature);
	opf->mt2u(fp);

	opf->setBufSize(MAX_BUF_LEN);
	sprintf(opf->getInBuffer()," LineNumber=\"%d\" Location=\"%d\" Modifiers=\"%d\"",
		lineNumber,location,modifiers);
    opf->mt2u(fp);

	if(sourceFile != NULL)
	{
		opf->setBufSize(MAX_BUF_LEN + strlen(sourceFile));
		sprintf(opf->getInBuffer()," SourceFile=\"%s\"",sourceFile);
		opf->mt2u(fp);
	}

	bool hasSon = false;
	if(attributes->size() > 0)
	{
		hasSon = true;
	}

	if(!hasSon && exceptionObjName == NULL && monitorObjName == NULL && getReturnType() == NULL)
	{
		opf->setBufSize(MAX_BUF_LEN);
		sprintf(opf->getInBuffer()," />\n");
		opf->mt2u(fp);
		return;
	}
	opf->setBufSize(MAX_BUF_LEN);
    sprintf(opf->getInBuffer()," >\n");
	opf->mt2u(fp);
    
	//write attributes
	vector<TypeElementData*>::iterator iter;
	for (iter = attributes->begin(); iter != attributes->end(); ++iter) 
	{
	 	TypeElementData * typeElementData = (TypeElementData *)(*iter);
	    typeElementData->writeToFile(fp,opf);
	}

	//add by Qiu Song on 20090818 for メソッド監視
	//write return value
	//例：<Return DefinedType="abc.ReturnClass" Name="returnClass"/>
	if(getReturnType() != NULL)
	{
		opf->setBufSize(MAX_BUF_LEN + strlen(getReturnType()));
		char* definedType = HelperFunc::convertClassSig(getReturnType());
		if(definedType != NULL)
		{
			sprintf(opf->getInBuffer(),"<Return DefinedType=\"%s\"",definedType);
			opf->mt2u(fp);
			delete definedType;
		}

		//Nameがあるかどうか判断する
		if(getReturnName() != NULL && strcmp(getReturnName(), METHOD_RETURN_VALUE) != 0)
		{
			opf->setBufSize(MAX_BUF_LEN + strlen(getReturnName()));
			sprintf(opf->getInBuffer()," Name=\"%s\"",getReturnName());
			opf->mt2u(fp);
		}

		//Valueがあるかどうか
		if(getReturnValue() != NULL)
		{
			opf->setBufSize(MAX_BUF_LEN + strlen(getReturnValue()));
			sprintf(opf->getInBuffer()," Value=\"%s\"",getReturnValue());
			opf->mt2u(fp);
		}

		//objRefがあるかどうか
		if(getObjectRef() >= 0)
		{
			opf->setBufSize(MAX_BUF_LEN);
			jlong objectHashCode = opf->getObjectHashCode(getObjectRef());
			if(objectHashCode != -1)
			{
#ifdef _LINUX
				sprintf(opf->getInBuffer()," ObjectRef=\"%lld\"",objectHashCode);
#else
				sprintf(opf->getInBuffer()," ObjectRef=\"%I64d\"",objectHashCode);
#endif
				opf->mt2u(fp);
			}
		}
		opf->setBufSize(MAX_BUF_LEN);
		sprintf(opf->getInBuffer()," />\n");
		opf->mt2u(fp);
	}
	//end of add by Qiu Song on 20090818 for メソッド監視

	//add by Qiu Song on 20091006 for 出力機能の修正(10/5の仕様変更)
    if(exceptionObjName != NULL && exceptionObjRef >= 0)
	{
		opf->setBufSize(MAX_BUF_LEN);
		//sprintf(opf->getInBuffer(), "<ExceptionObject ClassName=\"%s\" ObjectRef=\"%d\" />\n",exceptionObjName, exceptionObjRef);
#ifdef _LINUX
		sprintf(opf->getInBuffer(), "<ExceptionObject DefinedType=\"%s\" ObjectRef=\"%lld\" />\n",exceptionObjName, opf->getObjectHashCode(exceptionObjRef));
#else
		sprintf(opf->getInBuffer(), "<ExceptionObject DefinedType=\"%s\" ObjectRef=\"%I64d\" />\n",exceptionObjName, opf->getObjectHashCode(exceptionObjRef));
#endif	
		opf->mt2u(fp);
	}

	if(monitorObjName != NULL && monitorObjRef >= 0)
	{
		opf->setBufSize(MAX_BUF_LEN);
		if(useMonitorThreadName != NULL)
		{
//			sprintf(opf->getInBuffer(),"<ContendMonitorObject ClassName=\"%s\" ObjectRef=\"%d\" UseThreadName =\"%s\" />\n", monitorObjName, monitorObjRef, useMonitorThreadName);
#ifdef _LINUX
			sprintf(opf->getInBuffer(),"<ContendMonitorObject DefinedType=\"%s\" ObjectRef=\"%lld\" UseThreadName =\"%s\" />\n", monitorObjName, opf->getObjectHashCode(monitorObjRef), useMonitorThreadName);
#else
			sprintf(opf->getInBuffer(),"<ContendMonitorObject DefinedType=\"%s\" ObjectRef=\"%I64d\" UseThreadName =\"%s\" />\n", monitorObjName, opf->getObjectHashCode(monitorObjRef), useMonitorThreadName);
#endif	
		}
		else
		{
//			sprintf(opf->getInBuffer(),"<ContendMonitorObject ClassName=\"%s\" ObjectRef=\"%d\" />\n", monitorObjName, monitorObjRef);
#ifdef _LINUX
			sprintf(opf->getInBuffer(),"<ContendMonitorObject DefinedType=\"%s\" ObjectRef=\"%lld\" />\n", monitorObjName, opf->getObjectHashCode(monitorObjRef));
#else
			sprintf(opf->getInBuffer(),"<ContendMonitorObject DefinedType=\"%s\" ObjectRef=\"%I64d\" />\n", monitorObjName, opf->getObjectHashCode(monitorObjRef));
#endif
		}
		opf->mt2u(fp);
	}
	//end of add by Qiu Song on 20091006 for 出力機能の修正(10/5の仕様変更)

	//end the tag
	opf->setBufSize(MAX_BUF_LEN);
	sprintf(opf->getInBuffer(),"</Method>\n");
	opf->mt2u(fp);
}

//add by Qiu Song on 20091127 for チケット:536 
void MethodData::setReturnType(char * returnType)
{ 
	if(returnType != NULL)
	{
		if(objReturnType != NULL)
		{
			delete objReturnType;
		}
		int charNumber = calEscapeXMLChar(returnType);
		escapeXMLChar(returnType,&objReturnType,charNumber);
	}
}

void MethodData::setReturnValue(char * returnValue)
{ 
	if(returnValue != NULL)
	{
		if(objReturnValue != NULL)
		{
			delete objReturnValue;
		}
		int charNumber = calEscapeXMLChar(returnValue);
		escapeXMLChar(returnValue,&objReturnValue,charNumber);
	}
}

void MethodData::setReturnName(char * returnName)
{ 
    if(returnName != NULL)
	{
		if(objReturnName != NULL)
		{
			delete objReturnName;
		}
		int charNumber = calEscapeXMLChar(returnName);
		escapeXMLChar(returnName,&objReturnName,charNumber);
	}
}

void MethodData::setExceptionObjName(char* objName)
{
	if(objName != NULL)
	{
		if(exceptionObjName != NULL)
		{
			delete exceptionObjName;
		}
		int charNumber = calEscapeXMLChar(objName);
		escapeXMLChar(objName,&exceptionObjName,charNumber);
	}
}

void MethodData::setMonitorObjName(char* objName)
{
	if(objName != NULL)
	{
		if(monitorObjName != NULL)
		{
			delete monitorObjName;
		}
		int charNumber = calEscapeXMLChar(objName);
		escapeXMLChar(objName,&monitorObjName,charNumber);
	}
}

void MethodData::setUseMonitorThreadName(char* objName)
{
	if(objName != NULL)
	{
		if(useMonitorThreadName != NULL)
		{
			delete useMonitorThreadName;
		}
		int charNumber = calEscapeXMLChar(objName);
		escapeXMLChar(objName,&useMonitorThreadName,charNumber);
	}
}
//end of add by Qiu Song on 20091127 for チケット:536

InstanceData::InstanceData(string& name, string& url)
: objectRef(NULL)
, objectSize(NULL)
, className(name)
, classUrl(url)
{
}

InstanceData::~InstanceData()
{
	if (objectRef != NULL)
	{
		objectRef->clear();
		delete objectRef;
		objectRef = NULL;
	}
		
	if (objectSize != NULL)
	{
		objectSize->clear();
		delete objectSize;
		objectSize = NULL;
	}
}

long InstanceData::getDumpSize()
{
	long dumpSize  = 0;
    
	dumpSize += className.length();
	dumpSize += classUrl.length();
	
	if (objectRef != NULL)
	{
		dumpSize += objectRef->size() * (sizeof(long) * 2);
	}
	
	return dumpSize;
}

void InstanceData::writeToFile(FILE *fp,OutputToFile* opf)
{
	if ((objectRef == NULL)	|| (objectRef->size() == 0))
		return;

	opf->setBufSize(MAX_BUF_LEN + className.length() + classUrl.length());
	sprintf(opf->getInBuffer(),"<MonitorObject ClassName=\"%s\" ClassUrl=\"%s\" >\n",
			className.c_str(), classUrl.c_str());
	opf->mt2u(fp);
	
	int seqId = 1;
	vector<long>::iterator itRef = objectRef->begin();
	vector<long>::iterator itSize = objectSize->begin();
	for (;itRef != objectRef->end();itRef++,itSize++)
	{
		opf->setBufSize(MAX_BUF_LEN);
		//modified by Qiu Song on 20091016 for RefIDはHashに変更する
		jlong objectHashCode = opf->getObjectHashCode(*itRef);
		if(objectHashCode == -1)
		{
			sprintf(opf->getInBuffer(),"<Instance SeqId=\"%d\" ObjectRef=\"%ld\" ObjectSize=\"%ld\" />\n",
				seqId++, *itRef, *itSize);
		}
		else
		{
#ifdef _LINUX
			sprintf(opf->getInBuffer(),"<Instance SeqId=\"%d\" ObjectRef=\"%lld\" ObjectSize=\"%ld\" />\n",
				seqId++, objectHashCode, *itSize);
#else
			sprintf(opf->getInBuffer(),"<Instance SeqId=\"%d\" ObjectRef=\"%I64d\" ObjectSize=\"%ld\" />\n",
				seqId++, objectHashCode, *itSize);
#endif
		}
		//end of modified by Qiu Song on 20091016 for RefはHashに変更する
		opf->mt2u(fp);
	}

	 opf->setBufSize(MAX_BUF_LEN);
	 strcpy(opf->getInBuffer(),"</MonitorObject>\n");
	 opf->mt2u(fp);
}

OutputToFile::OutputToFile()
{
	encodePos = 0;
	inBuf = NULL;
	outBuf = NULL;
	oldBufSize = 0;
	//add by Qiu Song on 20091016 for RefIdはHashに変更する
    m_objectDatas = NULL;
	//end of add by Qiu Song on 20091016 for RefIdはHashに変更する
}

OutputToFile::~OutputToFile()
{
	freeBuf();
}

void OutputToFile::setBufSize(long size)
{
	if(oldBufSize >= size )
	{
		return;
	}

	oldBufSize = size;
	freeBuf();
    inBuf = new char[size];
	outBuf = new unsigned char[size];
}

void OutputToFile::setDumpType(int paramType)
{
    dumpType = paramType;
}

void OutputToFile::freeBuf()
{
    if(inBuf != NULL)
	{
		delete[] inBuf;
		inBuf = NULL;
	}

    if(outBuf != NULL)
	{
		delete[] outBuf;
		outBuf = NULL;
	}

}

void OutputToFile::encode(char* from,int bytes)
{
    unsigned char* key = Global::getEncodeKey();

	int i = 0;
	while(i < bytes)
	{
		char oldChar = from[i];
		int keyPos = encodePos % ENCODE_KEY_NUMBER;
		unsigned char keyChar = key[keyPos];
		unsigned char encodedChar = oldChar ^ keyChar;
		outBuf[i] = encodedChar;
		i++;
		encodePos++;
	}
}

void OutputToFile::mt2u(FILE* fp)
{
	//test by Qiu Song on 20090811(TODO:削除べきです)
	//fprintf(fp,"%s",inBuf);
	//return;
	//end of test by Qiu Song on 20090811(TODO:削除べきです)
	if(Global::isNeedEncode())
	{
		encode(inBuf,strlen(inBuf));
		fwrite(outBuf,1,strlen(inBuf),fp);
		
	}else
	{
		fprintf(fp,"%s",inBuf);
	}
	
	return;
}

//ダンプファイル出力の実装メソッド
bool OutputToFile::writeToFile(const char* fileName,JNIEnv *jni,
							   char* exceptionName,
							   char* threadName,
							   char* strDumptime,
							   vector<ObjectData*> *objectDataes,
							   vector<MethodData*> *methodDataes,
							   vector<InstanceData*> *instanceDataes,
							   int exceptionObjRefId,//add by Qiu Song on 20090811 for 例外オブジェクトダンプ
							   jint threadStutas,    //add by Qiu Song on 20090818 for スレッド状態/待機理由
					           jlong cpuTime,        //add by Qiu Song on 20090818 for CPU時間
							   jint  threadPriority, //add by Qiu Song on 20091021 for スレッド優先度の指定
							   int monitorObjRefId,  //add by Qiu Song on 20090819 for モニターオブジェクトダンプ
							   char* monitorClassName,//add by Qiu Song on 20090819 for モニターオブジェクトダンプ
							   string className,//add by Qiu Song on 20090828 for メソッド監視
							   string methodName,//add by Qiu Song on 20090828 for メソッド監視
							   string methodSig,//add by Qiu Song on 20090828 for メソッド監視
							   string dumpPosition,//add by Qiu Song on 20090828 for メソッド監視
							   char* useMonitorThreadName, //add by Qiu Song on 20091006 for モニターオブジェクトダンプ
							   bool dumpCompleted)
{
    FILE* fp;
	if(Global::isNeedEncode())
	{
		fp = fopen(fileName, "wb");
	}else
	{
        fp = fopen(fileName, "w");
	}
	
	if(fp == NULL)
	{
    	string logBuf;
	    logBuf = "can't open file for writing: ";
		logBuf += fileName;
        LOG4CXX_ERROR(Global::logger, logBuf.c_str());
		return false;
	}

    char* encoding = "UTF-8";

	//get jvm information  
	SystemInfo* systemInfo = SystemInfo::getInstance();
    systemInfo->getJVMInfo(jni);

	setBufSize(MAX_BUF_LEN);
	sprintf(inBuf,"<?xml version=\"1.0\" encoding=\"%s\"?>\n",encoding);
	mt2u(fp);

	setBufSize(MAX_BUF_LEN);
	sprintf(inBuf,"<Dump>\n");
	mt2u(fp);

    setBufSize(MAX_BUF_LEN);
	sprintf(inBuf,"<StackTrace DumpTime=\"%s\"\n",strDumptime);
	mt2u(fp);

	setBufSize(MAX_BUF_LEN + strlen(systemInfo->getSystemID()) + 
		strlen(systemInfo->getJvmID()));
    sprintf(inBuf," SystemID =\"%s\" JVMID =\"%s\" \n",
		  systemInfo->getSystemID(),systemInfo->getJvmIDEscaped());
	mt2u(fp);

	setBufSize(MAX_BUF_LEN + strlen(systemInfo->getJvmVersion()) + 
		strlen(systemInfo->getJvmName()));
    sprintf(inBuf," JVMVersion =\"%s\" JVMName =\"%s\" \n",
		  systemInfo->getJvmVersion(),systemInfo->getJvmName());
	mt2u(fp);

	if(dumpCompleted)
	{
		setBufSize(MAX_BUF_LEN);
        sprintf(inBuf," JVMVender =\"%s\" DumpCompleted=\"true\" \n",
		  systemInfo->getJvmVendor());
	}else
	{
		setBufSize(MAX_BUF_LEN);
        sprintf(inBuf," JVMVender =\"%s\" DumpCompleted=\"false\" \n",
		  systemInfo->getJvmVendor());
	}
	mt2u(fp);
 
	//add by Qiu Song on 20090818 for スレッド状態/CPU時間/待機理由の追加
	setBufSize(MAX_BUF_LEN);
#ifdef _LINUX
	sprintf(inBuf," Status =\"%s\"  CPUTime=\"%lld\" WaitReason=\"%s\" \n",
			HelperFunc::getThreadStatusString(threadStutas), cpuTime, 
			HelperFunc::getThreadWaitingReason(threadStutas));
#else
	//windows
	sprintf(inBuf," Status =\"%s\"  CPUTime=\"%I64d\" WaitReason=\"%s\" \n",
			HelperFunc::getThreadStatusString(threadStutas), cpuTime, 
			HelperFunc::getThreadWaitingReason(threadStutas));
#endif
    mt2u(fp);
	//end of add by Qiu Song on 20090818 for スレッド状態/CPU時間/待機理由の追加
    //add by Qiu Song on 20091022 for スレッド優先度の指定
    sprintf(inBuf," ThreadPriority =\"%d\" \n", threadPriority);
	mt2u(fp);
	//end of add by Qiu Song on 20091022 for スレッド優先度の指定

	if ((exceptionName != NULL) && strlen(exceptionName) > 0)
	{
		setBufSize(MAX_BUF_LEN + strlen(exceptionName)) ;
        if(dumpType == DUMP_KIND_THREAD || dumpType == DUMP_KIND_EXCEPTION_ALL ||
			dumpType == DUMP_KIND_METHOD_ALL )
		{
			sprintf(inBuf," ThreadId = \"\" ThreadName =\"%s\">\n", exceptionName);
		}else
		{
			//get current thread id
			unsigned long threadId = HelperFunc::getCurrentThreadId();
			if(dumpType == DUMP_KIND_EXCEPTION || dumpType == DUMP_KIND_THROWABLE)
			{
                sprintf(inBuf," ThreadId =\"0x%x\" ExceptionName =\"%s\" ThreadName =\"%s\">\n",
					threadId,exceptionName,threadName);
			}
			else
			{
				sprintf(inBuf," ThreadId =\"0x%x\" ThreadName =\"%s\">\n" ,threadId ,threadName);
			}
		}

		mt2u(fp);
	}
	else
	{
		setBufSize(MAX_BUF_LEN) ;
		
		//add by Qiu Song on 20100222 for 構築関数監視：「<」と「>」の対応
        string methodNameOutput = methodName;
		if(methodName.at(0) == '<')
		{
			methodNameOutput = className;
		}
		//end of add by Qiu Song on 20100222 for 構築関数監視：「<」と「>」の対応
		//add by Qiu Song on 20090818 for スレッド名の追加ダンプ
		if(dumpType == DUMP_KIND_METHOD)
		{
			sprintf(inBuf," ThreadName =\"%s\" \n ClassName =\"%s\" MethodName =\"%s\" MethodSignature =\"%s\" Position =\"%s\">\n" ,
					threadName,className.c_str(), methodNameOutput.c_str(), methodSig.c_str(), dumpPosition.c_str());
		}
		else
		{
			sprintf(inBuf," ThreadId = \"\" ThreadName =\"%s\">\n" , threadName);
		}
		//end of add by Qiu Song on 20090818 for スレッド名の追加ダンプ
		mt2u(fp);
	}
	

	//write object pool
	setBufSize(MAX_BUF_LEN);
    sprintf(inBuf,"<ObjectPool>\n");
    	vector<ObjectData*>::iterator iter;
	mt2u(fp);

	for (iter = objectDataes->begin(); iter != objectDataes->end(); ++iter) 
	{
		ObjectData * objData = (ObjectData *)(*iter);
		if(objData != NULL)
		{
			objData->writeToFile(fp,this);
		}
	}
	setBufSize(MAX_BUF_LEN);
	sprintf(inBuf,"</ObjectPool>\n");
	mt2u(fp);

	//write methods
	vector<MethodData*>::iterator iterMethod;
	for (iterMethod = methodDataes->begin(); iterMethod != methodDataes->end(); ++iterMethod) 
	{
		MethodData * methodData = (MethodData *)(*iterMethod);
		//add by Qiu Song on 20091006 for 出力機能の改善(10/5の仕様変更)
		if((iterMethod+1) == methodDataes->end())
		{
			methodData->setExceptionObjName(exceptionName);
			methodData->setExceptionRefId(exceptionObjRefId);
			if(methodData->getLocation() != -1)
			{
				methodData->setMonitorObjName(monitorClassName);
				methodData->setMonitorRefId(monitorObjRefId);
				methodData->setUseMonitorThreadName(useMonitorThreadName);
			}
		}
		else if((iterMethod+2) == methodDataes->end())
		{
			MethodData* methodDataLast = (MethodData *)(*(iterMethod+1));
            if(methodDataLast->getLocation() == -1)
			{
				methodData->setMonitorObjName(monitorClassName);
				methodData->setMonitorRefId(monitorObjRefId);
				methodData->setUseMonitorThreadName(useMonitorThreadName);
			}
		}
		//end of add by Qiu Song on 20091006 for 出力機能の改善(10/5の仕様変更)
		methodData->writeToFile(fp,this);
	}

	setBufSize(MAX_BUF_LEN);
    sprintf(inBuf,"</StackTrace>\n");
	mt2u(fp);

	//write instance
	vector<InstanceData*>::iterator iterInstance;
	for (iterInstance = instanceDataes->begin(); iterInstance != instanceDataes->end(); ++iterInstance) 
	{
		InstanceData * instanceData = (InstanceData *)(*iterInstance);
		instanceData->writeToFile(fp,this);
	}

	setBufSize(MAX_BUF_LEN);
    sprintf(inBuf,"</Dump>\n");
	mt2u(fp);
	fclose(fp);

	return true;
}

//add by Qiu Song on 20091016 for RefIdはHashCodeに変更する
jlong OutputToFile::getObjectHashCode(int id)
{
	if(m_objectDatas == NULL)
	{
		return -1;
	}
	else
	{
		ObjectData* objData = getObjectData(id);
		if(objData == NULL)
		{
			return -1;
		}
		else
		{
			return objData->getHashCode();
		}
	}
}
//end of add by Qiu Song on 20091016 for RefIdはHashCodeに変更する



