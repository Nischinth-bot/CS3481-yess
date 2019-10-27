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
    
    uint64_t stat = SAOK, icode = 0, valE = 0, dstM = RNONE, dstE = RNONE;
    icode = mreg->geticode()->getOutput();
    dstE = mreg->getdstE()->getOutput();
    dstM = mreg->getdstM()->getOutput();
    valE = mreg->getvalE()->getOutput();

    setWinput(wreg, stat, icode, valE, 0, dstE, dstM);
    return 0;
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
