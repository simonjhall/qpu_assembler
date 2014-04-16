/*
 * main.cpp
 *
 *  Created on: 15 Apr 2014
 *      Author: simon
 */

#include "InstructionTree.h"
#include "grammar.tab.hpp"

#include <stdio.h>

extern FILE *yyin;
extern std::list<Base *> s_statements;

int main(int argc, const char *argv[])
{
	if (argc == 2)
		yyin = fopen(argv[1], "r");

	yyparse();

	for (auto it = s_statements.begin(); it != s_statements.end(); it++)
		(*it)->DebugPrint(0);

	for (auto it = s_statements.begin(); it != s_statements.end(); it++)
	{
		Assemblable *p = dynamic_cast<Assemblable *>(*it);
		if (p)
		{
			Assemblable::Fields f;
			p->Assemble(f);

			uint64_t output = Assemblable::CombineFields(f);
			printf("0x%08lx, 0x%08lx,\n", output & 0xffffffff, output >> 32);
		}
	}

return 0;
}

