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

#include <map>

int yyparse (void);

extern FILE *yyin;
extern std::list<Base *> s_statements;

std::list<Label *> s_declaredLabels;
std::list<Label *> s_usedLabels;
std::map<Register, DependencyBase *> s_scoreboard;

void ClearScoreboard(DependencyBase *p)
{
	for (int count = 32; count < 64; count++)
	{
		s_scoreboard[Register(Register::kRa, count)] = 0;
		s_scoreboard[Register(Register::kRb, count)] = p;
	}

	for (int count = 0; count < 32; count++)
	{
		s_scoreboard[Register(Register::kRa, count)] = p;
		s_scoreboard[Register(Register::kRb, count)] = p;
	}

	for (int count = 0; count < 6; count++)
		s_scoreboard[Register(Register::kAcc, count)] = p;

	s_scoreboard[Register(Register::kRa, 32)] = p;
	s_scoreboard[Register(Register::kRa, 35)] = p;
	s_scoreboard[Register(Register::kRa, 38)] = p;

	s_scoreboard[Register(Register::kRa, 39)] = 0;
	s_scoreboard[Register(Register::kRb, 39)] = 0;

	s_scoreboard[Register(Register::kRa, 41)] = p;
	s_scoreboard[Register(Register::kRa, 42)] = p;
	s_scoreboard[Register(Register::kRa, 48)] = p;
	s_scoreboard[Register(Register::kRa, 49)] = p;
	s_scoreboard[Register(Register::kRa, 50)] = p;
	s_scoreboard[Register(Register::kRa, 51)] = p;
}

int main(int argc, const char *argv[])
{
	unsigned int baseAddress = 0;

	if (argc >= 2)
		baseAddress = strtoll(argv[1], 0, 16);
	if (argc >= 3)
		yyin = fopen(argv[2], "r");

	printf("/""*\n");

	printf("base address is %08x\n", baseAddress);

	if (yyparse() != 0)
		assert(!"failure during yyparse\n");

	printf("initially read code\n");
	for (auto it = s_statements.begin(); it != s_statements.end(); it++)
		(*it)->DebugPrint(0);

	ClearScoreboard(0);

	bool reorder_enabled = false;
	for (auto it = s_statements.begin(); it != s_statements.end(); it++)
	{
		ReorderControl *pReorder = dynamic_cast<ReorderControl *>(*it);
		if (pReorder && pReorder->IsBegin())
		{
			reorder_enabled = true;
			ClearScoreboard(0);
			continue;
		}
		else if (pReorder && pReorder->IsEnd())
		{
			reorder_enabled = false;
			continue;
		}

		if (reorder_enabled)
		{
			DependencyConsumer *pConsumer = dynamic_cast<DependencyConsumer *>(*it);
			DependencyProvider *pProvider = dynamic_cast<DependencyProvider *>(*it);

			if (pConsumer)
			{
				Register::Registers regs;
				pConsumer->GetInputDeps(regs);

				for (auto it = regs.begin(); it != regs.end(); it++)
				{
					DependencyBase *p = s_scoreboard[*it];
					if (p)
						pConsumer->AddInputDep(*p);
				}
			}

			if (pProvider)
			{
				DependencyBase::Dependencies deps;
				pProvider->GetOutputDeps(deps);

//				for (auto it = deps.begin(); it != deps.end(); it++)
//					s_scoreboard[it->
			}
		}
	}

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

	printf("*""/\n");

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
				printf("0x%02llx,\n", output & 0xff);
				break;
			case 2:
				printf("0x%04llx,\n", output & 0xffff);
				break;
			case 4:
				printf("0x%08llx,\n", output & 0xffffffff);
				break;
			case 8:
				printf("0x%08llx, 0x%08llx,\n", output & 0xffffffff, output >> 32);
				break;
			default:
				assert(0);
			}

			address += sizeInBytes;
		}
	}

return 0;
}

