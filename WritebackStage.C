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
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"
#include "WritebackStage.h"


/*
 * doClockLow:
 * Performs the Fetch stage combinational logic that is performed when
 * the clock edge is low.
 *
 * @param: pregs - array of the pipeline register sets (F, D, E, M, W instances)
 * @param: stages - array of stages (FetchStage, DecodeStage, ExecuteStage,
 *         MemoryStage, WritebackStage instances)
 */
bool WritebackStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
    W* wreg = (W*)pregs[WREG];
    uint64_t icode = wreg->geticode()->getOutput();
    uint64_t stat = wreg->getstat()->getOutput();
    if(stat != SAOK || icode == IHALT)
    {
        return 1;
    }
    int64_t W_valE = wreg->getvalE()->getOutput();
    int64_t W_valM = wreg->getvalM()->getOutput();
    uint8_t dstM = wreg->getdstM()->getOutput();
    uint8_t dst_E = wreg->getdstE()->getOutput();

    RegisterFile * regField = RegisterFile::getInstance();
    bool error = false;
    regField->writeRegister(W_valE, dst_E, error);
    error = false;
    regField->writeRegister(W_valM, dstM, error); 
 
    return 0;
}

/* doClockHigh
 * applies the appropriate control signal to the F
 * and D register intances
 *
 * @param: pregs - array of the pipeline register (F, D, E, M, W instances)
 */
void WritebackStage::doClockHigh(PipeReg ** pregs)
{
   return;
}

