#pragma warning(disable : 4503)
#pragma warning(disable : 4786) //警告C4786を禁止

#if !defined(_HELPERFUNC_H)
#define _HELPERFUNC_H

#include <jvmti.h>

#include <xercesc/dom/DOM.hpp>
#include "Define.h"

#ifndef XERCESC20
XERCES_CPP_NAMESPACE_USE
#endif

BEGIN_NS(NS_COMMON)


/**
 * このクラスでは、様々なヘルパー関数を提供する。
 */
class HelperFunc  
{
private:
	/**
	 * コンストラクタ[禁止]
	 */
	HelperFunc() {};

	/**
	 * デストラクタ[禁止]
	 */
	virtual ~HelperFunc() {};

	/**
	 * primitiveタイプをJVM内部の形式から普通の形式に変換
	 * 
	 * @param internalType JVM内部形式のprimitiveタイプ
	 * @return 普通形式のprimitiveタイプ
	 */
	static char *getPrimitiveType(const char *internalType);

	/**
	 * オブジェクトのタイプをJVM内部の形式から普通の形式に変換
	 * 
	 * @param internalType JVM内部形式のオブジェクト・タイプ
	 * @return 普通形式のオブジェクト・タイプ
	 */
	static char *getObjectType(const char *internalType);

	/**
	 * 配列タイプをJVM内部の形式から普通の形式に変換
	 * 
	 * @param internalType JVM内部形式の配列タイプ
	 * @return 普通形式の配列タイプ
	 */
	static char *getArrayType(const char *internalType);

	/**
	 * Javaの基本型かどうかを判断する 
	 */
	static bool isBasicType(const char descriptor);
public:

	/**
	 * パラメータ「err」をチェックし、それがエラーである場合、ログに出力
	 *
	 * @param jvmti jvmti環境を示すオブジェクト
	 * @param err チェック対象となるjvmtiErrorタイプのエラー
	 * @param str 「err」がエラーである場合、出力したいメッセージ
	 * @return <ul>
	 *			<li>true: エラーあり</li>
	 *			<li>false: エラーなし</li>
	 *		   </ul>
	 */
	static bool validateJvmtiError(jvmtiEnv *jvmti, jvmtiError err, const char *str);

	/**
	 * DOMエレメントにパラメータ「name」と「value」に示される属性を追加
	 *
	 * @param elem 追加先となるDOMエレメント
	 * @param name 属性名
	 * @param value 属性の値
	 */
	static void addAttrToDOMElement(DOMElement *elem, const char *name, const char *value);

	/**
	 * Stringタイプのオブジェクトの値を取得
	 * 
	 * @param jni JNI環境を示すオブジェクト
	 * @param object 処理対象となるオブジェクト
	 * @param value Stringタイプのオブジェクトの値。このパラメータがこの関数外で生成される。
	 * @param len パラメータ「value」の長さ
	 */
	static void getStringObjectValue(JNIEnv *jni, jobject object, char* value, int len);

	static void getJStringValue(JNIEnv *jni, jstring strValue, char *value, int len);
	static char* getJStringValue(JNIEnv *jni, jstring strValue);
	/**
	 * 渡されたメソッドのシグネチャによって当該メソッドのパラメータの数を取得
	 * 
	 * @param methodSig メソッドのシグネチャ
	 * @return メソッド・パラメータの数
	 */
	static int getMethodParamNum(const char *methodSig);

    static void getMethodParamType(char * methodSig,int paramNum,bool isStaticMethod,
									char* varTypes);
	/**
	 * クラスのシグネチャをJVM内部用の形式から普通の形式に変換
	 * 
	 * @param internalType JVM内部形式のクラス・シグネチャ
	 * @return 普通形式のクラス・シグネチャ
	 */
	static char *convertClassSig(const char *internalType);

	/**
	 * メソッドのシグネチャをJVM内部用の形式から普通の形式に変換
	 * 
	 * @param methodSig JVM内部形式のメソッド・シグネチャ
	 * @return 普通形式のメソッド・シグネチャ
	 */
	static char *convertMethodSig(const char *methodSig);

	/**
	 * パラメータentryTableでlocationに対応する行番号を検索
	 * 
	 * @param entryTable 行番号情報を含む配列
	 * @param entryCount 行番号情報配列のエントリ・サイズ
	 * @param location 検索対象となる行の位置
	 * @return 検索結果となる行番号
	 */
	static int getLineNumberByLocation(jvmtiLineNumberEntry *entryTable, 
		jint entryCount, jlocation location);

	/**
	 * パラメータattributesに属性名がattrNameに対応する属性値を取得
	 * 
	 * @return attrNameに対応する属性値（UTF-8コード）
	 */
	static char* getAttrValueUtf8(DOMNamedNodeMap *attributes, const char* attrName);

	/**
	 * 指定したディレクトリ名が存在するかを判断する
	 */
	static bool isValidDir(char* dirName);
	static bool isValidFile(const char* fileName);
    static bool makeDirectory(char *dirName);
	static int utf16be_to_utf8_sub( char *pUtf8, const unsigned short *pUcs2, int nUcsNum, bool bCountOnly);
    static char *utf16be_to_utf8(const jchar utf16char, int *nBytesOut);
	static void getLongTypeValue(jlong value,char*buf);
	static char* strdup(const char* data);
    static char* replaceChar(const char* data,char oldChar,char newChar);
	static long  getFileSize(char* fileFullPath);
	static bool  renameFile(char* oldName,char* newName);
	static void  mySleep(int seconds);
	static bool  containInvalidFileNameChar(char* name);
	static void  convertInvalidFileNameChar(char* name);
    static char* utf8_to_native(const char *pUtf8Str);
	static bool  isBigEndian();
	static bool  hasEnoughFreeDiskSpace();
	static bool  removeFile(const char* fileName);
	static unsigned long  getCurrentThreadId();

	//add by Qiu Song on 20090819 for Excat Version 3.0
	//スレッド状態(int→文字列)を取得する
	static char* getThreadStatusString(jint threadStatus);
    //スレッド待ち理由を取得する
	static char* getThreadWaitingReason(jint threadStatus);
	//カレントスレッドは指定されたタイプかどうか判断する
	static bool shouldDumpCurrentThread(jvmtiEnv* jvmti, jthread curThread, char* threadStatus, int threadPriority);
	
	//カレントスレッドの状態は指定されたと一致するかどうか
	static bool isCurrentThreadStatusMatch(jvmtiEnv* jvmti, jthread curThread, char* threadStatus);

	//カレントスレッドの優先度は指定されたと一致するかどうか
	static bool isCurrentThreadPriorityMatch(jvmtiEnv* jvmti, jthread curThread, int threadPriority);

	//指定タイプのオブジェクトダンプ稼動か判断する
	static bool shouldDumpCurrentObject(jvmtiEnv* jvmti, JNIEnv *jni, jobject curObject);
	//クラスは指定されたダンプタイプかどうか判断する
	static bool isFilterObjType(char *javaClassName);
	//メソッドsigからリターンタイプを取得する
	static char* getReturnTypeFromMethodSig(const char *methodSig);
	//除外例外一覧ファイルのパスを取得する
	static char* getExcludeExceptionListFilePath();
	//「null」のstringを戻す
	static char* getNullString();
	//指定文字列に「*」があるかどうか
	static bool hasWildCode(const char* stringBuf);
	//文字列が指定されたのとマッチかどうか判断する
	static bool doesStringMatch(const char* curMethodName, const char* monitorMethodName);
	//文字列が指定されたのとマッチかどうか判断する(wild codeを含める)
	static bool doesStringMatchWithWildCode(const char* curMethodName, const char* monitorMethodName);
	//wild code前と後の文字列を取得する
	static void getStringBeforeAndAfterWildCode(const char* stringBuf, char* strBeforeWildCode, char* strAfterWildCode);
	//指定位置の前と後の文字列を取得する
	static void getStringBeforeAndAfterPosition(const char* stringBuf, int nPos, char* stringBufLeft, char* stringBufRight);
	//文字列が指定されたのとマッチかどうか判断する(先頭文字はwild codeの場合)
	static bool doesStringMatchWithFirstWildCode(const char* stringBuf, const char* matchString, char* stringBufRight);
	//文字列が指定されたのとマッチかどうか判断する(先頭文字はwild code以外の場合)
	static bool doesStringMatchWithFirstNotWildCode(const char* stringBuf, const char* matchString,
													char* strBeforeWildCode, char* strAfterWildCode,
													char* stringBufLeft, char* stringBufRight);
	//wild code指定は有効かどうか
	static bool isWildCodeCorrect(const char*);

	//booleanの戻り値はtrueまたはfalseに変更
	static void convertBoolValue(char* type, char* value);

	//charの戻り値を表示できるように変更
	static void convertCharValue(char* type, char* value);
	
	//jobjectのクラス名を取得する
	static char* getClassNameFromJobject(jvmtiEnv* jvmti, JNIEnv *jni, jobject curObject);

	//メソッドのクラス名を取得する
	static char* getMethodDeclaringClassName(jvmtiEnv* jvmti, JNIEnv *jni, jmethodID methodId);

	//クラス名からパッケージ名を取得する
	static char* getPackageNameFromClassName(char* className);

	//OSのMACアドレスのチェック関数
	static bool CheckMacAddress(char* mac);

	//親フォルダ名を取得する
	static char* getPreForldName(char* strPathName);

	//指定文字列に、最後の指定文字を探す
	static int findLastCharOf(char* strPathName, char strChar);

	//フォルダを削除する関数
    static int removeDir(char* dirPath);

	//指定フォルダまで、空くフォルダを削除する関数
    static int removeDirUntil(char* dirPathStart, char* dirPathEnd);

	//ダンプファイルを削除する
    static void removeDumpFile(char* filePath);

#ifdef _WINDOWS
	//WindowsシステムのMACアドレスのチェック関数
	static bool CheckMacAddressForWindows(char* mac);
#endif

#ifdef _LINUX
	//LinuxシステムのMACアドレスを取得する関数
	static bool CheckMacAddressForLinux(char* mac);
#endif
	//end of add by Qiu Song on 20090819 for Excat Version 3.0

//add by Qiu Song on 20091120 for SolarisとHP-UXのMacアドレスを取得する関数
#ifdef _EXCAT_SOLARIS
	//LinuxシステムのMACアドレスを取得する関数
	static bool CheckMacAddressForSolaris(char* mac);
#endif

#ifdef _EXCAT_HPUX
	//LinuxシステムのMACアドレスを取得する関数
	static bool CheckMacAddressForHPUX(char* mac);
#endif
//end of //add by Qiu Song on 20091120 for SolarisとHP-UXのMacアドレスを取得する関数
private:
	static int utf8_to_utf16be_sub(unsigned short *pUcs2, const char *pUtf8,int nUtf8Num, 
									bool bCountOnly, bool bBigEndian);
    static unsigned short* utf8_to_utf16be(const char *pUtf8Str, int *nNumOut, bool bBigEndian);
};
END_NS

#endif  //_HELPERFUNC_H
