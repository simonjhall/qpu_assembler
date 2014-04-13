all:
	bison -d grammar.ypp
	flex tokeniser.l
	g++ -g -c -o InstructionTree.o InstructionTree.cpp -Wall -I ~/git/qpu -Wfatal-errors --std=c++11
	g++ -g -o mycalc InstructionTree.o grammar.tab.cpp lex.yy.c -I ~/git/qpu -Wfatal-errors --std=c++11
