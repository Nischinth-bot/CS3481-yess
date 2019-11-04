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
#include "ExecuteStage.h"
#include "DecodeStage.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"

/*
 * doClockLow:
 * Performs the Fetch stage combinational logic that is performed when
 * the clock edge is low.
 *
 * @param: pregs - array of the pipeline register sets (F, D, E, M, W instances)
 * @param: stages - array of stages (FetchStage, DecodeStage, ExecuteStage,
 *         MemoryStage, WritebackStage instances)
 */
bool DecodeStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
    D * dreg = (D *) pregs[DREG];
    W * wreg = (W *) pregs[WREG];
    M * mreg = (M *) pregs[MREG];
    E * ereg = (E *) pregs[EREG];

    ExecuteStage* e = (ExecuteStage*) stages[ESTAGE];

    uint64_t icode = 0, ifun = 0, valC = 0, valA = 0, valB = 0;
    uint64_t dstE = RNONE, dstM = RNONE, srcA = RNONE, srcB = RNONE, stat = SAOK; 

    icode = dreg-> geticode()-> getOutput();
    ifun = dreg-> getifun()-> getOutput();
    valC = dreg-> getvalC()-> getOutput(); 
    stat = dreg-> getstat() -> getOutput();

    dstE = dst_E(dreg, icode);
    dstM = dst_M(dreg, icode);
    srcA = d_srcA(dreg, icode);
    srcB = d_srcB(dreg, icode);

    valB = FwdB(dreg, wreg, mreg, srcB, e);
    valA = sel_FwdA(dreg, wreg, mreg, srcA, e);

    setEInput(ereg,stat, icode,ifun, valC, valA, valB, dstE, dstM, srcA, srcB);
    return false;
}



/* Simulation of HCL for d_srcA
 * @param: D* - A pointer to an instance of a Decode pipe register class.
 * @param: icode - The icode of the instruction passing through the pipeline.
 * @return: The appropriate d_srcA value given the icode.
 */
uint8_t DecodeStage::d_srcA(D * dreg, uint8_t icode)
{
    uint8_t D_rA_ret[4] = {IRRMOVQ, IRMMOVQ, IOPQ, IPUSHQ};
    uint8_t RSP_ret[2] = {IPOPQ, IRET};
    for(uint8_t code : D_rA_ret)
    {
        if(code == icode) return dreg->getrA()->getOutput();
    }
    for(uint8_t code : RSP_ret)
    {
        if(code == icode) return RSP;
    }
    return RNONE;
}

/* Simulation of HCL for d_srcB
 * @param: D* - A pointer to an instance of a Decode pipe register class.
 * @param: icode - The icode of the instruction passing through the pipeline.
 * @return: The appropriate d_srcB value given the icode.
 */
uint8_t DecodeStage::d_srcB(D * dreg, uint8_t icode)
{
    uint8_t D_rB_ret[4] = {IOPQ, IRMMOVQ, IMRMOVQ};
    uint8_t RSP_ret[4] = {IPUSHQ, IPOPQ, ICALL, IRET};
    for(uint8_t code : D_rB_ret)
    {
        if(code == icode) return dreg->getrB()->getOutput();
    }
    for(uint8_t code : RSP_ret)
    {
        if(code == icode) return RSP;
    }
    return RNONE;
}


/* Simulation of HCL for dst_E
 * @param: D* - A pointer to an instance of a Decode pipe register class.
 * @param: icode - The icode of the instruction passing through the pipeline.
 * @return: The appropriate dst_E value given the icode.
 */
uint8_t DecodeStage::dst_E(D * dreg, uint8_t icode)
{
    uint8_t D_rB_ret[3] = {IRRMOVQ, IIRMOVQ, IOPQ};
    uint8_t RSP_ret[4] = {IPUSHQ, IPOPQ, ICALL, IRET};
    for(uint8_t code : D_rB_ret)
    {
        if(code == icode) return dreg->getrB()->getOutput();
    }
    for(uint8_t code : RSP_ret)
    {
        if(code == icode) return RSP;
    }
    return RNONE;
}

/* Simulation of HCL for dst_M
 * @param: D* - A pointer to an instance of a Decode pipe register class.
 * @param: icode - The icode of the instruction passing through the pipeline.
 * @return: The appropriate dst_M value given the icode.
 */
uint8_t DecodeStage::dst_M(D * dreg, uint8_t icode)
{
    uint8_t D_rA_ret[2] = {IMRMOVQ, IPOPQ};
    for(uint8_t code : D_rA_ret)
    {
        if(code == icode) return dreg->getrA()->getOutput();
    }
    return RNONE;
}

/**
 * Simplified Sel + FwdA HCL.
 * @param: dreg -  A pointer to an instance of the D pipe register class.
 * @return: d_rvalA 
 */
int64_t DecodeStage::sel_FwdA(D* dreg, W* wreg, M* mreg, uint8_t d_srcA, ExecuteStage* e)
{
    if(d_srcA == e->gete_dstE()) return e->gete_valE();
    if(d_srcA == mreg->getdstE()->getOutput()) return mreg->getvalE()->getOutput();
    if(d_srcA == wreg->getdstE()->getOutput()) return wreg->getvalE()->getOutput();

    uint8_t rA =  dreg->getrA()->getOutput();
    bool error = false;
    RegisterFile * regFile = RegisterFile::getInstance();
    return regFile->readRegister(rA, error);
}


/**
 * Simplified FwdB 
 * @param: derg - A pointer to an instance of the D Pipe register class.
 * @return: d_rvalB
 */
int64_t DecodeStage::FwdB(D* dreg, W* wreg, M* mreg, uint8_t d_srcB, ExecuteStage* e)
{
    if(d_srcB == e->gete_dstE()) return e->gete_valE();
    if(d_srcB == mreg->getdstE()->getOutput()) return mreg->getvalE()->getOutput(); 
    if(d_srcB == wreg->getdstE()->getOutput()) return wreg->getvalE()->getOutput();

    uint8_t rB =  dreg->getrB()->getOutput();
    bool error = false;
    RegisterFile * regFile = RegisterFile::getInstance();
    return regFile->readRegister(rB, error);
}

/* doClockHigh
 * applies the appropriate control signal to the D
 * and E register intances
 *
 * @param: pregs - array of the pipeline register (F, D, E, M, W instances)
 */
void DecodeStage::doClockHigh(PipeReg ** pregs)
{
    D * dreg = (D *) pregs[DREG];
    E * ereg = (E *) pregs[EREG];

    ereg->getstat()->normal();
    ereg->geticode()->normal();
    ereg->getifun()->normal();
    ereg->getvalC()->normal();
    ereg->getvalA()->normal();
    ereg->getvalB()->normal(); 
    ereg->getdstE()->normal();
    ereg->getdstM()->normal();
    ereg->getsrcA()->normal();
    ereg->getsrcB()->normal();

    dreg->getstat()->normal();
    dreg->geticode()->normal();
    dreg->getifun()->normal();
    dreg->getrA()->normal();
    dreg->getrB()->normal();
    dreg->getvalC()->normal();
    dreg->getvalP()->normal();
}

/* setDInput
 * provides the input to potentially be stored in the D register
 * during doClockHigh
 *
 * @param: dreg - pointer to the D register instance
 * @param: stat - value to be stored in the stat pipeline register within D
 * @param: icode - value to be stored in the icode pipeline register within D
 * @param: ifun - value to be stored in the ifun pipeline register within D
 * @param: rA - value to be stored in the rA pipeline register within D
 * @param: rB - value to be stored in the rB pipeline register within D
 * @param: valC - value to be stored in the valC pipeline register within D
 * @param: valP - value to be stored in the valP pipeline register within D
 */
void DecodeStage::setEInput(E * ereg, uint64_t stat, uint64_t icode, 
        uint64_t ifun, uint64_t valC, uint64_t valA,
        uint64_t valB, uint64_t dstE, uint64_t dstM, uint64_t srcA, uint64_t srcB)
{
    ereg->getstat()->setInput(stat);
    ereg->getvalA()->setInput(valA);
    ereg->getvalB()->setInput(valB);
    ereg->geticode()->setInput(icode);
    ereg->getifun()->setInput(ifun);
    ereg->getvalC()->setInput(valC);
    ereg->getdstE()->setInput(dstE);
    ereg->getdstM()->setInput(dstM);
    ereg->getsrcA()-> setInput(srcA);
    ereg->getsrcB()->setInput(srcB);
}

