CC = g++
CFLAGS = -g -c -Wall -std=c++11 -Og
OBJ = lab5.o MemoryTester.o Memory.o Tools.o RegisterFile.o \
RegisterFileTester.o ConditionCodes.o ConditionCodesTester.o

.C.o:
	$(CC) $(CFLAGS) $< -o $@


lab5: $(OBJ)

Tools.o : Tools.h Tools.C

clean:
	rm $(OBJ) lab5

run:
	make clean
	make lab5
	./run.sh

