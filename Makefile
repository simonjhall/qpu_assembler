CXX=g++
CFLAGS=-Wall -I ~/git/qpu -Wfatal-errors --std=c++11

all: myass

myass: InstructionTree.o DebugPrint.o Assemblable.o grammar.tab.cpp grammar.tab.hpp
	g++ -g -o myass Assemblable.o InstructionTree.o DebugPrint.o grammar.tab.cpp lex.yy.c $(CFLAGS)

InstructionTree.o: InstructionTree.cpp InstructionTree.h ../qpu/shared.h
	$(CXX) InstructionTree.cpp -o InstructionTree.o $(CFLAGS) -c

DebugPrint.o: DebugPrint.cpp InstructionTree.h ../qpu/shared.h
	$(CXX) DebugPrint.cpp -o DebugPrint.o $(CFLAGS) -c

Assemblable.o: Assemblable.cpp InstructionTree.h ../qpu/shared.h
	$(CXX) Assemblable.cpp -o Assemblable.o $(CFLAGS) -c

grammar.tab.cpp: grammar.tab.hpp

grammar.tab.hpp: grammar.ypp InstructionTree.h ../qpu/shared.h
	bison -d grammar.ypp

lex.yy.c: tokeniser.l InstructionTree.h ../qpu/shared.h grammar.tab.hpp
	flex tokeniser.l