//class to perform the combinational logic of
//the Fetch stage
class DecodeStage: public Stage
{
   private:
      void setEInput(E * dreg, uint64_t stat, uint64_t icode, uint64_t ifun, 
                     uint64_t valC, uint64_t valA, uint64_t valB, uint64_t dstE,
                     uint64_t dstM, uint64_t srcA, uint64_t srcB);
      uint8_t d_srcA(D * dreg, uint8_t icode);
      uint8_t d_srcB(D * dreg, uint8_t icode);
      uint8_t dst_E(D * dreg, uint8_t icode);
      uint8_t dst_M(D * dreg, uint8_t icode);
      uint64_t sel_FwdA(D* dreg);
      uint64_t FwdB(D* dreg);
   
   public:
      bool doClockLow(PipeReg ** pregs, Stage ** stages);
      void doClockHigh(PipeReg ** pregs);

};
