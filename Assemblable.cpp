/*
 * Assemblable.cpp
 *
 *  Created on: 15 Apr 2014
 *      Author: simon
 */

#include "InstructionTree.h"
#include <stdio.h>
#include <assert.h>

Assemblable::Assemblable()
{
}

Assemblable::~Assemblable()
{
}

void Assemblable::Assemble(Fields& rFields)
{
	printf("ASSEMBLING UKNOWN TYPE\n");
	assert(0);
}

unsigned int Assemblable::GetEncodedValue(void)
{
	printf("ENCODING UKNOWN TYPE\n");
	assert(0);
	return 0;
}

void AluInstruction::Assemble(Fields& rFields)
{
	m_rLeft.Assemble(rFields);
	m_rRight.Assemble(rFields);

	rFields.push_back(Field(60, 15, m_rSignal.GetEncodedValue()));
}

void BasePipeInstruction::AssembleAs(Fields& rFields, bool aluPipe)
{
	rFields.push_back(Field(45, 1, (unsigned int)m_setFlags));
	m_rOpcode.Assemble(rFields);

	if (aluPipe)
	{
		rFields.push_back(Field(38, 63, m_rDest.GetId()));

		if (m_rDest.GetLocation() != Register::kAcc)			//accumulators are the same in ra/rb
			rFields.push_back(Field(44, 1,
					m_rDest.GetLocation() == Register::kRa ? 0 : 1));

		if (m_rSource1.GetLocation() == Register::kAcc)
			rFields.push_back(Field(9, 7, m_rSource1.GetId()));
		else
		{
			rFields.push_back(Field(18, 63, m_rSource1.GetId()));

			if (m_rSource1.GetLocation() == Register::kRa)
				rFields.push_back(Field(9, 7, (unsigned int)MuxEncoding::kRfA));
			else if (m_rSource1.GetLocation() == Register::kRb)
				rFields.push_back(Field(9, 7, (unsigned int)MuxEncoding::kRfB));
			else
				assert(0);
		}

		//see if it's a register or small immediate
		Register *r = dynamic_cast<Register *>(&m_rSource2);
		SmallImm *i = dynamic_cast<SmallImm *>(&m_rSource2);

		if (r && !i)
		{
			rFields.push_back(Field(12, 63, r->GetId()));

			if (r->GetLocation() == Register::kRa)
				rFields.push_back(Field(9, 7, (unsigned int)MuxEncoding::kRfA));
			else if (r->GetLocation() == Register::kRb)
				rFields.push_back(Field(9, 7, (unsigned int)MuxEncoding::kRfB));
			else
				assert(0);
		}
		else if (!r && i)
		{
			//the value
			rFields.push_back(Field(12, 63, i->GetEncodedValue()));
			//the small imm signal
			rFields.push_back(Field(60, 15, kSmallImmOrVecRot));
		}
		else
			assert(0);

		rFields.push_back(Field(49, 7, m_rCondition.GetEncodedValue()));
	}
	else
	{
	}
}

void AddPipeInstruction::Assemble(Fields& rFields)
{
	AssembleAs(rFields, true);
}

void MulPipeInstruction::Assemble(Fields& rFields)
{
	AssembleAs(rFields, false);
}

unsigned int AluSignal::GetEncodedValue(void)
{
	return (unsigned int)m_signal;
}

void Opcode::Assemble(Fields &rFields)
{
	if (m_leftType)
		rFields.push_back(Field(24, 31, m_leftOp));
	else
		rFields.push_back(Field(29, 7, m_rightOp));
}

unsigned int SmallImm::GetEncodedValue(void)
{
	if (m_rValue.m_denom == 1)
	{
		switch(m_rValue.m_num)
		{
		case 1: return 32;
		case 2: return 33;
		case 4: return 34;
		case 8: return 35;
		case 16: return 36;
		case 32: return 37;
		case 64: return 38;
		case 128: return 39;
		default: assert(0);
		}
	}
	else if (m_rValue.m_denom == 0)
	{
		if (m_rValue.m_value >= 0 && m_rValue.m_value < 16)
			return m_rValue.m_value;
		else if (m_rValue.m_value >= -16 && m_rValue.m_value < 0)
			return m_rValue.m_value;
		else
			assert(0);
	}
	else if (m_rValue.m_num == 1)
	{
		switch(m_rValue.m_denom)
		{
		case 2: return 47;
		case 4: return 46;
		case 8: return 45;
		case 16: return 44;
		case 32: return 43;
		case 64: return 42;
		case 128: return 41;
		case 256: return 40;
		default: assert(0);
		}
	}
	else
		assert(0);

	//keep the compiler happy
	return 0;
}

unsigned int InstructionCondition::GetEncodedValue(void)
{
	return (unsigned int)m_condition;
}
