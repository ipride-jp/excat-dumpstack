#include <string.h>
#include "OpCodeScan.h"
#include "ClassFileConstant.h"

#include "../common/Global.h"


#define NEXT_4BYTE_BOUNDARY(opcode_pos) (((opcode_pos)+4) & (~3))

USE_NS(NS_CLASS_FILE_OPERATION)
USE_NS(NS_COMMON)

OpCodeScan::OpCodeScan(jint bytecode_count_param, unsigned char* bytecodes_ptr_param)
{
	bytecode_count = bytecode_count_param;
    bytecodes_ptr = bytecodes_ptr_param;
}

OpCodeScan::~OpCodeScan()
{
}

//argSlotNum include this
bool OpCodeScan::setType(char* varType,int index,char type,int localVarCount,int argSlotNum)
{
	if(index > localVarCount -1)
	{
		return false;
	}
	//we have known the type of argument
	if(index > argSlotNum - 1) //argSlotNum include this
	    varType[index] = type;
	return true;
}

unsigned OpCodeScan::getCodeAtLocation(jint location)
{
    if(location <0 || location > bytecode_count - 1)
		return 0;

	unsigned char* pos = bytecodes_ptr;
	pos += location;
	return readU1(pos);
}

/**
Jrockit(1.4.2_12,1.5.0_11以前)では、以下のバイトコードの場合
　　36 invokevirtual #9 <AClass.first>
　　39 istore 10
　　41 iload 10
slot10の変数(int,double)の値を取得できません(JVMが落ちる可能性がある)。
float,long,参照型の場合、当現象が発生しません。
*/
int  OpCodeScan::chkInvalidSlotForJrockit(jint location)
{
	unsigned currentByte = getCodeAtLocation(location);
	int invalidSlot = -1;
	if(currentByte >=opc_invokevirtual  && currentByte<=opc_invokeinterface)
	{
		int currentLen = opcode_length(currentByte);
		if(currentLen <= 0){
			return invalidSlot;
		}
        //get next byte code
		unsigned char* pos = bytecodes_ptr;
	    pos += location + currentLen;
        if(location > bytecode_count - 1)
		    return invalidSlot;
        unsigned nextByteCode = readU1(pos);
		
        switch(nextByteCode)
		{
		case opc_istore_0:
		case opc_astore_0:
        case opc_fstore_0:
        case opc_lstore_0:
        case opc_dstore_0:
			invalidSlot = 0;
			break;
		case opc_istore_1:
        case opc_astore_1:
        case opc_fstore_1:
		case opc_lstore_1:
        case opc_dstore_1:
			invalidSlot = 1;
			break;
		case opc_istore_2:
        case opc_dstore_2:
        case opc_fstore_2:
		case opc_lstore_2:
        case opc_astore_2:
			invalidSlot = 2;
			break;
		case opc_istore_3:
        case opc_dstore_3:
        case opc_fstore_3:
        case opc_astore_3:
		case opc_lstore_3:
			invalidSlot = 3;
			break;
        case opc_istore:
		case opc_dstore:
        case opc_astore:
        case opc_lstore:
        case opc_fstore:
			invalidSlot = readU1(pos +1 );
			break;
        default:
            invalidSlot = -1;
		}
	}//of if(currentByte >=opc_invokevirtual ...

	return invalidSlot;
}

//argSlotNum include this
void OpCodeScan::getVarTypes(jint count,  char* varTypes,int posCurrent,int argSlotNum)
{
	if(count == 0)
	{
		return;
	}

	char* temp = varTypes;

	unsigned char* pos = bytecodes_ptr;
	unsigned opcode;
    int lvIndex; //local variable index
	int instr_len;
	int pos_index =0;
	int header;
	int low;
	int high;
	int npairs;
	while(pos < bytecodes_ptr + bytecode_count && (pos - bytecodes_ptr)<= posCurrent)
	{
		opcode = readU1(pos++);
		lvIndex = -1;
		//wide operator
        if(opcode == opc_wide )
		{
			opcode = readU1(pos++);  //get real operator
	        lvIndex = readU1(pos++);  //get local variable index
			//skip the next two bytes
			pos += 2;
			if(opcode == opc_iinc)
			{  //if iinc skip the next two bytes
			   pos += 2;
			}
		}else
		{
			 /* Process this opcode */
			switch (opcode) {
				case opc_tableswitch:
					pos_index = pos - bytecodes_ptr - 1;
					header = NEXT_4BYTE_BOUNDARY(pos_index);
					pos += header - (pos_index+1);
					//skip 4 bytes
					pos +=4;
					low = readU4(pos);
					pos +=4;
					high = readU4(pos);
					pos +=4;
					pos += (high+1-low) * 4;
					break;
				case opc_lookupswitch:
					pos_index = pos - bytecodes_ptr - 1;
					header = NEXT_4BYTE_BOUNDARY(pos_index);
					pos += header - (pos_index+1);
					pos +=4;
					npairs = readU4(pos);
					pos += 4;
					pos +=  npairs * 8;
					break;
				default:
					instr_len = opcode_length(opcode);
					if(instr_len <= 0)
					{
						char logBuf[MAX_BUF_LEN];
						sprintf(logBuf,"Can't analyzer op code at pos %d.",pos - bytecodes_ptr -1);
						LOG4CXX_DEBUG(Global::logger, logBuf);
						return;
					}
					pos += instr_len-1;
					break;
			}
		}
        
		switch(opcode)
		{
		//int
        case opc_iload_0:
		case opc_istore_0:
			if(!setType(temp,0,'I',count,argSlotNum))
			{
				return;
			}
			break;
        case opc_iload_1:
        case opc_istore_1:
			if(!setType(temp,1,'I',count,argSlotNum))
			{
				return;
			}
			break;
        case opc_iload_2:
        case opc_istore_2:
			if(!setType(temp,2,'I',count,argSlotNum))
			{
				return;
			}
			break;
        case opc_iload_3:
        case opc_istore_3:
			if(!setType(temp,3,'I',count,argSlotNum))
			{
				return;
			}
			break;
        case opc_iload:
		case opc_istore:
			if(lvIndex == -1)
			{
				lvIndex  = readU1(pos -1 );
			}
			if(!setType(temp,lvIndex,'I',count,argSlotNum))
			{
				return;
			}
			break;
        //long:J
        case opc_lload_0:
		case opc_lstore_0:
			if(!setType(temp,0,'J',count,argSlotNum))
			{
				return;
			}
			break;
        case opc_lload_1:
        case opc_lstore_1:
			if(!setType(temp,1,'J',count,argSlotNum))
			{
				return;
			}
			break;
        case opc_lload_2:
        case opc_lstore_2:
			if(!setType(temp,2,'J',count,argSlotNum))
			{
				return;
			}
			break;
        case opc_lload_3:
        case opc_lstore_3:
			if(!setType(temp,3,'J',count,argSlotNum))
			{
				return;
			}
			temp[3] = 'J';
			break;
        case opc_lload:
		case opc_lstore:
			if(lvIndex == -1)
			{
				lvIndex  = readU1(pos -1 );
			}
			if(!setType(temp,lvIndex,'J',count,argSlotNum))
			{
				return;
			}
			break;
       //float:F
        case opc_fload_0:
		case opc_fstore_0:
			if(!setType(temp,0,'F',count,argSlotNum))
			{
				return;
			}
			break;
        case opc_fload_1:
        case opc_fstore_1:
			if(!setType(temp,1,'F',count,argSlotNum))
			{
				return;
			}
			break;
        case opc_fload_2:
        case opc_fstore_2:
			if(!setType(temp,2,'F',count,argSlotNum))
			{
				return;
			}
			break;
        case opc_fload_3:
        case opc_fstore_3:
			if(!setType(temp,3,'F',count,argSlotNum))
			{
				return;
			}
			break;
        case opc_fload:
		case opc_fstore:
			if(lvIndex == -1)
			{
				lvIndex  = readU1(pos -1 );
			}
			if(!setType(temp,lvIndex,'F',count,argSlotNum))
			{
				return;
			}
  		   break;
      //double:D
        case opc_dload_0:
		case opc_dstore_0:
			if(!setType(temp,0,'D',count,argSlotNum))
			{
				return;
			}
			break;
        case opc_dload_1:
        case opc_dstore_1:
			if(!setType(temp,1,'D',count,argSlotNum))
			{
				return;
			}
			break;
        case opc_dload_2:
        case opc_dstore_2:
			if(!setType(temp,2,'D',count,argSlotNum))
			{
				return;
			}
			break;
        case opc_dload_3:
        case opc_dstore_3:
			if(!setType(temp,3,'D',count,argSlotNum))
			{
				return;
			}
			break;
        case opc_dload:
		case opc_dstore:
			if(lvIndex == -1)
			{
				lvIndex  = readU1(pos -1 );
			}
			if(!setType(temp,lvIndex,'D',count,argSlotNum))
			{
				return;
			}
    		break;
      //object:L
        case opc_aload_0:
		case opc_astore_0:
			if(!setType(temp,0,'L',count,argSlotNum))
			{
				return;
			}
			break;
        case opc_aload_1:
        case opc_astore_1:
			if(!setType(temp,1,'L',count,argSlotNum))
			{
				return;
			}
			break;
        case opc_aload_2:
        case opc_astore_2:
			if(!setType(temp,2,'L',count,argSlotNum))
			{
				return;
			}
			break;
        case opc_aload_3:
        case opc_astore_3:
			if(!setType(temp,3,'L',count,argSlotNum))
			{
				return;
			}
			break;
        case opc_aload:
		case opc_astore:
			if(lvIndex == -1)
			{
				lvIndex  = readU1(pos -1 );
			}
			if(!setType(temp,lvIndex,'L',count,argSlotNum))
			{
				return;
			}
			break;
		default:
			break;
		}

    }
}

int OpCodeScan::opcode_length(unsigned opcode)
{
    /* Define array that holds length of an opcode */
    static unsigned char _opcode_length[opc_MAX+1] = 
			  JVM_OPCODE_LENGTH_INITIALIZER;
    if(opcode >  opc_MAX || _opcode_length[opcode]==0)
	{
		char logBuf[MAX_BUF_LEN];
		sprintf(logBuf,"unknown opcode:%d",opcode);
		LOG4CXX_DEBUG(Global::logger, logBuf);
		return -1;
	}
	else
	{
		return _opcode_length[opcode];
	}
}

unsigned OpCodeScan::readU1(unsigned char* pos) 
{
    return ((unsigned)(*pos)) & 0xFF;
}

unsigned OpCodeScan::readU2(unsigned char* pos) 
{
    unsigned res;
    
    res = readU1(pos);
    return (res << 8) + readU1(pos + 1);
}

signed short OpCodeScan::readS2(unsigned char* pos) 
{
    unsigned res;
    
    res = readU1(pos);
    return ((res << 8) + readU1(pos + 1)) & 0xFFFF;
}

unsigned OpCodeScan::readU4(unsigned char* pos) 
{
    unsigned res;
    
    res = readU2(pos);
    return (res <<16) + readU2(pos + 2);
}

//add by Qiu Song on 20090817 for メソッド監視
//挿入したcallbacksメソッドの直近のopCodeを取得する
bool OpCodeScan::getLastOpCodeBeforeCallbacks(jint location, unsigned& opCode, jint& opCodePos)
{
	//if not the callbacks func before return code
	if(opc_invokestatic != getCodeAtLocation(location) || location <= 2)
	{
		return false;
	}

	return getLastNOpCodeBeforeLocation(2, location, opCode, opCodePos, true);
}

//指定位置の前の直近のopコードを取得する
bool OpCodeScan::getLastNOpCodeBeforeLocation(int nNo, jint location, unsigned& opCode, jint& opCodePos, bool bReturnMode)
{
	unsigned char* pos = bytecodes_ptr;
	unsigned opcode = 0;
    int lvIndex; //local variable index
	int instr_len;
	int pos_index =0;
	int header;
	int low;
	int high;
	int npairs;

	unsigned preOpcode[MAX_BYTECODE_SIZE];
	unsigned preOpCodePos[MAX_BYTECODE_SIZE];
	int nLastPos = 0;
	while(pos < bytecodes_ptr + bytecode_count && (pos - bytecodes_ptr) < location )
	{
		//preOpcode = opcode;//l2dなどの対応
		//preOpCodePos = opCodePos;

		opCodePos = pos - bytecodes_ptr;
		opcode = readU1(pos++);

		preOpcode[nLastPos] = opcode;
		preOpCodePos[nLastPos] = opCodePos;
		nLastPos ++;

		lvIndex = -1;
		//wide operator
        if(opcode == opc_wide )
		{
			opcode = readU1(pos++);  //get real operator
	        lvIndex = readU1(pos++);  //get local variable index
			//skip the next two bytes
			pos += 2;
			if(opcode == opc_iinc)
			{  //if iinc skip the next two bytes
			   pos += 2;
			}
		}
		else
		{
			 /* Process this opcode */
			switch (opcode)
			{
				case opc_tableswitch:
					pos_index = pos - bytecodes_ptr - 1;
					header = NEXT_4BYTE_BOUNDARY(pos_index);
					pos += header - (pos_index+1);
					//skip 4 bytes
					pos +=4;
					low = readU4(pos);
					pos +=4;
					high = readU4(pos);
					pos +=4;
					pos += (high+1-low) * 4;
					break;
				case opc_lookupswitch:
					pos_index = pos - bytecodes_ptr - 1;
					header = NEXT_4BYTE_BOUNDARY(pos_index);
					pos += header - (pos_index+1);
					pos +=4;
					npairs = readU4(pos);
					pos += 4;
					pos +=  npairs * 8;
					break;
				default:
					instr_len = opcode_length(opcode);
					if(instr_len <= 0)
					{
						char logBuf[MAX_BUF_LEN];
						sprintf(logBuf,"Can't analyzer op code at pos %d.",pos - bytecodes_ptr -1);
						LOG4CXX_DEBUG(Global::logger, logBuf);
						return false;
					}
					pos += instr_len-1;
					break;
			}
		}
	}

	if(nLastPos >= nNo)
	{
	    opCode = preOpcode[nLastPos - nNo];
	    opCodePos = preOpCodePos[nLastPos - nNo];
		if(bReturnMode == true && (opCode == opc_i2l || opCode == opc_i2f || opCode == opc_i2d || 
		   opCode == opc_l2i || opCode == opc_l2f || opCode == opc_l2d ||
		   opCode == opc_f2i || opCode == opc_f2l || opCode == opc_f2d ||
		   opCode == opc_d2i || opCode == opc_d2l || opCode == opc_d2f ||
		   opCode == opc_i2b || opCode == opc_i2c || opCode == opc_i2s ||
		   opCode == opc_checkcast || opCode == checkcast_quick))
		{
			opCode = preOpcode[nLastPos - nNo -1];
			opCodePos = preOpCodePos[nLastPos - nNo -1];
		}
		return true;
	}
	return false;
}
//戻り値のタイプと位置を取得する
//return: 0 - 戻り値(bipushの場合) 
//        1 - Var Tableのslot index(xloadの場合)
//        2 - constant Poolの位置(ldcの場合)
//        3 - constant Poolの位置(getfieldの場合)
int OpCodeScan::getReturnValueTypeAndSlot(unsigned opCode, jint opCodePos, char& valueType, int& valueSlot)
{
	//戻り値のタイプを取得する
	getReturnValueType(opCode, valueType);

	//戻り値のVar Tableの位置を取得する
    int nNextOpcodePos;
	int nOpcodeType = getOpcodeTypeAndValueSlot(opCode, opCodePos, valueSlot, nNextOpcodePos);
	return nOpcodeType;
}

void OpCodeScan::getReturnValueType(unsigned opCode, char& valueType)
{
	switch(opCode)
	{
		case opc_iload_0:
		case opc_iload_1:
		case opc_iload_2:
		case opc_iload_3:
		case opc_iload:
		case opc_iconst_m1:
        case opc_iconst_0:
		case opc_iconst_1:
		case opc_iconst_2:
		case opc_iconst_3:
		case opc_iconst_4:
		case opc_iconst_5:
			valueType = 'I';
			break;
		case opc_aload_0:
		case opc_aload_1:
		case opc_aload_2:
		case opc_aload_3:
		case opc_aload:
		case opc_aconst_null:
			valueType = 'L';
			break;
		case opc_fload_0:
		case opc_fload_1:
		case opc_fload_2:
		case opc_fload_3:
		case opc_fload:
		case opc_fconst_0:
		case opc_fconst_1:
		case opc_fconst_2:
			valueType = 'F';
			break;
		case opc_lload_0:
		case opc_lload_1:
		case opc_lload_2:
		case opc_lload_3:
		case opc_lload:
		case opc_lconst_0:
		case opc_lconst_1:
			valueType = 'J';
			break;
		case opc_dload_0:
		case opc_dload_1:
		case opc_dload_2:
		case opc_dload_3:
		case opc_dload:
		case opc_dconst_0:
		case opc_dconst_1:
			valueType = 'D';
			break;
		default:
			valueType = 'I';
			break;
	}
}

int OpCodeScan::getOpcodeTypeAndValueSlot(unsigned opCode, jint opCodePos, int& valueSlot, int& nNextOpcodePos)
{
	unsigned char* pos = bytecodes_ptr;
	pos += (opCodePos +1);

	switch(opCode)
	{
		case opc_iload_0:
		case opc_aload_0:
		case opc_fload_0:
		case opc_lload_0:
		case opc_dload_0:
			valueSlot = 0;
			nNextOpcodePos = opCodePos +1;
			return VAR_RETURN;
		case opc_iload_1:
		case opc_aload_1:
		case opc_fload_1:
		case opc_lload_1:
		case opc_dload_1:
			valueSlot = 1;
			nNextOpcodePos = opCodePos +1;
			return VAR_RETURN;
		case opc_iload_2:
		case opc_aload_2:
		case opc_fload_2:
		case opc_lload_2:
		case opc_dload_2:
			valueSlot = 2;
			nNextOpcodePos = opCodePos +1;
			return VAR_RETURN;
		case opc_iload_3:
		case opc_aload_3:
		case opc_fload_3:
		case opc_lload_3:
		case opc_dload_3:
			valueSlot = 3;
			nNextOpcodePos = opCodePos +1;
			return VAR_RETURN;
		case opc_iload:
		case opc_dload:
		case opc_aload:
		case opc_lload:
		case opc_fload:
			valueSlot = readU1(pos);
			nNextOpcodePos = opCodePos +2;
			return VAR_RETURN;
		case opc_bipush:
			valueSlot = readU1(pos);
			nNextOpcodePos = opCodePos +2;
			return BIPUSH_RETURN;
		case opc_sipush:
			valueSlot = readU2(pos);
			nNextOpcodePos = opCodePos +3;
			return BIPUSH_RETURN;
		case opc_aconst_null:
		case opc_iconst_m1:
        case opc_iconst_0:
		case opc_iconst_1:
		case opc_iconst_2:
		case opc_iconst_3:
		case opc_iconst_4:
		case opc_iconst_5:
		case opc_lconst_0:
		case opc_lconst_1:
		case opc_fconst_0:
		case opc_fconst_1:
		case opc_fconst_2:
		case opc_dconst_0:
		case opc_dconst_1:
			valueSlot = -1;
			nNextOpcodePos = opCodePos +1;
			return CONST_RETURN;
		default: //need check here(TODO)
			valueSlot = readU2(pos);
			nNextOpcodePos = opCodePos +3;
			break;
	}
	nNextOpcodePos = -1;
	return -1;
}
//end of add by Qiu Song on 20090817 for メソッド監視