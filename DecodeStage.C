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
#include "MemoryStage.h"
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
    MemoryStage * m = (MemoryStage*) stages[MSTAGE];

    uint64_t icode = 0, ifun = 0, valC = 0, valA = 0, valB = 0;
    uint8_t dstE = RNONE, dstM = RNONE, srcA = RNONE, srcB = RNONE, stat = SAOK; 

    icode = dreg-> geticode()-> getOutput();
    ifun = dreg-> getifun()-> getOutput();
    valC = dreg-> getvalC()-> getOutput(); 
    stat = dreg-> getstat() -> getOutput();

    dstE = dst_E(dreg, icode);
    dstM = dst_M(dreg, icode);
    srcA = getd_srcA(dreg, icode);
    srcB = getd_srcB(dreg, icode);

    valB = FwdB(dreg, wreg, mreg, srcB, e, m);
    valA = sel_FwdA(dreg, wreg, mreg, srcA, e, m);
    
   
    setEInput(ereg,stat, icode,ifun, valC, valA, valB, dstE, dstM, srcA, srcB);
    return false;
}



/* Simulation of HCL for d_srcA
 * @param: D* - A pointer to an instance of a Decode pipe register class.
 * @param: icode - The icode of the instruction passing through the pipeline.
 * @return: The appropriate d_srcA value given the icode.
 */
uint8_t DecodeStage::getd_srcA(D * dreg, uint8_t icode)
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
uint8_t DecodeStage::getd_srcB(D * dreg, uint8_t icode)
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
int64_t DecodeStage::sel_FwdA(D* dreg, W* wreg, M* mreg, uint8_t d_srcA, ExecuteStage* e, MemoryStage * m)
{
    if(dreg->geticode()->getOutput() == ICALL || dreg->geticode()->getOutput() == IJXX) return dreg->getvalP()->getOutput(); 
    //d_srcA == D_valP : D_valP
    if(d_srcA == RNONE) return 0;
    if(d_srcA == e->gete_dstE()) return e->gete_valE(); //d_srcA == e_dstE
    if(d_srcA == mreg->getdstE()->getOutput()) return mreg->getvalE()->getOutput(); //d_srcA == M_dstE :: M_valE
    if(d_srcA == mreg->getdstM()->getOutput()) return m->getm_valM(); //d_srcA == M_dstM :: m_valM
    if(d_srcA == wreg->getdstM()->getOutput()) return wreg->getvalM()->getOutput(); //d_srcA == W_dstM :: W_valM
    if(d_srcA == wreg->getdstE()->getOutput()) return wreg->getvalE()->getOutput(); //d_srcA == W_dstE :: W_valE
    
    bool error = false;
    RegisterFile * regFile = RegisterFile::getInstance();
    return regFile->readRegister(d_srcA, error);
}


/**
 * Simplified FwdB 
 * @param: derg - A pointer to an instance of the D Pipe register class.
 * @return: d_rvalB
 */
int64_t DecodeStage::FwdB(D* dreg, W* wreg, M* mreg, uint8_t d_srcB, ExecuteStage* e, MemoryStage * m)
{
    if(d_srcB == RNONE) return 0;
    if(d_srcB == e->gete_dstE()) return e->gete_valE();
    if(d_srcB == mreg->getdstE()->getOutput()) return mreg->getvalE()->getOutput(); 
    if(d_srcB == mreg->getdstM()->getOutput()) return m->getm_valM(); //d_srcA == M_dstE :: m_valM
    if(d_srcB == wreg->getdstE()->getOutput()) return wreg->getvalE()->getOutput();
    if(d_srcB == wreg->getdstM()->getOutput()) return wreg->getvalM()->getOutput(); //d_srcA == W_dstM :: W_valM
    bool error = false;
    RegisterFile * regFile = RegisterFile::getInstance();
    return regFile->readRegister(d_srcB, error);
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

/**
 * Specialized method to perform pop operation. Puts R[%rsp] in valA and valB.
 * Updates the stack pointer.
 * @param: valA: Reference to the valA variable.
 * @paramm: valB: Reference to the valB variable.
 */
void DecodeStage::performPop(uint64_t &valA, uint64_t &valB)
{
    RegisterFile * regs = RegisterFile::getInstance();
    bool error = false;
    uint64_t rsp = regs->readRegister(RSP, error);
    valA = rsp;
    valB = rsp;
    error = false;
    regs->writeRegister(rsp + 8, RSP, error);
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

