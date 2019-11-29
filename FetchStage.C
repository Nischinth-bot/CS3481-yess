#include <string>
#include <cstdint>
#include "Memory.h"
#include "Tools.h"
#include "RegisterFile.h"
#include "PipeRegField.h"
#include "PipeReg.h"
#include "F.h"
#include "D.h"
#include "M.h"
#include "W.h"
#include "Stage.h"
#include "FetchStage.h"
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
bool FetchStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
    F * freg = (F *) pregs[FREG];
    D * dreg = (D *) pregs[DREG];

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

    setDInput(dreg, stat, icode, ifun, rA, rB, valC, valP);
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
 * Simulator of the need_regids HCL
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
 *
 */
uint64_t FetchStage::PCincrement(uint64_t f_pc, bool nregids, bool nvalc)
{
    int increment = 1;
    if(nregids) increment += 1;
    if(nvalc) increment += 8;
    return (f_pc + increment);
}

/**
 *predPC
 *Simulator of the predPC HCL
 *@param: f_icode, f_valC, f_valP
 *@return: the correct PC value 
 */
uint64_t FetchStage::predictPC(uint64_t f_icode, uint64_t f_valC, uint64_t f_valP)
{
    uint64_t checkArray[NUM_PREDPC] = {IJXX, ICALL};
    for (uint64_t ID : checkArray)
        if(f_icode == ID) return f_valC;
    return f_valP;
}

/* doClockHigh
 * applies the appropriate control signal to the F
 * and D register intances
 *
 * @param: pregs - array of the pipeline register (F, D, E, M, W instances)
 */
void FetchStage::doClockHigh(PipeReg ** pregs)
{
    F * freg = (F *) pregs[FREG];
    D * dreg = (D *) pregs[DREG];

    freg->getpredPC()->normal();
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


bool FetchStage::instr_valid(uint8_t f_icode)
{
    uint8_t array [] = { INOP, IHALT, IRRMOVQ, IIRMOVQ, IRMMOVQ, IMRMOVQ, 
        IOPQ, IJXX, ICALL, IRET, IPUSHQ, IPOPQ };
    for (uint8_t ID : array)
        if(f_icode == ID) return 1;
    return 0;
}

uint8_t FetchStage::f_stat(uint8_t f_icode, uint32_t addr)
{

    if(mem_error(addr)) return SADR;
    if(!instr_valid(f_icode)) return SINS;
    if(f_icode == IHALT) return SHLT;
    return SAOK;
}

uint8_t FetchStage::f_icode(uint8_t f_icode, uint32_t addr)
{
    if(mem_error(addr)) return INOP;
    return  f_icode;
}

uint8_t FetchStage::f_ifun(uint8_t ifun, uint32_t addr)
{
    if(mem_error(addr)) return FNONE;
    return ifun;

}

bool FetchStage::mem_error(uint32_t addr)
{
    return (addr < 0 || addr > MEMSIZE);
}

    
