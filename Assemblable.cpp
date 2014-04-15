/*
 * Assemblable.cpp
 *
 *  Created on: 15 Apr 2014
 *      Author: simon
 */

#include "InstructionTree.h"
#include <stdio.h>

Assemblable::Assemblable()
{
}

Assemblable::~Assemblable()
{
}

void Assemblable::Assemble(Fields& rFields)
{
	printf("ASSEMBLING UKNOWN TYPE\n");
}


void AluInstruction::Assemble(Fields& rFields)
{
}
