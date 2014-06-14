CXX=g++
CFLAGS=-Wall -I ~/git/qpu -Wfatal-errors --std=c++11 -g -m32 -Wno-unused-function -O3

all: myass

myass: main.o InstructionTree.o DebugPrint.o Assemblable.o LoadStore.o grammar.tab.cpp grammar.tab.hpp lex.yy.c
	$(CXX) -g -o myass main.o Assemblable.o InstructionTree.o DebugPrint.o LoadStore.o grammar.tab.cpp lex.yy.c $(CFLAGS)

InstructionTree.o: InstructionTree.cpp InstructionTree.h ../qpu/shared.h
	$(CXX) InstructionTree.cpp -o InstructionTree.o $(CFLAGS) -c

DebugPrint.o: DebugPrint.cpp InstructionTree.h ../qpu/shared.h
	$(CXX) DebugPrint.cpp -o DebugPrint.o $(CFLAGS) -c

Assemblable.o: Assemblable.cpp InstructionTree.h ../qpu/shared.h
	$(CXX) Assemblable.cpp -o Assemblable.o $(CFLAGS) -c
	
LoadStore.o: LoadStore.cpp LoadStore.h InstructionTree.h ../qpu/shared.h
	$(CXX) LoadStore.cpp -o LoadStore.o $(CFLAGS) -c

main.o: main.cpp InstructionTree.h ../qpu/shared.h grammar.tab.hpp
	$(CXX) main.cpp -o main.o $(CFLAGS) -c

grammar.tab.cpp: grammar.tab.hpp

grammar.tab.hpp: grammar.ypp InstructionTree.h ../qpu/shared.h
	bison -d grammar.ypp

lex.yy.c: tokeniser.l InstructionTree.h ../qpu/shared.h grammar.tab.hpp
	flex tokeniser.l
