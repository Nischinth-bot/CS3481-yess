//class to perform the combinational logic of
//the Fetch stage
class ExecuteStage: public Stage
{
    private:
        void setMInput(M* mreg, uint64_t stat, uint64_t icode, uint64_t Cnd, uint64_t valE, uint64_t valA, uint64_t dstE, uint64_t dstM);
        int64_t aluA(E* ereg);
        int64_t aluB(E* ereg);
        uint8_t aluFUN(E* ereg);    
        bool set_cc(uint8_t icode);
        int64_t e_dstE(E* ereg);
        int64_t ALU(E* ereg);
        void CC(E* ereg, int64_t valE);
        bool Cond(uint8_t icode, uint8_t ifun);
    public:
        bool doClockLow(PipeReg ** pregs, Stage ** stages);
        void doClockHigh(PipeReg ** pregs);
        int64_t gete_valE();
        uint8_t gete_dstE();
};
