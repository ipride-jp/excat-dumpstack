# Microsoft Developer Studio Project File - Name="DumpStack" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** 編集しないでください **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=DumpStack - Win32 Debug
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "DumpStack_IBM.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "DumpStack_IBM.mak" CFG="DumpStack - Win32 Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "DumpStack - Win32 Release" ("Win32 (x86) Dynamic-Link Library" 用)
!MESSAGE "DumpStack - Win32 Debug" ("Win32 (x86) Dynamic-Link Library" 用)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/障害解析/03開発/DumpStack", ZOEAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "DumpStack - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DUMPSTACK_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I ".\include" /I "D:\jdk1.5.0_06\include" /I "D:\jdk1.5.0_06\include\win32" /I "..\license\\" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DUMPSTACK_EXPORTS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib xerces-c_2.lib libLicense.lib /nologo /dll /machine:I386 /out:"c:\ccat\DumpStack\DumpStack.dll" /libpath:".\lib" /libpath:"..\license\vc6\release"

!ELSEIF  "$(CFG)" == "DumpStack - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DUMPSTACK_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /I ".\include" /I ".\incluce\license\\" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DUMPSTACK_EXPORTS" /D "_DUMP_DEBUG" /D "VC6" /FR /YX /FD /I /I /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib xerces-c_2.lib libLicense.lib /nologo /dll /debug /machine:I386 /out:"c:\ccat\DumpStack\DumpStack.dll" /pdbtype:sept /libpath:".\lib" /libpath:"..\license\vc6\debug"
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ENDIF 

# Begin Target

# Name "DumpStack - Win32 Release"
# Name "DumpStack - Win32 Debug"
# Begin Group "classfileoperation"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\classfileoperation\AttributeInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\src\classfileoperation\AttributeInfo.h
# End Source File
# Begin Source File

SOURCE=.\src\classfileoperation\BaseInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\src\classfileoperation\BaseInfo.h
# End Source File
# Begin Source File

SOURCE=.\src\classfileoperation\ClassFile.cpp
# End Source File
# Begin Source File

SOURCE=.\src\classfileoperation\ClassFile.h
# End Source File
# Begin Source File

SOURCE=.\src\classfileoperation\ClassFileConstant.h
# End Source File
# Begin Source File

SOURCE=.\src\classfileoperation\CpInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\src\classfileoperation\CpInfo.h
# End Source File
# Begin Source File

SOURCE=.\src\classfileoperation\ExceptionInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\src\classfileoperation\ExceptionInfo.h
# End Source File
# Begin Source File

SOURCE=.\src\classfileoperation\FieldInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\src\classfileoperation\FieldInfo.h
# End Source File
# Begin Source File

SOURCE=.\src\classfileoperation\MethodInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\src\classfileoperation\MethodInfo.h
# End Source File
# End Group
# Begin Group "common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\common\Config.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\Config.h
# End Source File
# Begin Source File

SOURCE=.\src\common\Define.h
# End Source File
# Begin Source File

SOURCE=.\src\common\DumpObject.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\DumpObject.h
# End Source File
# Begin Source File

SOURCE=.\src\common\ExcludeClass.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\ExcludeClass.h
# End Source File
# Begin Source File

SOURCE=.\src\common\Global.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\Global.h
# End Source File
# Begin Source File

SOURCE=.\src\common\HelperFunc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\HelperFunc.h
# End Source File
# Begin Source File

SOURCE=.\src\common\JniLocalRefAutoRelease.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\JniLocalRefAutoRelease.h
# End Source File
# Begin Source File

SOURCE=.\src\common\JvmtiAutoRelease.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\JvmtiAutoRelease.h
# End Source File
# Begin Source File

SOURCE=.\src\common\JvmUtilFunc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\JvmUtilFunc.h
# End Source File
# Begin Source File

SOURCE=.\src\common\MessageFile.h
# End Source File
# Begin Source File

SOURCE=.\src\common\MonitoringClass.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\MonitoringClass.h
# End Source File
# Begin Source File

SOURCE=.\src\common\MonitoringInstance.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\MonitoringInstance.h
# End Source File
# Begin Source File

SOURCE=.\src\common\MonitoringMethod.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\MonitoringMethod.h
# End Source File
# Begin Source File

SOURCE=.\src\common\MonitoringTarget.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\MonitoringTarget.h
# End Source File
# Begin Source File

SOURCE=.\src\common\ObjectAutoRelease.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\ObjectAutoRelease.h
# End Source File
# Begin Source File

SOURCE=.\src\common\OutputSetting.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\OutputSetting.h
# End Source File
# Begin Source File

SOURCE=.\src\common\SimpleStackTrace.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\SimpleStackTrace.h
# End Source File
# End Group
# Begin Group "jniutility"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\jniutility\AgentCallbackHandler.cpp
# End Source File
# Begin Source File

SOURCE=.\src\jniutility\AgentCallbackHandler.h
# End Source File
# Begin Source File

SOURCE=.\src\jniutility\AgentRoutine.cpp
# End Source File
# Begin Source File

SOURCE=.\src\jniutility\AgentRoutine.h
# End Source File
# Begin Source File

SOURCE=.\src\jniutility\ExceptionNameMan.cpp
# End Source File
# Begin Source File

SOURCE=.\src\jniutility\ExceptionNameMan.h
# End Source File
# Begin Source File

SOURCE=.\src\jniutility\ExceptionTableMan.cpp
# End Source File
# Begin Source File

SOURCE=.\src\jniutility\ExceptionTableMan.h
# End Source File
# Begin Source File

SOURCE=.\src\jniutility\ExceptionTrace.cpp
# End Source File
# Begin Source File

SOURCE=.\src\jniutility\ExceptionTrace.h
# End Source File
# Begin Source File

SOURCE=.\src\jniutility\Monitor.cpp
# End Source File
# Begin Source File

SOURCE=.\src\jniutility\Monitor.h
# End Source File
# Begin Source File

SOURCE=.\src\jniutility\PE_Debug.cpp
# End Source File
# Begin Source File

SOURCE=.\src\jniutility\PE_Debug.h
# End Source File
# Begin Source File

SOURCE=.\src\jniutility\SignalHandlerRegister.cpp
# End Source File
# Begin Source File

SOURCE=.\src\jniutility\SignalHandlerRegister.h
# End Source File
# End Group
# Begin Group "output"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\output\Attribute.cpp
# End Source File
# Begin Source File

SOURCE=.\src\output\Attribute.h
# End Source File
# Begin Source File

SOURCE=.\src\output\Instance.cpp
# End Source File
# Begin Source File

SOURCE=.\src\output\Instance.h
# End Source File
# Begin Source File

SOURCE=.\src\output\Method.cpp
# End Source File
# Begin Source File

SOURCE=.\src\output\Method.h
# End Source File
# Begin Source File

SOURCE=.\src\output\ObjectData.cpp
# End Source File
# Begin Source File

SOURCE=.\src\output\ObjectData.h
# End Source File
# Begin Source File

SOURCE=.\src\output\ObjectPool.cpp
# End Source File
# Begin Source File

SOURCE=.\src\output\ObjectPool.h
# End Source File
# Begin Source File

SOURCE=.\src\output\StackTrace.cpp
# End Source File
# Begin Source File

SOURCE=.\src\output\StackTrace.h
# End Source File
# Begin Source File

SOURCE=.\src\output\SuperClass.cpp
# End Source File
# Begin Source File

SOURCE=.\src\output\SuperClass.h
# End Source File
# Begin Source File

SOURCE=.\src\output\TypedElement.cpp
# End Source File
# Begin Source File

SOURCE=.\src\output\TypedElement.h
# End Source File
# End Group
# Begin Group "antlrparse"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\antlrparse\CcatFormatException.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrparse\CcatFormatException.hpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrparse\CcatLexer.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrparse\CcatLexer.hpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrparse\CcatParser.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrparse\CcatParser.hpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrparse\CcatParserTokenTypes.hpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrparse\CcatTypeValue.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrparse\CcatTypeValue.hpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrparse\MismatchedUnicodeCharException.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrparse\MismatchedUnicodeCharException.hpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrparse\UnicodeCharBuffer.hpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrparse\UnicodeCharScanner.hpp
# End Source File
# End Group
# Begin Group "antlrlib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\antlrlib\ANTLRUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\ASTFactory.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\ASTNULLType.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\ASTRefCount.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\BaseAST.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\BitSet.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\CharBuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\CharScanner.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\CommonAST.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\CommonASTWithHiddenTokens.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\CommonHiddenStreamToken.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\CommonToken.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\InputBuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\LLkParser.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\MismatchedCharException.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\MismatchedTokenException.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\NoViableAltException.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\NoViableAltForCharException.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\Parser.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\RecognitionException.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\String.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\Token.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\TokenBuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\TokenRefCount.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\TokenStreamBasicFilter.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\TokenStreamHiddenTokenFilter.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\TokenStreamRewriteEngine.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\TokenStreamSelector.cpp
# End Source File
# Begin Source File

SOURCE=.\src\antlrlib\TreeParser.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\common\Logger.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\Logger.h
# End Source File
# Begin Source File

SOURCE=.\src\classfileoperation\OpCodeScan.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\ReadCfgFileException.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\ReadCfgFileException.h
# End Source File
# Begin Source File

SOURCE=.\readme.txt
# End Source File
# Begin Source File

SOURCE=.\resource_IBM.h
# End Source File
# Begin Source File

SOURCE=.\resource_IBM.rc
# End Source File
# Begin Source File

SOURCE=.\src\common\SystemInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\SystemInfo.h
# End Source File
# End Target
# End Project
