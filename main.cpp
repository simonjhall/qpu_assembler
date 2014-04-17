/*
 * main.cpp
 *
 *  Created on: 15 Apr 2014
 *      Author: simon
 */

#include "InstructionTree.h"
#include "grammar.tab.hpp"

#include <stdio.h>
#include <assert.h>

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

			unsigned int sizeInBytes;
			uint64_t output = Assemblable::CombineFields(f, sizeInBytes);

			switch (sizeInBytes)
			{
			case 1:
				printf("0x%02lx,\n", output & 0xff);
				break;
			case 2:
				printf("0x%04lx,\n", output & 0xffff);
				break;
			case 4:
				printf("0x%08lx,\n", output & 0xffffffff);
				break;
			case 8:
				printf("0x%08lx, 0x%08lx,\n", output & 0xffffffff, output >> 32);
				break;
			default:
				assert(0);
			}
		}
	}

return 0;
}

