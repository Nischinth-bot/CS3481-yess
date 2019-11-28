//class to perform the combinational logic of
//the Fetch stage
class MemoryStage: public Stage
{
   private:
      uint64_t valM = 0;
      void setWinput(W * wreg, uint64_t stat, uint64_t icode,
                     uint64_t valE, uint64_t valM,
                     uint64_t dstE, uint64_t dstM);
        uint64_t mem_addr(uint8_t M_icode, int64_t M_valE, int64_t M_valA);
        bool mem_write(uint8_t M_icode);
        bool mem_read(uint8_t M_icode);
   public:
      int64_t getm_valM();
      bool doClockLow(PipeReg ** pregs, Stage ** stages);
      void doClockHigh(PipeReg ** pregs);

};
