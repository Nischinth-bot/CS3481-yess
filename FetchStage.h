//class to perform the combinational logic of
//the Fetch stage
#define NUM_REGIDS 7
#define NUM_VALC 5
#define NUM_PREDPC 2

class FetchStage: public Stage
{
    private:
        void setDInput(D * dreg, uint64_t stat, uint64_t icode, uint64_t ifun, 
                uint64_t rA, uint64_t rB,
                uint64_t valC, uint64_t valP);
        void getRegIds(uint64_t f_pc, uint8_t icode, uint8_t &rA, uint8_t &rB);
        void doDNormal(PipeReg ** pregs);
        void doDStall(PipeReg ** pregs);
        void doDBubble(PipeReg ** pregs);
        
        uint64_t selectPC(F* freg, M* mreg, W* wreg);
        uint64_t  predictPC(uint64_t f_icode, uint64_t f_valC, uint64_t f_valP);
        uint64_t PCincrement(uint64_t f_icode, bool nregids, bool nvalC);
        uint64_t buildValC(uint64_t f_pc, uint8_t icode);

        uint8_t f_icode(uint8_t f_icode, uint32_t addr);
        uint8_t f_ifun(uint8_t ifun, uint32_t addr);
        uint8_t f_stat(uint8_t f_icode, uint32_t addr);
        
        bool instr_valid(uint8_t icode);
        bool mem_error(uint32_t addr);
        bool FStall(E* ereg, DecodeStage * d, uint8_t D_icode, uint8_t E_icode, uint8_t M_icode);
        bool need_regids(uint64_t f_icode);
        bool needValC(uint64_t f_icode);
        bool doDBubble(uint8_t D_icode, uint8_t E_icode, 
        uint8_t M_icode, uint8_t E_dstM,bool e_cnd, uint8_t d_srcA, uint8_t d_srcB);
        
        bool F_stall;
        bool D_stall;
        bool D_bubble;

    public:
        bool doClockLow(PipeReg ** pregs, Stage ** stages);
        void doClockHigh(PipeReg ** pregs);

};
