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
#include <string.h>
#include <stdlib.h>

extern FILE *yyin;
extern std::list<Base *> s_statements;

std::list<Label *> s_declaredLabels;
std::list<Label *> s_usedLabels;

int main(int argc, const char *argv[])
{
	unsigned int baseAddress = 0;

	if (argc >= 2)
		baseAddress = strtol(argv[1], 0, 16);
	if (argc >= 3)
		yyin = fopen(argv[2], "r");

	printf("base address is %08x\n", baseAddress);

	yyparse();

	for (auto it = s_statements.begin(); it != s_statements.end(); it++)
		(*it)->DebugPrint(0);

	unsigned int address = baseAddress;

	//walk it once, to get addresses and fill in labels
	for (auto it = s_statements.begin(); it != s_statements.end(); it++)
	{
		Label *l = dynamic_cast<Label *>(*it);
		if (l)
			l->SetAddress(address);

		Assemblable *p = dynamic_cast<Assemblable *>(*it);
		if (p)
		{
			Assemblable::Fields f;
			p->Assemble(f);

			unsigned int sizeInBytes;
			uint64_t output;
			if (!Assemblable::CombineFields(f, sizeInBytes, output))
				assert(!"failed to combine fields\n");

			address += sizeInBytes;
		}
	}

	//link labels
	for (auto outer = s_usedLabels.begin(); outer != s_usedLabels.end(); outer++)
	{
		bool found = false;

		for (auto inner = s_declaredLabels.begin(); inner != s_declaredLabels.end(); inner++)
		{
			if (strcmp((*outer)->GetName(), (*inner)->GetName()) == 0)
			{
				(*outer)->Link(*inner);
				found = true;
				break;
			}
		}

		if (!found)
		{
			printf("undefined reference to \'%s\'\n", (*outer)->GetName());
			assert(0);
		}
	}


	address = baseAddress;

	//and a second time for the output
	for (auto it = s_statements.begin(); it != s_statements.end(); it++)
	{
		Assemblable *p = dynamic_cast<Assemblable *>(*it);
		if (p)
		{
			Assemblable::Fields f;
			p->Assemble(f);

			unsigned int sizeInBytes;
			uint64_t output;
			if (!Assemblable::CombineFields(f, sizeInBytes, output))
				assert(!"failed to combine fields\n");

			printf("/*%08x*/\t", address);

			switch (sizeInBytes)
			{
			case 0:			//labels
				printf("\n");
				break;
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

			address += sizeInBytes;
		}
	}

return 0;
}

