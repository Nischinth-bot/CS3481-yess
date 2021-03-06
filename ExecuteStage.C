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
#include "MemoryStage.h"
#include "Instructions.h"
#include "ConditionCodes.h"
#include "Tools.h"
#include "ExecuteStage.h"

void clearCC(ConditionCodes* codes); //HELPER METHOD
/*
 * doClockLow:
 * Performs the Fetch stage combinational logic that is performed when
 * the clock edge is low.
 *
 * @param: pregs - array of the pipeline register sets (F, D, E, M, W instances)
 * @param: stages - array of stages (FetchStage, DecodeStage, ExecuteStage,
 *         MemoryStage, WritebackStage instances)
 */
bool ExecuteStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
    E * ereg = (E *) pregs[EREG];
    M * mreg = (M *) pregs[MREG];
    W* wreg = (W *) pregs[WREG];
    MemoryStage * mptr = (MemoryStage *) stages[MSTAGE];

    uint64_t stat = SAOK, icode = 0, ifun = 0, dstM = RNONE;

    M_bubble = calculateControlSignals(mptr, wreg);
    stat = ereg->getstat()->getOutput();
    ifun = ereg->getifun()->getOutput();
    icode = ereg->geticode()->getOutput();
    dstM = ereg->getdstM()->getOutput();
    dstE = e_dstE(ereg);
    valE = ALU(ereg); 

    Cnd = Cond(icode,ifun);

    if(set_cc(icode, wreg, mptr))
    {
        CC(ereg, valE);
    }
    int64_t valA = ereg->getvalA()->getOutput();
    setMInput(mreg, stat, icode, Cnd, valE, valA, dstE, dstM);
    return 0;
}

/**
 *@param: E_icode: icode from Ereg
 @param: wreg : Pointer to an instance of the WritebackRegister class.
 @param: m : Pointer an instance of the MemoryStage class.
 @return: true if condition codes should be set for the instruction currently in the 
 E register.
 */
bool ExecuteStage::set_cc(uint64_t E_icode, W* wreg, MemoryStage* m)
{
    bool ic = (E_icode == IOPQ);
    if(!ic) return 0;

    uint64_t mstat = m->getm_stat();
    bool nmstat = (mstat != SADR && mstat != SINS && mstat != SHLT); 
    if(!nmstat) return 0;  //!(m_stat in {SADR,SINS,SHLT});

    uint64_t wstat = wreg->getstat()->getOutput();
    bool nwstat = (wstat != SADR && wstat != SINS && wstat != SHLT);    
    if(!nwstat) return 0; //!(W_stat() in {SADR,SINS,SHLT};)

    return 1;
}

/**
 *Simulator for aluA HCL 
 *@param: A pointer to the instance of the E register class.
 *@return: Appropriate aluA value depending on E_icode
 */
int64_t ExecuteStage::aluA(E * ereg)
{
    uint8_t E_icode = ereg->geticode()->getOutput();
    if(E_icode == IRRMOVQ || E_icode == IOPQ) return ereg->getvalA()->getOutput();
    if(E_icode == IIRMOVQ || E_icode == IRMMOVQ || E_icode == IMRMOVQ) return ereg->getvalC()->getOutput();
    if(E_icode == ICALL || E_icode == IPUSHQ) return -8;
    if(E_icode == IRET || E_icode == IPOPQ) return 8; 
    return 0;
}

/*
 * Simulator for aluB HCL
 * @param: A pointer to the instance of the E register class.
 * @return: Appropriate aluB value depending on E_icode
 */
int64_t ExecuteStage::aluB(E* ereg)
{
    uint8_t E_icode = ereg->geticode()->getOutput();
    if(E_icode == IRRMOVQ || E_icode == IIRMOVQ) return 0;
    uint8_t E_valB[] = {IRMMOVQ, IMRMOVQ, IOPQ, ICALL, IPUSHQ, IRET, IPOPQ};
    for(uint8_t icode : E_valB)
        if(E_icode == icode) return ereg->getvalB()->getOutput();
    return 0;
}

/*
 * Simulator for aluFUN HCL
 * @param: A pointer to the instance of the E register class.
 * @return: Appropriate aluFUN value depending on E_icode
 */
uint8_t ExecuteStage::aluFUN(E* ereg)
{
    uint8_t E_icode = ereg->geticode()->getOutput();
    if(E_icode == IOPQ) return ereg->getifun()->getOutput();
    return 0;
}

/**
 * Simulator for e_dstE HCL 
 * @param: pregs - Array of pipeline registers
 *
 * @param: A pointer to the instance of the E register class.
 * @return: The appropriate e_dstE value depending on E_icode
 */
int64_t ExecuteStage::e_dstE(E* ereg)
{    
    if(ereg->geticode()->getOutput() == IRRMOVQ 
            && !Cond(ereg->geticode()->getOutput(),ereg->getifun()->getOutput())) 
        return RNONE;
    return ereg->getdstE()->getOutput();
}

/**
 * Simulator for CC unit
 * Sets the condition codes if set_cc returns true
 * @param: A pointer to the instance of the E register class.
 */
void ExecuteStage::CC(E* ereg, int64_t valE)
{
    ConditionCodes * codes = ConditionCodes::getInstance();
    clearCC(codes);
    uint8_t ifun = aluFUN(ereg);
    bool error = false;
    if(valE == 0) //if valE is 0, then set the ZF code and return.
    {
        codes->setConditionCode(1,ZF,error); 
    }
    if(ifun == 0) //if ifun is ADDQ, then check if there was overflow and set OF code.
    {
        if(Tools::addOverflow(aluA(ereg), aluB(ereg)))
        {
            codes->setConditionCode(1,OF,error); 
        }
    }
    else if(ifun == 1)//or if ifun is SUBQ, then check if there was overflow and set OF code.
    {
        if(Tools::subOverflow(aluA(ereg), aluB(ereg)))
        {
            codes->setConditionCode(1,OF,error);
        }
    }

    if(Tools::sign(valE) == 1)  //check if valE is negative and set SF if so.
    {
        codes->setConditionCode(1,SF,error); 
        return;
    }
}
/**
 * Simulator of the ALU hardware unit.
 * @param: ereg - A pointer to the instance of the E register class.
 * @return: valE - a 64 bit value 
 */

int64_t ExecuteStage::ALU(E* ereg)
{
    uint64_t A = aluA(ereg);
    uint64_t B = aluB(ereg);
    uint8_t ifun = aluFUN(ereg);
    switch(ifun)
    {
        case 1 : return B - A; //SUBQ
        case 2:  return B & A; //ANDQ
        case 3:  return B ^ A; //XORQ 
        default: break;
    }
    return B + A; //ANDQ
}

/**
 *Simulator for the Cond hardware.
 @param: E_icode 
 @param: E_ifun
 @return : Appropriate e_Cnd value as would be calculated by the Cond hardware.
 */
bool ExecuteStage::Cond(uint8_t icode, uint8_t ifun)
{
    if(icode != IJXX && icode != ICMOVXX) 
    {
        return 0;
    }
    else
    {

        ConditionCodes * cc = ConditionCodes::getInstance();
        bool error = false;
        bool sf = cc->getConditionCode(SF,error);
        bool of = cc->getConditionCode(OF,error);
        bool zf = cc->getConditionCode(ZF,error);

        switch(ifun)
        {
            case 0:  // jmp/rrmovq
                return 1;
            case 1: // jle/cmovle
                error = false;
                sf = cc->getConditionCode(SF,error);
                of = cc->getConditionCode(OF,error);
                zf = cc->getConditionCode(ZF,error);
                return ((sf^of) | zf);
            case 2:  //jl/cmovl             
                error = false;
                sf = cc->getConditionCode(SF,error);
                of = cc->getConditionCode(OF,error);
                return (sf^of);
            case 3: //je/cmove
                error = false;
                zf = cc->getConditionCode(ZF,error);
                return zf;
            case 4: //jne/cmovne
                error = false;
                zf = cc->getConditionCode(ZF,error);
                return !zf;
            case 5: //jge/cmovge
                error = false;
                sf = cc->getConditionCode(SF,error);
                of = cc->getConditionCode(OF,error);
                return (!(sf^of));
            case 6: //jg/cmvog
                error = false;
                sf = cc->getConditionCode(SF,error);
                of = cc->getConditionCode(OF,error);
                zf = cc->getConditionCode(ZF,error);
                return (!(sf^of) & !zf);

            default:
                return 0;
        }
    }
}

/**
 * Public helper method.
 * @return: e_valE
 */
int64_t ExecuteStage::gete_valE()
{
    return valE;
}

/**
 * Public helper method.
 * @return: e_dstE
 */
uint8_t ExecuteStage::gete_dstE()
{
    return dstE;
}

/**
 * Public helper method.
 * @returb: e_Cnd
 */
bool ExecuteStage::gete_Cnd()
{
    return Cnd;
}


/**
 * @param: mptr: A pointer to an instance of the MemoryStage class.
 * @param: wreg: A pointer to an instance of the Writeback register class.
 * @return: True if the M register should be bubbled at the end
 * of the current clock cycle.
 */
bool ExecuteStage::calculateControlSignals(MemoryStage * mptr, W* wreg)
{
    uint8_t m_stat = mptr->getm_stat();
    if(m_stat == SADR || m_stat == SINS || m_stat == SHLT) return 1;
    uint8_t W_stat = wreg->getstat()->getOutput();
    if(W_stat == SADR || W_stat == SINS || W_stat == SHLT) return 1;
    return 0;
}

/**
 * Helper method that clears the condition codes before each call to CC. Precautionary.
 * @param: codes - A pointer to the ConditionCodes instance
 */
void clearCC(ConditionCodes* codes)
{
    bool error = false;
    codes->setConditionCode(0,OF,error);
    codes->setConditionCode(0,ZF,error);
    codes->setConditionCode(0,SF,error);
}


/* doClockHigh
 * applies the appropriate control signal to the F
 * and D register intances
 *
 * @param: pregs - array of the pipeline register (F, D, E, M, W instances)
 */
void ExecuteStage::doClockHigh(PipeReg ** pregs)
{
    E * ereg = (E *) pregs[EREG];
    M * mreg = (M*) pregs[MREG];

    ereg->getstat()->normal();
    ereg->geticode()->normal();
    ereg->getifun()->normal();
    ereg->getvalC()->normal();
    ereg-> getvalA() -> normal();
    ereg-> getvalB() -> normal();
    ereg-> getdstE() -> normal();
    ereg-> getsrcA() -> normal();
    ereg-> getsrcB() -> normal();

    if(!M_bubble)
    {
        mreg->getstat()->normal();
        mreg->geticode()->normal();
        mreg->getCnd()->normal();
        mreg->getvalE()->normal();
        mreg->getvalA()->normal();
        mreg->getdstE()->normal();
        mreg->getdstM()->normal(); 
    }
    else
    {
        mreg->getstat()->bubble(SAOK);
        mreg->geticode()->bubble(INOP);
        mreg->getCnd()->bubble();
        mreg->getvalE()->bubble();
        mreg->getvalA()->bubble();
        mreg->getdstE()->bubble(RNONE);
        mreg->getdstM()->bubble(RNONE); 
    }

}

void ExecuteStage::setMInput(M* mreg, uint64_t stat, uint64_t icode, uint64_t Cnd, uint64_t valE, 
        uint64_t valA, uint64_t dstE, uint64_t dstM)
{
    mreg->getstat()->setInput(stat);
    mreg->geticode()->setInput(icode);
    mreg->getCnd()-> setInput(Cnd);
    mreg->getvalE()->setInput(valE);
    mreg->getvalA()->setInput(valA);
    mreg->getdstE()->setInput(dstE);
    mreg->getdstM()->setInput(dstM);
}


