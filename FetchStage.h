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
      uint64_t selectPC(F* freg, M* mreg, W* wreg);
      bool need_regids(uint64_t f_icode);
      bool needValC(uint64_t f_icode);
      uint64_t  predictPC(uint64_t f_icode, uint64_t f_valC, uint64_t f_valP);
      uint64_t PCincrement(uint64_t f_icode, bool nregids, bool nvalC);
      void getRegIds(uint64_t f_pc, uint8_t icode, uint8_t &rA, uint8_t &rB);
      uint64_t buildValC(uint64_t f_pc, uint8_t icode);
   
   public:
      bool doClockLow(PipeReg ** pregs, Stage ** stages);
      void doClockHigh(PipeReg ** pregs);

};
