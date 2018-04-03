#include <jni.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _LINUX
#include <unistd.h>
#include <sys/statvfs.h>
#include <pthread.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h> 
#include <net/if_arp.h> 
#else
#include <windows.h>
#include <direct.h>//add by Qiu Song on 200901013 for 自動監視機能のフォルダ削除
#endif

#include "JvmtiAutoRelease.h"
#include "HelperFunc.h"
#include "Global.h"
#include "Define.h"
#include "JniLocalRefAutoRelease.h"
#if defined(_EXCAT_SOLARIS) || defined(_EXCAT_AIX) ||defined(_EXCAT_HPUX)
#include <iconv.h>
#include <langinfo.h>
#endif

#define MAXINTERFACES   32

USE_NS(NS_COMMON)

unsigned long  HelperFunc::getCurrentThreadId()
{
#ifdef _LINUX
	return (unsigned long)pthread_self();
#else
	return ::GetCurrentThreadId();
#endif
}

char* HelperFunc::strdup(const char* data)
{
	if(data == NULL)
	{
		return NULL;
	}
	char* copy = new char[strlen(data) + 1];
	strcpy(copy,data);
	return copy;
}

char* HelperFunc::replaceChar(const char* data,char oldChar,char newChar)
{
	if(data == NULL){
		return NULL;
	}

	char* after = HelperFunc::strdup(data);
	char* p= after;
	while(*p != 0)
	{
		if(*p == oldChar)
		{
			*p = newChar;
		}
		p++;
	}

	return after;
}

bool HelperFunc::removeFile(const char* fileName)
{
	int ret = remove(fileName);
	return (ret == 0);
}

bool HelperFunc::isValidDir(char* dirName)
{
     if(dirName == NULL)
		 return false;

	 char* name = dirName;
     int len = strlen(name);

#ifndef _LINUX
	 if (len > 3)
	 {
		 if(name[len -1] == FILE_SPERATOR_C)
		 {
			name = HelperFunc::strdup(dirName);
			name[len -1]  = 0;
		 }
	 }
#endif

     struct stat fi;
	 int ret = stat(name,&fi);
	 if(ret >=0)
	 {
		 if(S_IFDIR & fi.st_mode)
		 {
			 if (name != dirName)
				 delete[] name;
			 return true;
		 }
	 }
	 if (name != dirName)
		 delete[] name;
	 return false;
}

//make a directory if dirName doesn't exist
bool HelperFunc::makeDirectory(char *dirName)
{
	bool bRet = false;

	if(dirName == NULL)
	{
		return false;
	}

	//if the directory exists,do nothing and return true
	if(isValidDir(dirName))
	{
		return true;
	}
#ifdef _LINUX
	bRet = (mkdir(dirName,0777) == 0);
#else
    bRet = (CreateDirectory(dirName,NULL) != 0);
#endif

	return bRet;
}

bool HelperFunc::isValidFile(const char* fileName)
{
     if(fileName == NULL)
		 return false;

     struct stat fi;
	 int ret = stat(fileName,&fi);
	 if(ret >=0)
	 {
		 if(S_IFREG & fi.st_mode)
		 {
			 return true;
		 }else
		 {
			 return false;
		 }
	 }else
	 {
          return false;
	 }

}

long  HelperFunc::getFileSize(char* fileFullPath)
{
	if(fileFullPath == NULL)
	{
		return -1;
	}

	struct stat fi;
	int ret = stat(fileFullPath,&fi);
    if(ret >= 0)
	{
		 if(S_IFREG & fi.st_mode)
		 {
			 return fi.st_size;
		 }else
		 {
			 return -1;
		 }
	}else
	{
		return -1;
	}
}

bool  HelperFunc::renameFile(char* oldName,char* newName)
{
    return rename(oldName,newName) == 0;
}

static char invalid_char[]={'\\','/','*',':','?','"','<','>','|'};
bool HelperFunc::containInvalidFileNameChar(char* name)
{
	if(name == NULL)
		return false;
    int len = sizeof(invalid_char) /sizeof(char);
	for(int i = 0; i < len; i++)
	{
		if(strchr(name,invalid_char[i]) != NULL)
			return true;
	}

	return false;
}

void HelperFunc::convertInvalidFileNameChar(char* name)
{
	if(name == NULL)
		return;

    int len = sizeof(invalid_char) /sizeof(char);
	for(int i = 0; i < len; i++)
	{
		char* pStart = name;
		char* pTmp = NULL;
		while (pStart[0] != 0)
		{
			pTmp = strchr(pStart,invalid_char[i]);
			if(pTmp != NULL)
				pTmp[0] = '_';
			else
				break;
			pStart = pTmp++;
		}
	}
	return;
}

void HelperFunc::getLongTypeValue(jlong value,char*buf)
{
#ifdef _LINUX
	sprintf(buf,"%lld",value);
#else
	//windows
	sprintf(buf,"%I64d",value);
#endif
}

#ifdef _LINUX
#include <unistd.h>
void  HelperFunc::mySleep(int seconds)
{
	int sleepTime = seconds;
    while(sleepTime > 0)
	{
		sleepTime = sleep(sleepTime);
	}
	
}
#else
#include <windows.h>
void  HelperFunc::mySleep(int seconds)
{
	DWORD dwMilliseconds = seconds * 1000;
	Sleep(dwMilliseconds);
}
#endif 

// Unicode(UTF-16) -> UTF-8 下請け
int HelperFunc::utf16be_to_utf8_sub( char *pUtf8, const unsigned short *pUcs2, int nUcsNum, bool bCountOnly)
{
    int nUcs2, nUtf8 = 0;

    for ( nUcs2=0; nUcs2 < nUcsNum; nUcs2++) {
        if ( (unsigned short)pUcs2[nUcs2] <= 0x007f) {
            if ( bCountOnly == false) {
                pUtf8[nUtf8] = (pUcs2[nUcs2] & 0x007f);
            }
            nUtf8 += 1;
        } else if ( (unsigned short)pUcs2[nUcs2] <= 0x07ff) {
            if ( bCountOnly == false) {
                pUtf8[nUtf8] = ((pUcs2[nUcs2] & 0x07C0) >> 6 ) | 0xc0; // 2002.08.17 修正
                pUtf8[nUtf8+1] = (pUcs2[nUcs2] & 0x003f) | 0x80;
            }
            nUtf8 += 2;
        } else {
            if ( bCountOnly == false) {
                pUtf8[nUtf8] = ((pUcs2[nUcs2] & 0xf000) >> 12) | 0xe0; // 2002.08.04 修正
                pUtf8[nUtf8+1] = ((pUcs2[nUcs2] & 0x0fc0) >> 6) | 0x80;
                pUtf8[nUtf8+2] = (pUcs2[nUcs2] & 0x003f) | 0x80;
            }
            nUtf8 += 3;
        }
    }
    if ( bCountOnly == false) {
        pUtf8[nUtf8] = '\0';
    }

    return nUtf8;
}
//change a char in utf16be to utf8
char *HelperFunc::utf16be_to_utf8(const jchar utf16char, int *nBytesOut)
{
	//check if the char is surrogate
	if((utf16char <=0xDBFF && utf16char >= 0xD800) ||
       (utf16char <=0xDFFF && utf16char >= 0xDC00) ||
	   (utf16char >=0xFFF0) || utf16char == 0xFEFF || utf16char == 0x0000)    //special char
	{
		*nBytesOut = -1;
		return NULL;
	}
    int nUcsNum;
    char *pUtf8Str;

    nUcsNum = 1;

    unsigned short* wcharbuf = new unsigned short[nUcsNum + 1];
    wcharbuf[0] = utf16char;
    wcharbuf[1] = (wchar_t)0;

    *nBytesOut = utf16be_to_utf8_sub( NULL, wcharbuf, nUcsNum, true);

	pUtf8Str = new char[*nBytesOut + 3];
	memset(pUtf8Str, 0, *nBytesOut + 3);
    utf16be_to_utf8_sub( pUtf8Str, wcharbuf, nUcsNum, false);
    delete wcharbuf;
    return pUtf8Str;
}

// UTF-8 -> Unicode(UCS-2) 下請け
int HelperFunc::utf8_to_utf16be_sub(unsigned short *pUcs2, const char *pUtf8,int nUtf8Num, bool bCountOnly, bool bBigEndian)
{
    int nUtf8, nUcs2 = 0;
    char cHigh, cLow;

    for ( nUtf8=0; nUtf8 < nUtf8Num;) {
        if ( ( pUtf8[nUtf8] & 0x80) == 0x00) { // 最上位ビット = 0
            if ( bCountOnly == false) {
                pUcs2[nUcs2] = ( pUtf8[nUtf8] & 0x7f);
            }
            nUtf8 += 1;
        } else if ( ( pUtf8[nUtf8] & 0xe0) == 0xc0) { // 上位3ビット = 110
            if ( bCountOnly == false) {
                pUcs2[nUcs2] = ( pUtf8[nUtf8] & 0x1f) << 6;
                pUcs2[nUcs2] |= ( pUtf8[nUtf8+1] & 0x3f);
            }
            nUtf8 += 2;
        } else {
            if ( bCountOnly == false) {
                pUcs2[nUcs2] = ( pUtf8[nUtf8] & 0x0f) << 12;
                pUcs2[nUcs2] |= ( pUtf8[nUtf8+1] & 0x3f) << 6;
                pUcs2[nUcs2] |= ( pUtf8[nUtf8+2] & 0x3f);
            }
            nUtf8 += 3;
        }

        if ( bCountOnly == false) {
            if ( !bBigEndian) {
                // リトルエンディアンにする処理
                cHigh = (pUcs2[nUcs2] & 0xff00) >> 8;
                cLow = (pUcs2[nUcs2] & 0x00ff);
                pUcs2[nUcs2] = (cLow << 8) | cHigh;
            }
        }

        nUcs2 += 1;
    }
    if ( bCountOnly == false) {
        pUcs2[nUcs2] = L'\0';
    }

    return nUcs2;
}

unsigned short* HelperFunc::utf8_to_utf16be(const char *pUtf8Str, int *nNumOut, bool bBigEndian)
{
    int nUtf8Num;
    unsigned short *pUcsStr;

    nUtf8Num = strlen(pUtf8Str); // UTF-8文字列には，'\0' がない
    *nNumOut = utf8_to_utf16be_sub( NULL, pUtf8Str, nUtf8Num, true, bBigEndian);

    pUcsStr = (unsigned short *)calloc((*nNumOut + 1), sizeof(wchar_t));
    utf8_to_utf16be_sub( pUcsStr, pUtf8Str, nUtf8Num, false, bBigEndian);

    return pUcsStr;

}

bool HelperFunc::isBigEndian()
{
	typedef union   
	{   
	  long   LongValue;   
	  char   CharValue[sizeof(long)];   
	}u;

    u tmp;
	tmp.LongValue = 1;
    if(tmp.CharValue[0] ==1)   
	{   
      return true;   
    }
    return false;
}

#if defined(_EXCAT_SOLARIS) || defined(_EXCAT_AIX)||defined(_EXCAT_HPUX)
char* HelperFunc::utf8_to_native(const char *pUtf8Str)
{
    //convert to local char
	iconv_t utf8_to_local_cd;
	char* currentCode = nl_langinfo(CODESET);
	if(currentCode == NULL)
	{
		return HelperFunc::strdup(pUtf8Str);
	}
	if(strcmp("utf8",currentCode) == 0 ||
		strcmp("UTF-8",currentCode) == 0 ||
		strcmp("utf-8",currentCode) == 0 ||
		strcmp("UTF8",currentCode) == 0)
	{
		return HelperFunc::strdup(pUtf8Str);
	}
#ifdef _EXCAT_SOLARIS
	//solarisでgccのlibiconvが、PCKを認識できない
    if(strcmp("PCK",currentCode) == 0 )
	{
        currentCode = "SJIS";
	}
#endif
	utf8_to_local_cd = iconv_open(currentCode, "UTF-8"); 
	if (utf8_to_local_cd == (iconv_t) -1)
	{
		return NULL;
	}
	
    size_t inLen = strlen(pUtf8Str);
	size_t outLen = inLen * 4 + 1;
	char* localStr = new char[outLen];
    char* op = localStr;
    char* ip = (char*)pUtf8Str;
    
	size_t outLeft = outLen;
#ifdef _EXCAT_SOLARIS
    size_t n = iconv(utf8_to_local_cd,(const char**)&ip,&inLen,&op,&outLeft);
#else
    size_t n = iconv(utf8_to_local_cd,&ip,&inLen,&op,&outLeft);
#endif
    iconv_close(utf8_to_local_cd);
    if(n == (size_t)-1)
	{
		delete[] localStr;
		return NULL;
	}
	localStr[outLen-outLeft] = 0;
	return localStr;
}
#else
char* HelperFunc::utf8_to_native(const char *pUtf8Str)
{
    int nNumOut;
	unsigned short* pUcsStr = utf8_to_utf16be(pUtf8Str, &nNumOut, HelperFunc::isBigEndian());
    wchar_t* wp = new wchar_t[nNumOut + 1];
    for(int i=0;i<nNumOut;i++)
        wp[i] = (wchar_t)pUcsStr[i];
    wp[nNumOut] =(wchar_t)0;
	const size_t needLen = wcstombs(NULL,wp,0);
	if(needLen == -1)
	{
        delete[] wp;
		free(pUcsStr);
		return NULL;
	}
	char* localValue = new char[needLen + 1];
	wcstombs(localValue,wp,needLen);
	localValue[needLen] = 0;
        
    delete[] wp;
	free(pUcsStr);
	return localValue;
}
#endif

bool HelperFunc::validateJvmtiError(jvmtiEnv *jvmti, jvmtiError err, const char *str)
{
	//エラーである場合、
	if (err == JVMTI_ERROR_NONE)
		return true;

	//エラーでない場合
	char *errStr = NULL;
	jvmti->GetErrorName(err, &errStr);
	AUTO_REL_JVMTI_OBJECT(errStr);

	int len = MAX_BUF_LEN;
	if(errStr != NULL)
	{
		len += strlen(errStr);
	}
	if(str != NULL)
	{
		len += strlen(str);
	}
	char* buf = new char[len];
	sprintf(buf, "%d - %s : %s", err, (errStr == NULL ? "Unknown" : errStr), 
		(str == NULL ? "" : str));
	LOG4CXX_DEBUG(Global::logger, buf);
    delete[] buf;
	return false;
}

void HelperFunc::addAttrToDOMElement(DOMElement *elem, const char *name, const char *value)
{
	XMLCh xmlValue[MAX_BUF_LEN], xmlName[MAX_BUF_LEN];
	XMLString::transcode(name, xmlName, MAX_BUF_LEN - 1);
	XMLString::transcode(value, xmlValue, MAX_BUF_LEN - 1);
	elem->setAttribute(xmlName, xmlValue);
}

void HelperFunc::getStringObjectValue(JNIEnv *jni, jobject object, char* value, int len)
{
	jstring strValue = (jstring)object;
	if(strValue == NULL)
	{
		*value = 0;
		return;
	}
	
	jsize size = jni->GetStringLength(strValue); 
	if(size == 0)
	{
		*value = 0;
		return;
	}
	
	if (size > len / 4) 
	{
		size = len / 4;	
	}

	const jchar *buf = jni->GetStringChars(strValue, NULL);
	char *valuePos = value;
	for (int i = 0; i < size; i++) 
	{
		char jcharDigits[5];
		sprintf(jcharDigits, "%04x", buf[i]);
		strcpy(valuePos, jcharDigits);
		valuePos += 4;
	}
	value[size*4] = 0;
	jni->ReleaseStringChars(strValue, buf);
}

//get string for char in current locale
char* HelperFunc::getJStringValue(JNIEnv *jni, jstring strValue)
{
	if(strValue == NULL)
		return NULL;

    char* localValue = NULL;

	jsize size = jni->GetStringLength(strValue); 
	if(size == 0)
	{
		localValue = new char[1];
		*localValue = 0;
		return localValue;
	}

	const jchar *buf = jni->GetStringChars(strValue, NULL);
	if(buf == NULL)
	{
		return NULL;//GetStringChars failed
	}
    wchar_t *tmpWideChar = new wchar_t[size + 1];
	for(int i = 0; i < size;i++)
	{
        tmpWideChar[i] = buf[i];
	}
    tmpWideChar[size] = 0;
	const size_t needLen = wcstombs(NULL,tmpWideChar,0);
	if(needLen == -1)
	{
		delete[] tmpWideChar;
		jni->ReleaseStringChars(strValue, buf);
		return NULL;
	}

	localValue = new char[needLen + 1];
	wcstombs(localValue,tmpWideChar,needLen);
	localValue[needLen] = 0;

    delete[] tmpWideChar;
	
	jni->ReleaseStringChars(strValue, buf);
	return localValue;
}


void HelperFunc::getJStringValue(JNIEnv *jni, jstring strValue, char *value, int len)
{
    const char *buf = jni->GetStringUTFChars(strValue, NULL);

    //we want to limit the size of the string to be less than len
	int i = 0;
	int utflen = strlen(buf);
	int dt = 0;
	while(i < len -1 && i < utflen)
	{
		char ch = buf[i];
		
		if((ch & 0x80) == 0)
		{
			//one byte char
			dt = 1;
		}else
        if((ch & 0xE0) == 0xE0)
		{
			//three byte char
			dt = 3;
		}else
		{
			//two byte char
			dt = 2;
		}
		i+=dt;
	}

    if(i >= len -1)
	{
       //UTF-8で2バイト、３バイトコードがあるので、
		//残った分を捨てる
	   int halfbytenum = 0;
	   if(i == len -1)
	   {
		   halfbytenum = 0;
	   }else
	   {
		   halfbytenum = len -1 -(i -dt);
	   }
	    
	   strncpy(value,buf,len -1  - halfbytenum);
       *(value + len -1 - halfbytenum) = 0;
	}
    else
	{
	    strncpy(value,buf,len -1);
        *(value + len -1) = 0;
	}

	jni->ReleaseStringUTFChars(strValue, buf);
}

/**
 * Javaの基本型かどうかを判断する 
 */
bool  HelperFunc::isBasicType(const char descriptor)
{
	switch (descriptor)
	{
	case 'B':
    case 'C':
    case 'D':
    case 'F':
    case 'I':
    case 'J':
    case 'S':
    case 'Z':
		return true;
    default:
		return false;
	}
}

//get slot number of params
int HelperFunc::getMethodParamNum(const char *methodSig)
{
	if (methodSig == NULL)
		return 0;

	//find the position of the char '('
	const char *paramBegin = strchr(methodSig, '(');
	if (paramBegin == NULL)
		return 0;

	//find the position of the char ')'
	const char *paramEnd = strchr(methodSig, ')');
	if (paramEnd == NULL)
		return 0;
	
	paramBegin++;
	paramEnd--;

	const char *pos = paramBegin;
	int count = 0;

    while(pos <= paramEnd)
    {
		if(isBasicType(*pos))
		{  //basic type
			count++;
			if(*pos == 'J' || *pos == 'D')
			{
               count++; //two slot needed
			}
			pos++;
        }else
        if (*pos == 'L')
		{ //object type
		  count++;
          pos = strchr(pos, ';');
		  pos++;
		}else
        if(*pos == '[')
        { //array
			count++;
			while(pos <= paramEnd && *pos=='[')
            {
				pos++;
			}
            if(isBasicType(*pos))
            { 
				pos++;
			}else
            if(*pos == 'L')
            { //object array
			   pos = strchr(pos, ';');
               pos++;
			}//no else :we trust java is correct

		}//no else :we trust java is correct
	}

	return count;
}

//指定メソッドの引く数タイプを取得する(varTypes[]に設定する)
void HelperFunc::getMethodParamType(char * methodSig,int paramNum,bool isStaticMethod,
									char* varTypes)
{
	if (methodSig == NULL || paramNum <=0)
		return;

	//find the position of the char '('
	const char *paramBegin = strchr(methodSig, '(');
	if (paramBegin == NULL)
		return;

	//find the position of the char ')'
	const char *paramEnd = strchr(methodSig, ')');
	if (paramEnd == NULL)
		return;
	
	paramBegin++;
	paramEnd--;

	const char *pos = paramBegin;
	int count = 0;
    if(isStaticMethod)
	{
        count = 0;//no this
	}else
	{
		count = 1;
	}
    while(pos <= paramEnd)
    {
		if(isBasicType(*pos))
		{  //basic type
			varTypes[count] = *pos;
			count++;
			if(*pos == 'J' || *pos == 'D')
			{
               count++; //two slot needed
			}
			pos++;
        }else
        if (*pos == 'L')
		{ //object type
          varTypes[count] = 'L';
		  count++;
          pos = strchr(pos, ';');
		  pos++;
		}else
        if(*pos == '[')
        { //array
			varTypes[count] = 'L';
			count++;
			while(pos <= paramEnd && *pos=='[')
            {
				pos++;
			}
            if(isBasicType(*pos))
            { 
				pos++;
			}else
            if(*pos == 'L')
            { //object array
			   pos = strchr(pos, ';');
               pos++;
			}//no else :we trust java is correct

		}//no else :we trust java is correct
	}
}


char *HelperFunc::convertClassSig(const char *classSig)
{
	if (classSig == NULL)
		return NULL;

	//primitive type
	const char *primTypes = "ZCIBSJFD";
	if (strchr(primTypes, *classSig) != NULL)
	{
		char *buf = getPrimitiveType(classSig);
		if (strlen(classSig) > 1)
		{
			char *after = convertClassSig(classSig + 1);
			char *newBuf = new char[strlen(buf) + strlen(after) + 2];
			sprintf(newBuf, "%s,%s", buf, after);

			delete buf;
			delete after;

			return newBuf;
		}
		else
			return buf;
	}

	//array type
	if (strncmp(classSig, "[", 1) == 0)
		return getArrayType(classSig);

	//object type
	return getObjectType(classSig);
}

char *HelperFunc::getPrimitiveType(const char *classSig)
{
	char *buf = new char[MAX_BUF_LEN];
	memset(buf, 0, MAX_BUF_LEN);

	if (*classSig == 'Z')  //boolean
	{
		strcpy(buf, "boolean");
		return buf;
	}
	else if (*classSig == 'C') //char
	{
		strcpy(buf, "char");
		return buf;
	}
	else if (*classSig == 'I') //int
	{
		strcpy(buf, "int");
		return buf;
	} 
	else if (*classSig == 'B') //byte
	{
		strcpy(buf, "byte");
		return buf;
	}
	else if (*classSig == 'S') //short
	{
		strcpy(buf, "short");
		return buf;
	}
	else if (*classSig == 'J') //long
	{
		strcpy(buf, "long");
		return buf;
	}
	else if (*classSig == 'F') //float
	{
		strcpy(buf, "float");
		return buf;
	}
	else if (*classSig == 'D') //double
	{
		strcpy(buf, "double");
		return buf;
	}

	return NULL;
}

char *HelperFunc::getObjectType(const char *classSig)
{
	int len = strlen(classSig);
	//add by Qiu Song on 20090825 for メソッド監視
	if(classSig[0] != 'L' && classSig[len -1] != ';')
	{
		char *buf = new char[len + 1];
		strncpy(buf, classSig , len);
		buf[len] = '\0';
		return buf;
	}
	//end of add by Qiu Song on 20090825 for メソッド監視
	char *buf = new char[len - 1];

	//remove the first char 'L'
	//remove the last char ';'
	strncpy(buf, classSig + 1, len - 2);
	*(buf + len - 2) = '\0';

	//change the char '/' to char '.'
	int count = len - 2;
	int index = 0;
	while(index < count)
	{
		if (*(buf + index) == '/')
			*(buf + index) = '.';

		index++;
	}

	return buf;
}

char *HelperFunc::getArrayType(const char *classSig)
{
	char *buf = convertClassSig(classSig + 1);
	
	int len = strlen(buf);
	char *newBuf = new char[len + 3];
	sprintf(newBuf, "%s[]", buf);

	delete buf;

	return newBuf;
}

char *HelperFunc::convertMethodSig(const char *methodSig)
{
	if (methodSig == NULL)
		return NULL;

	//find the position of the char '('
	const char *paramBegin = strchr(methodSig, '(');
	if (paramBegin == NULL)
		return 0;

	//find the position of the char ')'
	const char *paramEnd = strchr(methodSig, ')');
	if (paramEnd == NULL)
		return 0;
	
	paramBegin++;
	paramEnd--;

	char *buf = new char[MAX_BUF_LEN * 2];
	char *bufPos = buf;
	
	//copy the char '('
	*bufPos = '(';
	bufPos++;

	const char *beginPos = paramBegin;
	const char *endPos = beginPos;
	int count = 0;

	//primitive type
	const char *primTypes = "ZCIBSJFD";
	char *tempBuf = NULL;
	while(beginPos <= paramEnd)
	{
		if (strchr(primTypes, *beginPos) != NULL)
		{
            tempBuf = getPrimitiveType(beginPos);
			beginPos++;
		}else
        if(*beginPos == '[')
		{
            endPos = beginPos;
			while(*endPos == '[')
			{   endPos++;
			}
            
			if(strchr(primTypes, *endPos) == NULL)
			{
                endPos = strchr(beginPos, ';');
			}

		    int len = endPos - beginPos + 1;
		    char* tempObjBuf = new char[len + 1];
		    strncpy(tempObjBuf, beginPos, len);
		    *(tempObjBuf + len) = '\0';
            tempBuf = getArrayType(tempObjBuf);
			delete tempObjBuf;
			beginPos = endPos + 1;
		}
		else
		{
			endPos = strchr(beginPos, ';');

		    int len = endPos - beginPos + 1;
		    char* tempObjBuf = new char[len + 1];
		    strncpy(tempObjBuf, beginPos, len);
		    *(tempObjBuf + len) = '\0';
            tempBuf = convertClassSig(tempObjBuf);
			delete tempObjBuf;
			beginPos = endPos + 1;
		}
		strcpy(bufPos, tempBuf);
		bufPos += strlen(tempBuf);
		delete tempBuf;
		*bufPos = ',';
		bufPos++;

	}
  
	//remove the last char ','
	if (*(bufPos - 1) == ',')
		bufPos--;

	//copy the char ')'
	*bufPos = ')';
	bufPos++;

	if (strcmp("V", paramEnd + 2) == 0)
	{
		strcpy(bufPos, "void");
		*(bufPos + strlen("void")) = '\0';

		return buf;
	}

	char *classSig = convertClassSig(paramEnd + 2);
	strcpy(bufPos, classSig);
	*(bufPos + strlen(classSig)) = '\0';
	delete classSig;

	return buf;
}

int HelperFunc::getLineNumberByLocation(jvmtiLineNumberEntry  *entryTable, jint entryCount, jlocation location)
{
	int lineNumber = 0;

	if (entryCount == 0)
		return lineNumber;

	if (entryCount == 1)
		return entryTable->line_number;

	int startPos = 0;
	int endPos = entryCount - 1;
	/*
	if(location > entryTable[endPos].start_location ||
		location < entryTable[startPos].start_location)
	{
        return lineNumber;
	}*/

	int currentPos = (startPos + endPos) / 2;
	int oldpos = -1;
	while (currentPos >= startPos && currentPos <= endPos) 
	{
		jvmtiLineNumberEntry  *entry = entryTable + currentPos;

		if (currentPos + 1 >= entryCount)
		{
			if (location >= entry->start_location)
			{
				lineNumber = entry->line_number;
				break;
			}
			else
			{
				endPos = currentPos;
				currentPos = (startPos + endPos) / 2;
			}
		}
		else
		{
			if (location >= entry->start_location  && 
				location < (entry + 1)->start_location)
			{
				lineNumber = entry->line_number;
				break;
			} 
			
			if (location < entry->start_location)
			{
				endPos = currentPos;
				currentPos = (startPos + endPos) / 2;
			}
			else
			{
				startPos = currentPos;
				currentPos = (startPos + endPos) / 2 + 1; 
			}
		}
		//以下が、jiangが追加、無限ループを防止するために
		if(currentPos != oldpos)
		{
			oldpos = currentPos;
		}else
		{
           lineNumber = entry->line_number;
		   break;
		}
	}

	return lineNumber;
}

/**
 * パラメータattributesに属性名がattrNameに対応する属性値を取得
 * 
 * @return attrNameに対応する属性値（UTF-8コード）
 */
char* HelperFunc::getAttrValueUtf8(DOMNamedNodeMap *attributes, const char* attrName)
{
	char* retValueUtf8 = NULL;
	unsigned short  *valueUtf16 = NULL;

	XMLCh buf[MAX_BUF_LEN];
	XMLString::transcode(attrName, buf, MAX_BUF_LEN - 1);
	DOMNode *node = attributes->getNamedItem(buf);
	if (node != NULL)
	{
		valueUtf16 = (unsigned short*)(node->getNodeValue());
        int utf16len = 0;
        while(valueUtf16 != NULL && valueUtf16[utf16len] !=0)
            utf16len++;
		if (valueUtf16 != NULL)
		{
			int len = HelperFunc::utf16be_to_utf8_sub(NULL, valueUtf16, utf16len, true);
			if (len > 0)
			{
				retValueUtf8 = new char[len + 1];
				memset(retValueUtf8, 0, len + 1);
				HelperFunc::utf16be_to_utf8_sub(retValueUtf8, valueUtf16, utf16len, false);
			}
			else
			{
				retValueUtf8 = new char[1];
				strcpy(retValueUtf8, "");
			}
		}
	}
	return retValueUtf8;
}


bool HelperFunc::hasEnoughFreeDiskSpace()
{
	char* dumpFilePath = Global::getConfig()->getDumpFilePath();
	unsigned __int64 needDiskSpace = Global::getConfig()->getMinDiskSpace();
#ifdef _LINUX
	struct statvfs disk_statfs;
	int ret = statvfs(dumpFilePath, &disk_statfs);
	if (ret == 0)
	{
		unsigned __int64 freeSize = (unsigned __int64)disk_statfs.f_bavail;
		freeSize *= disk_statfs.f_bsize;
		if (freeSize >= needDiskSpace)
		{
			return true;
		}
	}
	return false;
#else
	ULARGE_INTEGER size1, size2, size3;
    if (TRUE == GetDiskFreeSpaceEx(dumpFilePath, &size1, &size2, &size3))
	{
		if (size1.QuadPart >= needDiskSpace)
		{
			return true;
		}
	}
	return false;
#endif
}

//add by Qiu Song on 20090818 for スレッド状態/CPU時間/待機理由の追加
//スレッド状態(int→文字列)を取得する
char* HelperFunc::getThreadStatusString(jint threadStatus)
{
	switch(threadStatus & JVMTI_JAVA_LANG_THREAD_STATE_MASK)
	{
		case JVMTI_JAVA_LANG_THREAD_STATE_NEW:
			return "new";
		case JVMTI_JAVA_LANG_THREAD_STATE_TERMINATED:
			return "terminated";
		case JVMTI_JAVA_LANG_THREAD_STATE_RUNNABLE:
			return "runnable";
		case JVMTI_JAVA_LANG_THREAD_STATE_BLOCKED:
			return "blocked";
		case JVMTI_JAVA_LANG_THREAD_STATE_WAITING:
			return "waiting";
		case JVMTI_JAVA_LANG_THREAD_STATE_TIMED_WAITING:
			return "timed_waiting";
		default:
			return "runnable";
	}
}

//スレッド待ち理由を取得する
char* HelperFunc::getThreadWaitingReason(jint threadStatus)
{
	if (threadStatus & JVMTI_THREAD_STATE_IN_OBJECT_WAIT)
	{
		return "wait monitor object";
	}
	else if (threadStatus & JVMTI_THREAD_STATE_SLEEPING)
	{
		return "thread sleeping";
	}
	else if(threadStatus & JVMTI_THREAD_STATE_PARKED)
	{
		return "thread is parked";
	}
	return "";
}

//指定タイプスレッドダンプ
bool HelperFunc::shouldDumpCurrentThread(jvmtiEnv* jvmti, jthread curThread, char* threadStatus, int threadPriority)
{
	//スレッド状態の判断処理
	if( isCurrentThreadStatusMatch(jvmti, curThread, threadStatus) == false )
	{
		return false;
	}

	//スレッド優先度の判断処理
    if( isCurrentThreadPriorityMatch(jvmti, curThread, threadPriority) == false)
	{
		return false;
	}
	return true;
}

//カレントスレッドの状態は指定されたと一致するかどうか
bool HelperFunc::isCurrentThreadStatusMatch(jvmtiEnv* jvmti, jthread curThread, char* threadStatus)
{
	if( threadStatus == NULL || strcmp(threadStatus, "all") == 0)
	{
		return true;
	}
	else
	{
		jint nThreadStatus;
		jvmtiError err = jvmti->GetThreadState(curThread,&nThreadStatus);
		if(err != JVMTI_ERROR_NONE)
		{
			Global::logger->logError("failed to get thread status ,err cd =%d",err);
			return true;
		}
		int nThreadState= (nThreadStatus & JVMTI_JAVA_LANG_THREAD_STATE_MASK);

		//稼働中スレッドのみダンプの場合
		if( strcmp(threadStatus, "runnable") == 0 &&
			nThreadState == JVMTI_JAVA_LANG_THREAD_STATE_RUNNABLE)
		{
			return true;
		}
		//モニター待ち状態のみダンプの場合
		else if( strcmp(threadStatus, "wait") == 0 &&
			     nThreadState == JVMTI_JAVA_LANG_THREAD_STATE_WAITING)
		{
			return true;
		}
		return false;
	}
}


//カレントスレッドの優先度は指定されたと一致するかどうか
bool HelperFunc::isCurrentThreadPriorityMatch(jvmtiEnv* jvmti, jthread curThread, int threadPriority)
{
	if(threadPriority == 0)
	{
		return true;
	}
	else
	{
		jvmtiThreadInfo threadInfo;
		jvmtiError err = jvmti->GetThreadInfo(curThread, &threadInfo);
		if(err != JVMTI_ERROR_NONE)
		{
			Global::logger->logError("failed to get thread info ,err cd =%d",err);
			return true;
		}
		if(threadInfo.priority >= threadPriority)
		{
			return true;
		}
		return false;
	}
}

//カレントオブジェクトは指定されたタイプかどうか判断する
bool HelperFunc::shouldDumpCurrentObject(jvmtiEnv* jvmti, JNIEnv *jni, jobject curObject)
{
	char* javaClassName = getClassNameFromJobject(jvmti, jni, curObject);
	bool bResult = false;
	if(javaClassName != NULL)
	{
		bResult = HelperFunc::isFilterObjType(javaClassName);
		delete javaClassName;
		javaClassName = NULL;
	}
	return bResult;
}

//指定されたタイプかどうか判断する
bool HelperFunc::isFilterObjType(char *javaClassName)
{
	if( strcmp(javaClassName, "java.math.BigDecimal") == 0 || strcmp(javaClassName, "java.math.BigDecimal[]") == 0 ||
		strcmp(javaClassName, "java.math.BigInteger") == 0 || strcmp(javaClassName, "java.math.BigInteger[]") == 0 ||
		strcmp(javaClassName, "java.lang.Byte") == 0 || strcmp(javaClassName, "java.lang.Byte[]") == 0 ||
		strcmp(javaClassName, "java.lang.Double") == 0 || strcmp(javaClassName, "java.lang.Double[]") == 0 ||
		strcmp(javaClassName, "java.lang.Float") == 0 || strcmp(javaClassName, "java.lang.Float[]") == 0 ||
		strcmp(javaClassName, "java.lang.Integer") == 0 || strcmp(javaClassName, "java.lang.Integer[]") == 0 ||
		strcmp(javaClassName, "java.lang.Long") == 0 || strcmp(javaClassName, "java.lang.Long[]") == 0 ||
		strcmp(javaClassName, "java.lang.Short") == 0 || strcmp(javaClassName, "java.lang.Short[]") == 0 ||
		strcmp(javaClassName, "java.lang.Character") == 0 || strcmp(javaClassName, "java.lang.Character[]") == 0 ||
		strcmp(javaClassName, "java.lang.StringBuffer") == 0 || strcmp(javaClassName, "java.lang.StringBuffer[]") == 0 ||
		strcmp(javaClassName, "java.lang.StringBuilder") == 0 || strcmp(javaClassName, "java.lang.StringBuilder[]") == 0 ||
		strcmp(javaClassName, "java.lang.Boolean") == 0 || strcmp(javaClassName, "java.lang.Boolean[]") == 0 ||
		strcmp(javaClassName, "java.util.Date") == 0 || strcmp(javaClassName, "java.util.Date[]") == 0 ||
		strcmp(javaClassName, "java.sql.Time") == 0 || strcmp(javaClassName, "java.sql.Time[]") == 0 ||
		strcmp(javaClassName, "java.sql.Timestamp") == 0 || strcmp(javaClassName, "java.sql.Timestamp[]") == 0 ||
		strcmp(javaClassName, "java.lang.String[]") == 0 )
    {
		return true;
	}
	return false;
}

//メソッドsigからリターンのタイプを取得する
char* HelperFunc::getReturnTypeFromMethodSig(const char *methodSig)
{
	if (methodSig == NULL)
		return NULL;

	//find the position of the char '('
	const char *paramBegin = strchr(methodSig, '(');
	if (paramBegin == NULL)
		return 0;

	//find the position of the char ')'
	const char *paramEnd = strchr(methodSig, ')');
	if (paramEnd == NULL)
		return 0;

	char* returnType = new char[strlen(paramEnd)];
	memcpy(returnType, paramEnd + 1, strlen(paramEnd) -1);
	returnType[strlen(paramEnd) - 1] = '\0';
	return returnType;
}

//除外例外一覧ファイルのパスを取得する
char* HelperFunc::getExcludeExceptionListFilePath()
{
	char *filePath = getenv(EXCAT_HOME);
	if(filePath == NULL)
	{
		return NULL;
	}
	if(filePath[strlen(filePath) -1] != FILE_SPERATOR_C)
	{
		strcat(filePath, FILE_SPERATOR);
	}
	strcat(filePath, EXCLUDE_EXCEPTION_FILE_NAME);
	return filePath;
}

//「null」の文字列を戻す
char* HelperFunc::getNullString()
{
	char* returnValue = new char[5];
	strcpy(returnValue, "null");
	returnValue[4] = '\0';
	return returnValue;
}

//指定文字列にwild codeがあるかどうか
bool HelperFunc::hasWildCode(const char* stringBuf)
{
    if(strchr(stringBuf, '*') != NULL || strchr(stringBuf, '?') != NULL)
	{
		return true;
	}
	return false;
}

//文字列が指定されたのとマッチかどうか判断する
bool HelperFunc::doesStringMatch(const char* stringBuf, const char* matchString)
{
	//wild codeがないの場合
	if(HelperFunc::hasWildCode(matchString) == false)
	{
		return strcmp(stringBuf, matchString) == 0? true : false;
	}
	//wild codeがあるの場合
	else
	{
	    return doesStringMatchWithWildCode(stringBuf, matchString);
	}
}

bool HelperFunc::doesStringMatchWithWildCode(const char* stringBuf, const char* matchString)
{
	//「*」の場合
	if(strcmp(matchString, "*") == 0)
	{
        return true;
	}
    
	//wild codeの前と後の文字列を取得する
	char strBeforeWildCode[MAX_BUF_LEN];
	char strAfterWildCode[MAX_BUF_LEN];
	char stringBufLeft[MAX_BUF_LEN];
	char stringBufRight[MAX_BUF_LEN];
	memset(strBeforeWildCode, 0, MAX_BUF_LEN);
	memset(strAfterWildCode, 0, MAX_BUF_LEN);
	memset(stringBufLeft, 0, MAX_BUF_LEN);
	memset(stringBufRight, 0, MAX_BUF_LEN);

	getStringBeforeAndAfterWildCode(matchString, strBeforeWildCode, strAfterWildCode);
	//先頭は「*」の場合
	if(strlen(strBeforeWildCode) == 0)
	{
		return doesStringMatchWithFirstWildCode(stringBuf, matchString, stringBufRight);
	}

	//先頭は「*」以外の場合
	else
	{
        return doesStringMatchWithFirstNotWildCode(stringBuf, matchString, strBeforeWildCode,
			                                       strAfterWildCode, stringBufLeft, stringBufRight);
	}
}

void HelperFunc::getStringBeforeAndAfterWildCode(const char* stringBuf, char* strBeforeWildCode, char* strAfterWildCode)
{
	int nStringLength = strlen(stringBuf);
	int nPos = 0;
	int nBeforeWildCodePos = 0;
	for(nPos=0; nPos < nStringLength; nPos++,nBeforeWildCodePos++)
	{
		if(stringBuf[nPos] == '*' && (nPos +1) < nStringLength)
		{
			if(stringBuf[nPos +1] == '*' || stringBuf[nPos +1] == '?')
			{
				nPos++;
			}
			else
			{
				break;
			}
		}
		else if(stringBuf[nPos] == '*' || stringBuf[nPos] == '?')
		{
			break;
		}
		strBeforeWildCode[nBeforeWildCodePos] = stringBuf[nPos];
	}
	strBeforeWildCode[strlen(strBeforeWildCode) + 1 ] = '\0';
	strcat(strAfterWildCode, stringBuf+nPos);
	strAfterWildCode[strlen(strAfterWildCode) +1 ] = '\0';
}

void HelperFunc::getStringBeforeAndAfterPosition(const char* stringBuf, int nPos, char* stringBufLeft, char* stringBufRight)
{
	int nStringLength = strlen(stringBuf);
	for(int i = 0; i < nPos; i++)
	{
		stringBufLeft[i] = stringBuf[i];
	}
	stringBufLeft[nPos] = '\0';
	strcat(stringBufRight, stringBuf+nPos);
	stringBufRight[strlen(stringBufRight) +1 ] = '\0';
}

bool HelperFunc::doesStringMatchWithFirstWildCode(const char* stringBuf, const char* matchString, char* stringBufRight)
{
	memcpy(stringBufRight, matchString + 1, strlen(matchString) - 1);
	stringBufRight[strlen(matchString)] = '\0';
	//「?」の場合
	if(matchString[0] == '?')
	{
		if(strlen(stringBuf) == 0)
		{
			return false;
		}
		return doesStringMatch(stringBuf + 1, stringBufRight);
	}
	//「*」の場合
	for(int i= 0; i < strlen(stringBuf); i ++)
	{
		if(doesStringMatch(stringBuf + i, stringBufRight))
		{
			return true;
		}
	}
	return false;
}

bool HelperFunc::doesStringMatchWithFirstNotWildCode(const char* stringBuf, const char* matchString,
													 char* strBeforeWildCode, char* strAfterWildCode,
													 char* stringBufLeft, char* stringBufRight)
{
	int nBeforeWildCode = strlen(strBeforeWildCode);
	if(nBeforeWildCode > strlen(stringBuf))
	{
		return false;
	}

	getStringBeforeAndAfterPosition(stringBuf, nBeforeWildCode, stringBufLeft, stringBufRight);
    if(strcmp(stringBufLeft, strBeforeWildCode) != 0)
	{
		return false;
	}
	else
	{
		return doesStringMatch(stringBufRight, strAfterWildCode);
	}

}

//廃棄しました(comment by Qiu Song on 20090916)
bool HelperFunc::isWildCodeCorrect(const char* matchString)
{
    if(strstr(matchString, "**") != NULL || strstr(matchString, "*?") != NULL)
	{
		return false;
	}
	return true;
}

//booleanの戻り値はtrueまたはfalseに変更
void HelperFunc::convertBoolValue(char* type, char* value)
{
    if(type == NULL || value == NULL || strcmp(type, "boolean") != 0)
	{
		return;
	}
	if(strcmp(value, "1") == 0)
	{
		delete value;
		value = new char[5];
		memcpy(value, "true", 4);
		value[4] = '\0';
	}
	else if(strcmp(value, "0") == 0)
	{
		delete value;
		value = new char[6];
		memcpy(value, "false", 5);
		value[5] = '\0';
	}
}

//charの戻り値を表示できるように変更
void HelperFunc::convertCharValue(char* type, char* value)
{
    if(type == NULL || value == NULL || strcmp(type, "char") != 0)
	{
		return;
	}
	
	int nChar = atoi(value);
	delete value;
	value = new char[5];
	sprintf(value, "%04x", nChar);
	value[4] = '\0';
}

//jobjectのクラス名を取得する
char* HelperFunc::getClassNameFromJobject(jvmtiEnv* jvmti, JNIEnv *jni, jobject curObject)
{
	//get object signature
	jclass clazz = jni->GetObjectClass(curObject);
	if(clazz == NULL)
    {
		Global::logger->logError("failed to call GetObjectClass in HelperFunc::getClassNameFromJobject.");
		return NULL;
	}
    AUTO_REL_JNI_LOCAL_REF(jni, clazz);	

	//get class status
	jint status;
	jvmtiError err = jvmti->GetClassStatus(clazz,&status);
	if(err != JVMTI_ERROR_NONE)
	{
		Global::logger->logError("failed to call GetClassStatus in HelperFunc::getClassNameFromJobject,error cd =%d.",err);
		return NULL;//not exit
	}
	
	char *sig = NULL;
	err = jvmti->GetClassSignature(clazz, &sig, NULL);
	if(err != JVMTI_ERROR_NONE)
	{
		Global::logger->logError("failed to call GetClassSignature in HelperFunc::getClassNameFromJobject,error cd =%d.",err);
		return NULL;//not exit
	}
	AUTO_REL_JVMTI_OBJECT(sig);

    //get attribute type
	char* javaClassName = HelperFunc::convertClassSig(sig);
	return javaClassName;
}

//メソッドのクラス名を取得する
char* HelperFunc::getMethodDeclaringClassName(jvmtiEnv* jvmti, JNIEnv *jni, jmethodID methodId)
{
	jclass declaringClass = NULL;
	jvmtiError err = jvmti->GetMethodDeclaringClass(methodId, &declaringClass);
	if (err != JVMTI_ERROR_NONE){
		return NULL;
	}
	AUTO_REL_JNI_LOCAL_REF(jni, declaringClass);

	char *classSig = NULL;
	err = jvmti->GetClassSignature(declaringClass, &classSig, NULL);
	if (err != JVMTI_ERROR_NONE){
		return NULL;
	}
	AUTO_REL_JVMTI_OBJECT(classSig);
	char* javaClassName = HelperFunc::convertClassSig(classSig);
	return javaClassName;
}

//クラス名からパッケージ名を取得する
char* HelperFunc::getPackageNameFromClassName(char* className)
{
	char* packageName = new char[MAX_BUF_LEN * 4 + 1];
	memset(packageName, 0 , MAX_BUF_LEN * 4 + 1);
	sprintf(packageName, "PACKAGE_%s", className);
    char* tokon = strchr(packageName, '.');
	char* preTokon = NULL;
	while(tokon)
	{
		preTokon = tokon + 1;
		tokon = strchr(preTokon, '.');
	}
	if(preTokon != NULL)
	{
		preTokon = preTokon -1;
		preTokon[0] = '\0';
	}
	packageName = HelperFunc::replaceChar(packageName, '.', '_');
 	return packageName;
}


#ifdef _WINDOWS
//WindowsのMACアドレスを取得する関数
bool HelperFunc::CheckMacAddressForWindows(char* mac)
{
	if(mac == NULL)
	{
		return false;
	}
	NCB ncb; 
    typedef struct _ASTAT_ 
    { 
        ADAPTER_STATUS adapt; 
        NAME_BUFFER NameBuff [30]; 
    } ASTAT, * PASTAT; 
    ASTAT Adapter; 
    LANA_ENUM lana_enum; 
    
    UCHAR uRetCode; 
    memset( &ncb, 0, sizeof(ncb) ); 
    memset( &lana_enum, 0, sizeof(lana_enum)); 
    
    ncb.ncb_command = NCBENUM; 
    ncb.ncb_buffer = (unsigned char *) &lana_enum; 
    ncb.ncb_length = sizeof(LANA_ENUM); 
    uRetCode = Netbios( &ncb );
    if( uRetCode != NRC_GOODRET )
	{
        return false; 
    }

    for( int lana=0; lana<lana_enum.length; lana++ ) 
    { 
        char macTemp[20];
		memset(macTemp,0,sizeof(macTemp));

		ncb.ncb_command = NCBRESET; 
        ncb.ncb_lana_num = lana_enum.lana[lana]; 
        uRetCode = Netbios( &ncb ); 
		if( uRetCode != NRC_GOODRET )
		{
			return false;
		}
		::ZeroMemory(&ncb,sizeof(ncb));
		ncb.ncb_command	= NCBASTAT;
		ncb.ncb_lana_num = lana_enum.lana[lana];
		strcpy( (char* )ncb.ncb_callname, "*" );
		ncb.ncb_buffer = (unsigned char *)&Adapter;
		ncb.ncb_length = sizeof(Adapter);
		uRetCode = Netbios( &ncb );
		if( uRetCode != NRC_GOODRET )
		{
			return false;
		}
		sprintf(macTemp,"%02X:%02X:%02X:%02X:%02X:%02X", 
        Adapter.adapt.adapter_address[0], 
        Adapter.adapt.adapter_address[1], 
        Adapter.adapt.adapter_address[2], 
        Adapter.adapt.adapter_address[3], 
        Adapter.adapt.adapter_address[4], 
        Adapter.adapt.adapter_address[5] );

		if(strcmp(mac, macTemp) == 0)
		{
			return true;
		}
    } 
    return false;
}
#endif

#ifdef _LINUX
//LinuxのMACアドレスを取得する関数
bool HelperFunc::CheckMacAddressForLinux(char* mac)
{
#if defined(_EXCAT_SOLARIS) || defined(_EXCAT_HPUX)
	return false;
#else
	bool bReturn = false;
	register int fd, intrface, retn = 0;
	struct ifreq buf[MAXINTERFACES];
	struct arpreq arp;
	struct ifconf ifc;
	if((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
	{
		ifc.ifc_len = sizeof buf;
		ifc.ifc_buf = (caddr_t) buf;
		if(!ioctl (fd, SIOCGIFCONF, (char *) &ifc))
		{
			//get the info of interface
			intrface = ifc.ifc_len / sizeof (struct ifreq);
			//get the ip and mac address
			while(intrface-- > 0)
			{
				//mac address
				if(!(ioctl (fd, SIOCGIFHWADDR, (char *) &buf[intrface])))
				{
					char macTemp[20];
					memset(macTemp,0,sizeof(macTemp));
					sprintf(macTemp,"%02X:%02X:%02X:%02X:%02X:%02X", 
								(unsigned char)buf[intrface].ifr_hwaddr.sa_data[0], 
								(unsigned char)buf[intrface].ifr_hwaddr.sa_data[1], 
								(unsigned char)buf[intrface].ifr_hwaddr.sa_data[2], 
								(unsigned char)buf[intrface].ifr_hwaddr.sa_data[3], 
								(unsigned char)buf[intrface].ifr_hwaddr.sa_data[4], 
								(unsigned char)buf[intrface].ifr_hwaddr.sa_data[5] );
					if(strcmp(mac, macTemp) == 0)
					{
						bReturn = true;
						break;
					}
				}
				else
				{
					break;
				}
			}//end of while(intrface-- > 0)
		}
	}
	close (fd);
	return bReturn; 
#endif
}
#endif

//add by Qiu Song on 20091120 for SolarisとHP-UXのMacアドレスを取得する関数
#ifdef _EXCAT_SOLARIS

//SolarisシステムのMACアドレスを取得する関数
bool HelperFunc::CheckMacAddressForSolaris(char* mac)
{
	bool bReturn = false;
    FILE *cmd=popen("grep \"Ethernet\" /var/adm/messages.* | more ", "r");
    char tmp[256]={0x0};
    while(fgets( tmp, sizeof(tmp), cmd)!=NULL)
	{
		char* strBuff = strstr(tmp," ");
		char* macAddress;
		while(1)
		{
			 macAddress = strBuff;
			 char* strSearch = strstr(strBuff, " ");
			 if(strSearch == NULL)
			 {
				break;
			 }
			 strBuff = strSearch+1;
		}
    
		//we trust solaris is always right
		char macTemp[18];
		memset(macTemp,0,sizeof(macTemp));
		sprintf(macTemp,"%017d",0);
		mac[17]='\0';
		int nMacIndex = 16;
		int nMacLength = strlen(macAddress) - 2;
		for(int i= nMacLength; i >= 0; i--)
		{
			if(macAddress[i] == ':')
			{
				 if(nMacIndex % 3 != 2)
				 {
					 nMacIndex--;
				 }
			}
			if(macAddress[i] >= 'a' && macAddress[i] <= 'f')
			{
				macAddress[i] -= 32;
			}
			macTemp[nMacIndex--] = macAddress[i];
		}
		if(strcmp(macTemp, mac) == 0)
		{
			bReturn = true;
			break;
		}
	}
    pclose(cmd);
    return bReturn;
};

#endif

#ifdef _EXCAT_HPUX

//HPUXシステムのMACアドレスを取得する関数
bool HelperFunc::CheckMacAddressForHPUX(char* macAddressKey)
{
	char macAddress[13];
	macAddress[12] = '\0';
	int macIndex = 0;
	for(int i = 0; i < strlen(macAddressKey); i++)
	{
		if(macAddressKey[i] == ':')
		{
			continue;
		}
		macAddress[macIndex] = macAddressKey[i];
		macIndex++;
	}

    FILE *cmd=popen("lanscan", "r");
    char tmp[256]={0x0};
    bool bMacAddressMatch = false;
	while(fgets( tmp, sizeof(tmp), cmd)!=NULL)
	{	
		if(strstr(tmp, macAddress))
		{
			bMacAddressMatch = true;
			break;
		}
	}
    pclose(cmd);
    return bMacAddressMatch;
}

#endif


bool HelperFunc::CheckMacAddress(char* macAddressKey)
{
#ifndef _EXCAT_HPUX
	//modified by Qiu Song on 20091120 for SolarisとHPUXのMacアドレスの取得
#ifdef _EXCAT_SOLARIS
	return CheckMacAddressForSolaris(macAddressKey);
#elif defined(_LINUX)
    return CheckMacAddressForLinux(macAddressKey);
#else
	return CheckMacAddressForWindows(macAddressKey);
#endif
#else
	//HPUXの場合、Macアドレスのチェック対応
	return CheckMacAddressForHPUX(macAddressKey);
#endif
}
//end of add by Qiu Song on 20091120 for SolarisとHP-UXのMacアドレスを取得する関数

//親フォルダ名を取得する
char* HelperFunc::getPreForldName(char* pathName)
{
	//NULLの場合
	if(pathName == NULL || strlen(pathName) == 0)
	{
		return NULL;
	}

	int nSperatorPos = findLastCharOf(pathName, FILE_SPERATOR_C);
	if(nSperatorPos == -1)
	{
		return NULL;
	}

	char* preFoldName = new char[nSperatorPos + 1];
	memset(preFoldName, 0 , nSperatorPos + 1);
	memcpy(preFoldName, pathName, nSperatorPos);
	preFoldName[nSperatorPos] = '\0';
	return preFoldName;
}

//指定文字列に、最後の指定文字を探す
int HelperFunc::findLastCharOf(char* strPathName, char strChar)
{
	if(strPathName == NULL)
	{
		return -1;
	}
	for(int i = strlen(strPathName) - 1; i >= 0; i--)
	{
		if( strPathName[i] == strChar )
		{
			return i;
		}
	}
	return -1;
}

//フォルダを削除する関数
int HelperFunc::removeDir(char* dirPath)
{
    int nRet = rmdir(dirPath);
	return nRet;
}

//指定フォルダまで、空くフォルダを削除する関数
int HelperFunc::removeDirUntil(char* dirPathStart, char* dirPathEnd)
{
	if(dirPathStart == NULL || strlen(dirPathStart) == 0 ||
	   dirPathEnd == NULL || strlen(dirPathEnd) == 0 ||
	   strcmp(dirPathStart, dirPathEnd) == 0)
	{
		return -1;
	}
    char* prePath = getPreForldName(dirPathStart);
	if(strcmp(prePath, dirPathEnd) == 0)
	{
		return -1;
	}
	else
	{
		int nRet = removeDir(prePath);
		if(nRet != 0)
		{
			return -1;
		}
		else
		{
			removeDirUntil(prePath, dirPathEnd);
		}
	}
	if( prePath != NULL )
	{
		delete prePath;
		prePath = NULL;
	}
	return 0;
}

//ダンプファイルを削除する
void  HelperFunc::removeDumpFile(char* filePath)
{
    if (HelperFunc::isValidFile(filePath))
	{
		//delete dump file
		HelperFunc::removeFile(filePath);
		//delete dir
		int nRet = HelperFunc::removeDirUntil(filePath, Global::getConfig()->getDumpFilePath());
	}
}
//end of add by Qiu Song on 20090818 for スレッド状態/CPU時間/待機理由の追加