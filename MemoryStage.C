#include <string>
#include <cstdint>
#include "RegisterFile.h"
#include "PipeRegField.h"
#include "PipeReg.h"
#include "F.h"
#include "E.h"
#include "D.h"
#include "M.h"
#include "W.h"
#include "Stage.h"
#include "MemoryStage.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"
#include "Memory.h"
/*
 * doClockLow:
 * Performs the Fetch stage combinational logic that is performed when
 * the clock edge is low.
 *
 * @param: pregs - array of the pipeline register sets (F, D, E, M, W instances)
 * @param: stages - array of stages (FetchStage, DecodeStage, ExecuteStage,
 *         MemoryStage, WritebackStage instances)
 */


bool MemoryStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
    M * mreg = (M *) pregs[MREG];
    W * wreg = (W *) pregs[WREG];

    valM = 0;
    uint64_t stat = SAOK, icode = 0, valE = 0, valA = 0, dstM = RNONE, dstE = RNONE;
    icode = mreg->geticode()->getOutput();
    dstE = mreg->getdstE()->getOutput();
    dstM = mreg->getdstM()->getOutput();
    valE = mreg->getvalE()->getOutput();
    valA = mreg->getvalA()->getOutput();

    Memory * mem = Memory::getInstance();
    uint32_t addr  = mem_addr(icode, valE, valA);
    bool error = false;
    if(mem_read(icode))
    {
        valM = mem->getLong(addr, error);
    }
    if(mem_write(icode))
    {
       mem->putLong(valA, addr,  error);
    }
    setWinput(wreg, stat, icode, valE, valM, dstE, dstM);
    return 0;
}

/**
 * Simulator for mem_addr.
 * @param: M_icode: The icode of the instruction in the memory stage.
 * @param: M_valE: The valE in the Memory register.
 * @param: M_valA: The valA in the Memory register.
 * @return: Appropriate address word.
 */
uint64_t MemoryStage::mem_addr(uint8_t M_icode, int64_t M_valE, int64_t M_valA)
{
    if(M_icode == IRMMOVQ || M_icode == IPUSHQ || M_icode == ICALL || M_icode == IMRMOVQ) return M_valE;
    if(M_icode == IPOPQ || M_icode == IRET) return M_valA;
    return 0;
}


/**
 * Simulator for mem_read connection.
 * @param: M_icode : The icode of the instruction in the memory stage.
 * @return: True if M_icode is an MRMOVQ, POPQ or RET.
 */
bool MemoryStage::mem_read(uint8_t M_icode)
{
    return (M_icode == IMRMOVQ || M_icode == IPOPQ || M_icode == IRET);
}


bool MemoryStage::mem_write(uint8_t M_icode)
{   
    return (M_icode == IRMMOVQ || M_icode == IPUSHQ || M_icode == ICALL);
}

/**
 * @return: m_valM
 */
int64_t MemoryStage::getm_valM()
{
    return valM;
}

/* doClockHigh
 * applies the appropriate control signal to the F
 * and D register intances
 *
 * @param: pregs - array of the pipeline register (F, D, E, M, W instances)
 */
void MemoryStage::doClockHigh(PipeReg ** pregs)
{
    M * mreg = (M *) pregs[MREG];
    W * wreg = (W*) pregs[WREG];

    wreg->getstat()->normal();
    wreg->geticode()->normal();
    wreg->getvalE()->normal();
    wreg-> getvalM() -> normal();
    wreg-> getdstE() -> normal();
    wreg-> getdstM() -> normal();

    mreg->getstat()->normal();
    mreg->geticode()->normal();
    mreg->getCnd()->normal();
    mreg->getvalE()->normal();
    mreg->getvalA()->normal();
    mreg->getdstE()->normal();
    mreg->getdstM()->normal(); 

}

void MemoryStage::setWinput(W* wreg, uint64_t stat, uint64_t icode, uint64_t valE, uint64_t valM, uint64_t dstE, uint64_t dstM)
{
    wreg->getstat()->setInput(stat);
    wreg->geticode()->setInput(icode);
    wreg->getvalE()->setInput(valE);
    wreg->getvalM()->setInput(valM);
    wreg->getdstE()->setInput(dstE);
    wreg->getdstM()->setInput(dstM);
}
