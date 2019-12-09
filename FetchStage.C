#include <string>
#include <cstdint>
#include "Memory.h"
#include "Tools.h"
#include "RegisterFile.h"
#include "PipeRegField.h"
#include "PipeReg.h"
#include "F.h"
#include "D.h"
#include "E.h"
#include "M.h"
#include "W.h"
#include "Stage.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"
#include "MemoryStage.h"
#include "ExecuteStage.h"
#include "DecodeStage.h"
#include "FetchStage.h"



/*
 * doClockLow:
 * Performs the Fetch stage combinational logic that is performed when
 * the clock edge is low.
 *
 * @param: pregs - array of the pipeline register sets (F, D, E, M, W instances)
 * @param: stages - array of stages (FetchStage, DecodeStage, ExecuteStage,
 *         MemoryStage, WritebackStage instances)
 */
bool FetchStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{

    F * freg = (F *) pregs[FREG];
    D * dreg = (D *) pregs[DREG];
    E* ereg = (E *) pregs[EREG];
    M* mreg = (M *) pregs[MREG];

    DecodeStage * d = (DecodeStage *) stages[DSTAGE];
    ExecuteStage * e = (ExecuteStage *) stages[ESTAGE];

    uint64_t icode = 0, ifun = 0,  valP = 0;
    int64_t valC = 0;
    uint8_t rA = RNONE, rB = RNONE, stat = SAOK;

    uint64_t f_pc = selectPC((F*)pregs[FREG],(M*)pregs[MREG], (W*)pregs[WREG]);
    Memory* mem = Memory::getInstance();
    bool error = false;
    uint64_t instr = mem->getByte(f_pc,error); 
    ifun = Tools::getBits(instr,0,3);    
    icode = Tools::getBits(instr,4,7);

    icode = f_icode(icode, f_pc);
    ifun = f_ifun(ifun, f_pc);
    stat = f_stat(icode, f_pc);

    valP = PCincrement(f_pc,need_regids(icode),needValC(icode));
    if(needValC(icode)) valC = buildValC(f_pc, icode); 
    getRegIds(f_pc, icode, rA, rB);
    f_pc = predictPC(icode, valC, valP);
    freg->getpredPC()->setInput(f_pc);

    
    uint8_t D_icode = dreg->geticode()->getOutput();
    uint8_t E_icode = ereg->geticode()->getOutput();
    uint8_t M_icode = mreg->geticode()->getOutput();
    uint8_t E_dstM = ereg->getdstM()->getOutput();
    uint8_t d_srcA = d->get_srcA();
    uint8_t d_srcB = d->get_srcB();
    bool e_cnd = e->gete_Cnd();

    F_stall = FStall(ereg, d, D_icode, E_icode, M_icode);
    D_stall = F_stall;
    D_bubble = doDBubble(D_icode, E_icode, M_icode, E_dstM, e_cnd, d_srcA, d_srcB);

    if(!D_stall) setDInput(dreg, stat, icode, ifun, rA, rB, valC, valP);
    return false;
}

/**
 * selectPC
 * Simulator of the selectPC HCL
 * @param: Freg - Pointer to an instance of Fetch register class
 * @param: Mreg - Pointer to an instance of Memory register class
 * @param: Wreg - Pointer to an intance of Write register class
 * @return: Correct PC value for execution of next instruction
 */
uint64_t FetchStage::selectPC(F* freg, M* mreg, W* wreg)
{
    if(mreg->geticode()->getOutput() == IJXX && !mreg->getCnd()->getOutput()) 
    {
        return mreg->getvalA()->getOutput();
    }
    if(wreg->geticode()->getOutput() == IRET)
    {
        return wreg->getvalM()->getOutput();
    }
    return freg->getpredPC()->getOutput();
}

/**
 * need_regids
 * Simulator of the need_regids HCL
 * @param: f_icode
 * @return: true if f_icode denotes an instruction that needs registers
 */
bool FetchStage::need_regids(uint64_t f_icode)
{
    uint64_t checkArray[NUM_REGIDS] = {IRRMOVQ, IOPQ, IPUSHQ, IPOPQ, IIRMOVQ, IRMMOVQ, IMRMOVQ};
    for (uint64_t ID : checkArray)
        if(f_icode == ID) return 1;
    return 0;    
}
/**
 * needValC
 * Simulator of the need_valC HCL
 * @param: f_icode
 * @return: true if f_icode denotes an instruction that needs valC
 */
bool FetchStage::needValC(uint64_t f_icode)
{
    uint64_t checkArray[NUM_VALC] = {IIRMOVQ, IRMMOVQ, IMRMOVQ, IJXX, ICALL};
    for (uint64_t ID : checkArray)
        if(f_icode == ID) return 1;
    return 0;    
}


/**
 * @param: f_pc : The current PC value.
 * @param: nregids: Boolean indicating whether the instruction 
 * in FetchStage requires register ids for its execution.
 * @param: nvalc: Boolean indicating whether the instruction in FetchStage 
 * requires valC for its execution.
 * @return: The appropriate valP value considering the current instruction.
 */
uint64_t FetchStage::PCincrement(uint64_t f_pc, bool nregids, bool nvalc)
{
    uint64_t increment = 1;
    if(nregids) increment += 1;
    if(nvalc) increment += 8;
    return (f_pc + increment);
}

/**
 *predPC
 *Simulator of the predPC HCL
 *@param: f_icode, f_valC, f_valP
 *@return: the predicted PC value after execution of the current instruction.
 */
uint64_t FetchStage::predictPC(uint64_t f_icode, uint64_t f_valC, uint64_t f_valP)
{
    uint64_t checkArray[NUM_PREDPC] = {IJXX, ICALL};
    for (uint64_t ID : checkArray)
        if(f_icode == ID) return f_valC;
    return f_valP;
}

/**
 * Method sets rA and rB to the appropriate values if required.
 * @param: f_pc - The address of the program counter.
 * @param: icode - The icode of the instruction.
 * @param: rA - A reference to the rA field.
 * @param: rB - A reference to the rB field.
 */
void FetchStage::getRegIds(uint64_t f_pc, uint8_t icode, uint8_t &rA, uint8_t &rB)
{
    if(need_regids(icode))
    {
        bool error = false;
        Memory* mem = Memory::getInstance();
        uint8_t regByte  = mem->getByte(f_pc + 1, error);
        rB = Tools::getBits(regByte, 0, 3);
        rA = Tools::getBits(regByte, 4, 7);

    }
    return;
}

/**
 *Builds valC depending on the instruction.
 *@param: f_pc - The current address of the program counter.
  @param: icode - The icode of the current instruction
  @return: valC - an 8 byte constant word
 */
uint64_t FetchStage::buildValC(uint64_t f_pc, uint8_t icode)
{
    uint8_t byteArray[8] = {0};
    Memory* mem = Memory::getInstance();
    bool error = false;
    int64_t wordIndex = f_pc + 2;
    if(icode == IJXX || icode == ICALL) {wordIndex --;}
    uint8_t j = 0;
    for(int i = wordIndex; i < (wordIndex + 8); i ++)
    {
        byteArray[j++] = mem->getByte(i, error);
    }
    return Tools::buildLong(byteArray);
}


/**
 * Helper method.
 * @param: f_icode
 * @return: true if f_icode represents a valid instruction.
 */
bool FetchStage::instr_valid(uint8_t f_icode)
{
    uint8_t array [] = { INOP, IHALT, IRRMOVQ, IIRMOVQ, IRMMOVQ, IMRMOVQ, 
        IOPQ, IJXX, ICALL, IRET, IPUSHQ, IPOPQ };
    for (uint8_t ID : array)
        if(f_icode == ID) return 1;
    return 0;
}


/**
 * Method to calculate f_stat when the clock is low.
 * @param: f_icode
 * @param: addr:  the current address of PC
 * @return: appropriate stat value depending on icode and addr.
 */
uint8_t FetchStage::f_stat(uint8_t f_icode, uint32_t addr)
{

    if(mem_error(addr)) return SADR;
    if(!instr_valid(f_icode)) return SINS;
    if(f_icode == IHALT) return SHLT;
    return SAOK;
}

/**
 * Helper method.
 * @param: f_icode
 * @param: addr: address of pc
 * @return: INOP if address is invalid, else f_icode.
 */
uint8_t FetchStage::f_icode(uint8_t f_icode, uint32_t addr)
{
    if(mem_error(addr)) return INOP;
    return  f_icode;
}


/**
 * Helper method.
 * @param: ifun: function code of the current instruction.
 * @param: addr: address of the pc.
 * @return: FNONE if address is invalid, else ifun.
 */
uint8_t FetchStage::f_ifun(uint8_t ifun, uint32_t addr)
{
    if(mem_error(addr)) return FNONE;
    return ifun;

}

/**
 * Helper method.
 * @param: addr: the address to check.
 * @return: true if address is invalid.
 */
bool FetchStage::mem_error(uint32_t addr)
{
    return (addr < 0 || addr > MEMSIZE);
}

/**
 * @param: ereg : A pointer to an instance of the E register class.
 * @param: d : A pointer to an instance of the DecodeStage class.
 * @param: D_icode : The icode value in the D register.
 * @param: E_icode : The icode value in the E register.
 * @param: M_icode : The icode value in the M register.
 * @return: True if the Stall control signal should be applied to the 
 * Fetch register at the end of the current clock cycle.
 */
bool FetchStage::FStall(E* ereg, DecodeStage * d, uint8_t D_icode, uint8_t E_icode, uint8_t M_icode)
{
    uint32_t E_dstM = ereg->getdstM()->getOutput();
    bool A =  (E_icode == IMRMOVQ || E_icode == IPOPQ) 
        && (E_dstM == d->get_srcA() || E_dstM == d->get_srcB());
    bool B = (D_icode == IRET || E_icode == IRET || M_icode == IRET);
    return A || B;
}


/**
 * Method to calculate D_bubble when the clock is low.
 * @param: D_icode : icode value in the D register.
 * @param: E_icode : icode value in the E register.
 * @param: M_icode : icode value in the M register.
 * @param: E_dstM : dstM value in the E register.
 * @param: e_Cnd  : Cnd value calculated by the ExecuteStage class in the current
 * clock cycle.
 * @param: d_srcA : srcA value calculated by the DecodeStage class in the current 
 * clock cycle.
 * @param: d_srcB : srcB value calculated by the DecodeStage class in the current
 * clock cycle.
 * @return: True if D register should be bubbled at the end of the curent clock cycle.
 */
bool FetchStage::doDBubble(uint8_t D_icode, uint8_t E_icode, 
uint8_t M_icode, uint8_t E_dstM, bool e_Cnd, uint8_t d_srcA, uint8_t d_srcB)
{
    bool A =  (E_icode == IJXX && !e_Cnd);
    bool B = (D_icode == IRET || E_icode == IRET || M_icode == IRET);
    bool C = !((E_icode == IMRMOVQ || E_icode == IPOPQ) && (E_dstM == d_srcA || E_dstM == d_srcB));
    return (A || B) && C;
}


/* doClockHigh
 * applies the appropriate control signal to the F
 * and D register intances
 *
 * @param: pregs - array of the pipeline register (F, D, E, M, W instances)
 */
void FetchStage::doClockHigh(PipeReg ** pregs)
{
    if(D_bubble) {
        doDBubble(pregs);
    }
    else if (D_stall) {
        doDStall(pregs);
    }
    else{
        doDNormal(pregs);
    }

    F* freg = (F *) pregs[FREG];

    if(F_stall) {
        freg->getpredPC()->stall();
    }
    else {
        freg->getpredPC()->normal();
    }
}


/**
 * Bubble doClockHigh behavior for D register.
 * @param: pregs - array of the pipeline register
 */
void FetchStage::doDBubble(PipeReg ** pregs)
{
    D * dreg = (D *) pregs[DREG];
    dreg->getstat()->bubble(SAOK);
    dreg->geticode()->bubble(INOP);
    dreg->getifun()->bubble();
    dreg->getrA()->bubble(RNONE);
    dreg->getrB()->bubble(RNONE);
    dreg->getvalC()->bubble();
    dreg->getvalP()->bubble();

}

/**
 * Normal doClockHigh behavior for D register.
 * @param: pregs - array of the pipeline register
 */
void FetchStage::doDNormal(PipeReg ** pregs)
{
    D * dreg = (D *) pregs[DREG];
    dreg->getstat()->normal();
    dreg->geticode()->normal();
    dreg->getifun()->normal();
    dreg->getrA()->normal();
    dreg->getrB()->normal();
    dreg->getvalC()->normal();
    dreg->getvalP()->normal();

}


/**
 * Stalled doClockHigh  behavior for D register.
 * @param: pregs - array of the pipeline register
 */
void FetchStage::doDStall(PipeReg ** pregs)
{
    D * dreg = (D *) pregs[DREG];
    dreg->getstat()->stall();
    dreg->geticode()->stall();
    dreg->getifun()->stall();
    dreg->getrA()->stall();
    dreg->getrB()->stall();
    dreg->getvalC()->stall();
    dreg->getvalP()->stall();
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
void FetchStage::setDInput(D * dreg, uint64_t stat, uint64_t icode, 
        uint64_t ifun, uint64_t rA, uint64_t rB,
        uint64_t valC, uint64_t valP)
{
    dreg->getstat()->setInput(stat);
    dreg->geticode()->setInput(icode);
    dreg->getifun()->setInput(ifun);
    dreg->getrA()->setInput(rA);
    dreg->getrB()->setInput(rB);
    dreg->getvalC()->setInput(valC);
    dreg->getvalP()->setInput(valP);
}


