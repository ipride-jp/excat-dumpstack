#if !defined(_OPCODESCAN_H)
#define _OPCODESCAN_H

#include <jni.h>
#include "BaseInfo.h"

BEGIN_NS(NS_CLASS_FILE_OPERATION)


class OpCodeScan 
{
public:
	OpCodeScan(jint bytecode_count_param,  unsigned char* bytecodes_ptr_param);
	~OpCodeScan();
	//analyze opcode to get local var type and store it in varTypes
	//when there is no local variable,*varTypes will be null
    void getVarTypes(jint count,  char* varTypes,int posCurrent,int argSlotNum);
	unsigned getCodeAtLocation(jint location);
	int  chkInvalidSlotForJrockit(jint location);

	//add by Qiu Song on 20090817 for Method Monitor(insert callbacks before return)
	bool getLastOpCodeBeforeCallbacks(jint location, unsigned& opCode ,jint& opCodePos);
	bool getLastNOpCodeBeforeLocation(int nNo, jint location, unsigned& opCode, jint& opCodePos, bool bReturnMode = false);
	void getReturnValueType(unsigned opCode, char& valueType);
	int getReturnValueTypeAndSlot(unsigned opCode, jint opCodePos, char& valueType, int& valueSlot);
	int getOpcodeTypeAndValueSlot(unsigned opCode, jint opCodePos, int& valueSlot,  int& nNextOpcodePos);
	//end of add by Qiu Song on 20090817 for Method Monitor(insert callbacks before return)
private:
	unsigned readU1(unsigned char* pos);
	unsigned readU2(unsigned char* pos);
	signed short readS2(unsigned char* pos);
	unsigned readU4(unsigned char* pos);
	int opcode_length(unsigned opcode);
    bool setType(char* varType,int index,char type,int localVarCount,int argSlotNum);

private:
	//バイトコードの配列の長さ
	jint bytecode_count;
    //バイトコード配列へのポインタ
    unsigned char* bytecodes_ptr;

};
END_NS

#endif  //_OPCODESCAN_H
