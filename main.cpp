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
#include <set>
#include <vector>

int yyparse (void);

extern FILE *yyin;
extern std::list<Base *> s_statements, s_statementsRescheduled;

std::list<Label *> s_declaredLabels;
std::list<Label *> s_usedLabels;

Instruction &s_rSpareNop = AluInstruction::Nop();

std::list<Base *>::iterator BuildDeps(std::list<Base *>::iterator start, std::list<DependencyBase *> &rScoreboard, bool &rReordering)
{
	bool first = true;
	std::list<Base *>::iterator out = s_statements.end();

	for (auto it = start; it != s_statements.end(); it++)
	{
		ReorderControl *pReorder = dynamic_cast<ReorderControl *>(*it);
		if (pReorder)
		{
			bool newState = pReorder->IsBegin();

			if (newState != rReordering)
				printf("changing reordering from %s to %s\n", rReordering ? "ON" : "OFF", newState ? "ON" : "OFF");

			rReordering = newState;
			out = ++it;
			break;
		}

		DependencyConsumer *pConsumer = dynamic_cast<DependencyConsumer *>(*it);
		DependencyProvider *pProvider = dynamic_cast<DependencyProvider *>(*it);

		if (dynamic_cast<Label *>(*it) && !first)
		{
			out = it;
			break;
		}

		first = false;

		if (pConsumer)
		{
			Dependee::Dependencies deps;
			pConsumer->GetInputDeps(deps);

			for (auto d = deps.begin(); d != deps.end(); d++)
			{
				for (auto s = rScoreboard.begin(); s != rScoreboard.end(); s++)
				{
					DependencyBase *p = *s;
					if ((*d)->SatisfiesThis(*p))
					{
						pConsumer->AddResolvedInputDep(*p);
						break;
					}
				}
			}
		}

		if (pProvider)
		{
			DependencyBase::Dependencies deps;
			pProvider->GetOutputDeps(deps);

			for (auto d = deps.begin(); d != deps.end(); d++)
			{
				for (auto s = rScoreboard.begin(); s != rScoreboard.end(); s++)
				{
					if ((*d)->ProvidesSameThing(*(*s)))
					{
						rScoreboard.erase(s);
						break;
					}
				}

				rScoreboard.push_back(*d);
			}
		}

		if (dynamic_cast<BranchInstruction *>(*it) || !rReordering)
		{
			out = ++it;
			break;
		}
	}

	int count = 0;
	for (auto it = start; it != out; it++)
	{
		DependencyConsumer *pConsumer = dynamic_cast<DependencyConsumer *>(*it);
		if (pConsumer)
		{
			printf("instruction count %d, provider %p\n", count, dynamic_cast<DependencyProvider *>(*it));
			pConsumer->DebugPrintResolvedDeps();
			count++;
		}
	}

	return out;
}

void EmitNonInstructions(std::list<Base *>::iterator start, std::list<Base *>::iterator end)
{
	for (auto it = start; it != end; it++)
		if (!dynamic_cast<Instruction *>(*it))
			s_statementsRescheduled.push_back(*it);
}

static int schedules_run = 0;
static int found_solutions = 0;

void Schedule(std::list<DependencyProvider *> runInstructions, std::list<DependencyConsumer *> instructionsToRun,
		std::map<DependencyBase *, int> scoreboard, std::list<DependencyBase *> &rFinalScoreboard,
		std::list<DependencyProvider *> &rBestSchedule, bool &rFoundSchedule,
		bool branchInserted, int delaySlotsToFill,
		int currentCycle)
{
	assert(instructionsToRun.size() != 0);
	assert(delaySlotsToFill >= 0);

	schedules_run++;

	for (auto inst = instructionsToRun.begin(); inst != instructionsToRun.end(); inst++)
	{
		Instruction *i = dynamic_cast<Instruction *>(*inst);
		assert(i);

		bool isBranch;
		if (dynamic_cast<BranchInstruction *>(i))
			isBranch = true;
		else
			isBranch = false;

		//only one branch
		assert((isBranch && !branchInserted)
				|| !isBranch);

		assert(!isBranch || (isBranch && delaySlotsToFill == 3));

		//no point in running more if we have more instructions to do that branch delay slots free
		if (isBranch && ((int)instructionsToRun.size() - 1 > delaySlotsToFill))
			continue;

		//check if this one can run
		DependencyBase::Dependencies deps = (*inst)->GetResolvedInputDeps();

		int nopsNeeded = 0;
		size_t canRun = 0;

		for (auto dep = deps.begin(); dep != deps.end(); dep++)
		{
			int nops;
			if (!(*dep)->CanRun(scoreboard, nops, currentCycle))
				break;
			else
			{
				if (nops > nopsNeeded)
					nopsNeeded = nops;
				canRun++;
			}
		}

		if (canRun != deps.size())
			continue;

		std::list<DependencyProvider *> newRunInstructions = runInstructions;

		int inserted = nopsNeeded + 1;			//x nops plus the actual instruction

		//no more room to run
		if (branchInserted && (delaySlotsToFill - inserted < 0))
			return;

		//we can run, and we know how many nops need to be inserted
		for (auto count = 0; count < nopsNeeded; count++)
			newRunInstructions.push_back(&s_rSpareNop);

		//add in the instruction
		newRunInstructions.push_back(i);

		//write to the scoreboard
		DependencyBase::Dependencies outDeps;
		i->GetOutputDeps(outDeps);

		std::map<DependencyBase *, int> newScoreboard = scoreboard;

		//whilst getting rid of anything which already provides that dep
		for (auto d = outDeps.begin(); d != outDeps.end(); d++)
		{
			for (auto s = newScoreboard.begin(); s != newScoreboard.end(); s++)
				if ((*d)->ProvidesSameThing(*s->first))
				{
					newScoreboard.erase(s);
					break;
				}
			newScoreboard[*d] = currentCycle + nopsNeeded;
		}

		if (instructionsToRun.size() == 1)		//we already have processed the last one
		{
			//check the scoreboard matches the final scoreboard
			//and count how many nops are needed to make the scoreboard 'valid'
			//plus how many delay slots are still to fill
			int nops = delaySlotsToFill;

			//adjust delay slot to account for ones already inserted
			if (branchInserted)
			{
				if (nops)
					nops -= inserted;
				if (nops < 0)
					nops = 0;
			}

			for (auto final = rFinalScoreboard.begin(); final != rFinalScoreboard.end(); final++)
			{
				bool found = false;
				for (auto result = newScoreboard.begin(); result != newScoreboard.end(); result++)
				{
					int moreNopsNeeded;
					if (!result->first->CanRun(newScoreboard, moreNopsNeeded, currentCycle + nopsNeeded + 1))
						assert(!"not found in scoreboard\n");

					if (moreNopsNeeded > nops)
						nops = moreNopsNeeded;

					if (*final == result->first)
						found = true;
				}

				if (!found)
					return;			//scoreboard does not match what is expected
			}

			//add all nops
			for (auto count = 0; count < nops; count++)
				newRunInstructions.push_back(&s_rSpareNop);

			auto size = newRunInstructions.size();

			if (rFoundSchedule)
			{
				if (rBestSchedule.size() < size)
					return;			//not worth it
				else
				{
					found_solutions++;

					rBestSchedule = newRunInstructions;
					return;
				}
			}
			else
			{
				found_solutions++;

				rBestSchedule = newRunInstructions;
				rFoundSchedule = true;
				return;
			}
		}
		else
		{
			//now make a new instructionsToRun
			std::list<DependencyConsumer *> newInstructionsToRun;
			for (auto it = instructionsToRun.begin(); it != instructionsToRun.end(); it++)
				if (*it != *inst)
					newInstructionsToRun.push_back(*it);

			//not worth pursuing
			if (rFoundSchedule && newInstructionsToRun.size() >= rBestSchedule.size())
				return;

			int newDelaySlotsToFill;
			if (isBranch)
				newDelaySlotsToFill = delaySlotsToFill;		//no change, branch is in - three to go
			else if (branchInserted)
				newDelaySlotsToFill = delaySlotsToFill - inserted;		//decrement by instructions inserted
			else
				newDelaySlotsToFill = delaySlotsToFill;		//branch not encountered yet

			Schedule(newRunInstructions, newInstructionsToRun,
					newScoreboard, rFinalScoreboard,
					rBestSchedule, rFoundSchedule,
					branchInserted ? branchInserted : isBranch,
					newDelaySlotsToFill,
					currentCycle + nopsNeeded + 1);
		}
	}
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

	auto start = s_statements.begin();
	bool reordering = true;

	do
	{
		std::list<DependencyBase *> finalScoreboard;
		auto next_start = BuildDeps(start, finalScoreboard, reordering);
		EmitNonInstructions(start, next_start);

		std::list<DependencyProvider *> runInstructions;
		std::list<DependencyConsumer *> instructionsToRun;

		std::list<DependencyProvider *> bestSchedule;
		bool foundSchedule = false;

		for (auto it = start; it != next_start; it++)
		{
			DependencyConsumer *i = dynamic_cast<DependencyConsumer *>(*it);
			if (i)
				instructionsToRun.push_back(i);
		}

		if (instructionsToRun.size() != 0)
		{
			bool lastIsBranch = dynamic_cast<BranchInstruction *>(instructionsToRun.back()) ? true : false;

			//debug
			schedules_run = 0;
			found_solutions = 0;

			std::map<DependencyBase *, int> scoreboard;
			Schedule(runInstructions, instructionsToRun,
					scoreboard, finalScoreboard,
					bestSchedule, foundSchedule,
					false, lastIsBranch ? 3 : 0,
					0);

			assert(foundSchedule);

			for (auto it = bestSchedule.begin(); it != bestSchedule.end(); it++)
			{
				Base *p = dynamic_cast<Base *>(*it);
				assert(p);

				p->DebugPrint(0);
				s_statementsRescheduled.push_back(p);
			}

			printf("sequence of %d instructions run in %d cycles, took %d its with %d working solutions\n", instructionsToRun.size(), bestSchedule.size(), schedules_run, found_solutions);
		}

		start = next_start;
	} while (start != s_statements.end());

	unsigned int address = baseAddress;

	//walk it once, to get addresses and fill in labels
	for (auto it = s_statementsRescheduled.begin(); it != s_statementsRescheduled.end(); it++)
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
	for (auto it = s_statementsRescheduled.begin(); it != s_statementsRescheduled.end(); it++)
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
			{
				Label *l = dynamic_cast<Label *>(*it);
				assert(l);
				printf("/*%s:*/\n", l->GetName());
				break;
			}
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

