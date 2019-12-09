//class to perform the combinational logic of
//the Fetch stage
class DecodeStage: public Stage
{
    private:
        uint8_t srcA;
        uint8_t srcB;
        bool E_bubble;
        
        bool EBubble(uint8_t E_icode, uint8_t E_dstM, bool e_Cnd, 
        uint8_t d_srcA, uint8_t d_srcB);
        
        void doEBubble(PipeReg ** pregs);
        void doENormal(PipeReg ** pregs);
        void setEInput(E * dreg, uint64_t stat, uint64_t icode, uint64_t ifun, 
                uint64_t valC, uint64_t valA, uint64_t valB, uint64_t dstE,
                uint64_t dstM, uint64_t srcA, uint64_t srcB);

        uint8_t dst_E(D * dreg, uint8_t icode);
        uint8_t dst_M(D * dreg, uint8_t icode);
        int64_t sel_FwdA(D* dreg, W* wreg, M* mreg, uint8_t d_srcA, ExecuteStage* e, MemoryStage * m);
        int64_t FwdB(D* dreg, W* wreg, M* mreg, uint8_t d_srcB, ExecuteStage* e, MemoryStage* m);


        uint8_t getd_srcA(D * dreg, uint8_t icode);
        uint8_t getd_srcB(D * dreg, uint8_t icode);

    public:
        bool doClockLow(PipeReg ** pregs, Stage ** stages);
        void doClockHigh(PipeReg ** pregs);

        uint8_t get_srcA();
        uint8_t get_srcB();


};
