CC = g++
CFLAGS = -g -c -Wall -std=c++11 -Og
OBJ = yess.o Loader.o RegisterFile.o Memory.o ConditionCodes.o Tools.o Simulate.o FetchStage.o DecodeStage.o \
ExecuteStage.o MemoryStage.o WritebackStage.o F.o D.o E.o M.o W.o PipeReg.o PipeRegField.o 

.C.o:
	$(CC) $(CFLAGS) $< -o $@


yess: $(OBJ)

yess.o : yess.C Debug.h Memory.h Loader.h RegisterFile.h ConditionCodes.h PipeReg.h Stage.h Simulate.h

Simulate.o : Simulate.C Simulate.h F.h D.h E.h M.h W.h FetchStage.h DecodeStage.h \
ExecuteStage.h MemoryStage.h WritebackStage.h Memory.h RegisterFile.h \
ConditionCodes.h PipeRegField.h PipeReg.h

FetchStage.o : FetchStage.h F.h D.h E.h M.h W.h Stage.h Status.h Debug.h Memory.h Tools.h RegisterFile.h \
PipeRegField.h PipeReg.h Instructions.h 
DecodeStage.o : DecodeStage.h F.h D.h E.h M.h W.h Stage.h Status.h Debug.h RegisterFile.h PipeRegField.h PipeReg.h \
ExecuteStage.h MemoryStage.h DecodeStage.h Instructions.h
ExecuteStage.o : ExecuteStage.h F.h D.h E.h M.h W.h Stage.h Status.h Debug.h MemoryStage.h PipeRegField.h \
PipeReg.h Instructions.h Tools.h ConditionCodes.h RegisterFile.h
MemoryStage.o : MemoryStage.h F.h D.h E.h M.h W.h Stage.h Status.h Debug.h RegisterFile.h PipeRegField.h PipeReg.h \
Instructions.h Memory.h
WritebackStage.o :WritebackStage.h F.h D.h E.h M.h W.h Stage.h Status.h Debug.h RegisterFile.h PipeRegField.h PipeReg.h \
Instructions.h 

PipeRegField.o : PipeRegField.C PipeRegField.h
PipeReg.o : PipeReg.C PipeReg.h

F.o : F.C F.h PipeRegField.h PipeReg.h
D.o : D.C D.h PipeRegField.h PipeReg.h
E.o : E.C E.h PipeRegField.h PipeReg.h
M.o : M.C M.h PipeRegField.h PipeReg.h
W.o : W.C W.h PipeRegField.h PipeReg.h

Tools.o : Tools.C Tools.h

clean:
	rm *.o

run:
	make clean
	make yess
	./run.sh

