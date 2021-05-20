
#include <stdio.h>
#include "pin.H"
#include <assert.h>
#define  ull unsigned long long 

FILE * trace;
PIN_LOCK pinLock;
static ull MemoryCount=0;
static ull globalcount=0;

VOID ThreadStart(THREADID tid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    PIN_GetLock(&pinLock, tid+1);
    printf("threads begin %d\n",tid);
    PIN_ReleaseLock(&pinLock);
}

VOID ThreadFini(THREADID tid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
    PIN_GetLock(&pinLock, tid+1);
    printf("threads end  %d\n",tid);
    PIN_ReleaseLock(&pinLock);
}

VOID RecordMemAccess(THREADID tid,VOID *addr,UINT32 Ref_Size,UINT32 Type_request)
{
	ull Memory_pointer=(ull)addr;
	ull thread_id=(ull)tid;
	ull ReferenceSize=(ull)Ref_Size;
	ull Memory_bound=((Memory_pointer/64)+1)*64;
	ull Request_Type=(ull)Type_request;
	while(ReferenceSize>0)
	{
		fwrite(&globalcount,sizeof(ull),1,trace);
		fwrite(&thread_id,sizeof(ull),1,trace);
		fwrite(&Request_Type,sizeof(ull),1,trace);
		fwrite(&Memory_pointer,sizeof(ull),1,trace);
		fflush(trace);
		globalcount++;
		if(ReferenceSize>=8 && Memory_bound>=(Memory_pointer+8))
		{
			Memory_pointer+=8;
			ReferenceSize-=8;
		}
		else
		{
			if(ReferenceSize>=4 && Memory_bound>=(Memory_pointer+4))
			{
				Memory_pointer+=4;
				ReferenceSize-=4;
			}
			else
			{
				if(ReferenceSize>=2 && Memory_bound>=(Memory_pointer+2))
				{
					Memory_pointer+=2;
					ReferenceSize-=2;
				}
				else
				{
					if(ReferenceSize>=1 && Memory_bound>=(Memory_pointer+1))
					{
						Memory_pointer+=1;
						ReferenceSize-=1;
					}
				}
			}
		}
		MemoryCount++;
		Memory_bound=((Memory_pointer/64)+1)*64;
	}
}

VOID RecordMemRead(THREADID tid,VOID * addr,UINT32 ReferenceSize)
{
	PIN_GetLock(&pinLock, tid+1);
	RecordMemAccess(tid,addr,ReferenceSize,0);
	PIN_ReleaseLock(&pinLock);
}
VOID RecordMemWrite(THREADID tid,VOID * addr,UINT32 ReferenceSize)
{
	PIN_GetLock(&pinLock, tid+1);
	RecordMemAccess(tid,addr,ReferenceSize,1);
	PIN_ReleaseLock(&pinLock);
}

VOID Instruction(INS ins, VOID *v)
{
    UINT32 memOperands = INS_MemoryOperandCount(ins);
    USIZE ReferenceSize;
    
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
        if (INS_MemoryOperandIsRead(ins, memOp))
        {
        	ReferenceSize=INS_MemoryOperandSize(ins,memOp);
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
                IARG_THREAD_ID,
                IARG_MEMORYOP_EA, memOp,
                IARG_UINT32,ReferenceSize,
                IARG_END);
        }
        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
        	ReferenceSize=INS_MemoryOperandSize(ins,memOp);
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
                IARG_THREAD_ID,
                IARG_MEMORYOP_EA, memOp,
                IARG_UINT32,ReferenceSize,
                IARG_END);
        }
    }
}
VOID Fini(INT32 code, VOID *v)
{
    printf("Total Machine Access Count : %llu\n",MemoryCount);
    fclose(trace);
}

int main(int argc, char * argv[])
{   
    trace = fopen("trace.out","wb");
    
    PIN_InitLock(&pinLock);

    // Initialize pin
    if (PIN_Init(argc, argv)) 
    {
	printf("Error \n");
    }

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);

    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);
    
    PIN_AddFiniFunction(Fini, 0);
    // Start the program, never returns
    PIN_StartProgram();
    return 0;
}