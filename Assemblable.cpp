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

void Instruction::Assemble(Fields& rFields)
{
	rFields.push_back(Field(8));
}

void BranchInstruction::Assemble(Fields& rFields)
{
	Instruction::Assemble(rFields);

	//branch signal
	rFields.push_back(Field(57, 127, 120));

	//branch condition
	rFields.push_back(Field(52, 15, m_rCondition.GetEncodedValue()));

	//rel
	rFields.push_back(Field(51, 1, (unsigned int)!m_absolute));

	if (m_pSource)
	{
		//reg
		rFields.push_back(Field(50, 1, 1));

		assert(m_pSource->GetLocation() == Register::kRa);
		assert(m_pSource->GetId() < 32);

		//raddr
		rFields.push_back(Field(45, 31, m_pSource->GetId()));
	}

	assert(m_rDestA.GetLocation() != Register::kAcc);
	assert(m_rDestM.GetLocation() != Register::kAcc);
	assert(m_rDestA.GetLocation() != m_rDestM.GetLocation());

	if (m_rDestA.GetLocation() == Register::kRb)
		rFields.push_back(Field(44, 1, 1));
	else
		rFields.push_back(Field(44, 1, 0));

	//waddr
	rFields.push_back(Field(38, 63, m_rDestA.GetId()));
	rFields.push_back(Field(32, 63, m_rDestM.GetId()));

	//imm
	rFields.push_back(Field(0, 0xffffffff, (unsigned int)m_rImm.GetIntValue()));
}

void SemInstruction::Assemble(Fields& rFields)
{
	Instruction::Assemble(rFields);

	//sem signal
	rFields.push_back(Field(57, 127, 116));

	//inc/dec
	rFields.push_back(Field(4, 1, m_op == kInc ? 0 : 1));

	//id
	rFields.push_back(Field(0, 15, m_rSemId.GetIntValue()));
}

void IlInstruction::Assemble(Fields& rFields)
{
	Instruction::Assemble(rFields);

	//il signal
	rFields.push_back(Field(57, 127, 112));

	//todo fix this for dual issue
	//cond
	rFields.push_back(Field(49, 7, m_rCondition.GetEncodedValue()));
	rFields.push_back(Field(46, 7, kNever));

	//sf
	rFields.push_back(Field(45, 1, (unsigned int)m_setFlags));

	//ws
	rFields.push_back(Field(44, 1,
			m_rDest.GetLocation() == Register::kRa ? 0 : 1));

	//waddr
	if (m_rDest.GetLocation() != Register::kAcc)
		rFields.push_back(Field(38, 63, m_rDest.GetId()));
	else
		rFields.push_back(Field(38, 63, 32 + m_rDest.GetId()));

	//imm
	rFields.push_back(Field(0, 0xffffffff, (unsigned int)m_rImmediate.GetIntValue()));
}

void AluInstruction::Assemble(Fields& rFields)
{
	Instruction::Assemble(rFields);

	m_rLeft.Assemble(rFields);
	m_rRight.Assemble(rFields);

	//look for a signal already written - it may be a small imm
	bool found = false;
	for (auto it = rFields.begin(); it != rFields.end(); it++)
		if (it->m_position == 60 && it->m_mask == 15 && it->m_value == (unsigned int)kSmallImmOrVecRot)
		{
			found = true;
			break;
		}

	//todo could do better
	if (found && m_rSignal.GetEncodedValue() != AluSignal(kNoSignal).GetEncodedValue())
	{
		printf("warning signal set on instruction which already has small imm/vec rot signal set\n");
		m_rSignal.DebugPrint(0);
	}

	if (!found)
		rFields.push_back(Field(60, 15, m_rSignal.GetEncodedValue()));

	/*
	 * flags can only be set on one of them
	 * if the first element is a nop or never then the flags apply to the second one
	 */

	int flag_counts = 0;		//amount of times we find sf
	int flag_sets = 0;			//amount of times it's == 1

	for (auto it = rFields.begin(); it != rFields.end(); it++)
		if (it->m_position == 45 && it->m_mask == 1)
		{
			if (it->m_value)
				flag_sets++;
			flag_counts++;
		}

	assert(flag_counts <= 2);
	//uh-oh, multiple flag sets
	assert(flag_sets == 0 || flag_sets == 1);

	//remove the zero one
	if (flag_counts == 2 && flag_sets == 1)
		for (auto it = rFields.begin(); it != rFields.end(); it++)
			if (it->m_position == 45 && it->m_mask == 1 && it->m_value == 0)
			{
				rFields.erase(it);
				break;
			}
}

void BasePipeInstruction::AssembleAs(Fields& rFields, bool aluPipe)
{
	int wraddr_offset;
	int muxa_offset, muxb_offset;
	int cond_offset;

	if (aluPipe)
	{
		wraddr_offset = 38;
		muxa_offset = 9;
		muxb_offset = 6;
		cond_offset = 49;
	}
	else
	{
		wraddr_offset = 32;
		muxa_offset = 3;
		muxb_offset = 0;
		cond_offset = 46;
	}

	if (aluPipe && m_setFlags && IsNopOrNever())
		assert(0);

	//sf
	if (m_setFlags)
		rFields.push_back(Field(45, 1, (unsigned int)m_setFlags));

	//opcode
	m_rOpcode.Assemble(rFields);

	//ws
	if (m_rDest.GetLocation() != Register::kAcc)			//accumulators are the same in ra/rb
	{
		//wraddr
		rFields.push_back(Field(wraddr_offset, 63, m_rDest.GetId()));

		if (aluPipe)
			rFields.push_back(Field(44, 1,
					m_rDest.GetLocation() == Register::kRa ? 0 : 1));
		else
			rFields.push_back(Field(44, 1,
					m_rDest.GetLocation() == Register::kRa ? 1 : 0));
	}
	else
		rFields.push_back(Field(wraddr_offset, 63, 32 + m_rDest.GetId()));

	//mux a
	if (m_rSource1.GetLocation() == Register::kRa)
	{
		rFields.push_back(Field(muxa_offset, 7, (unsigned int)MuxEncoding::kRfA));
		rFields.push_back(Field(18, 63, m_rSource1.GetId()));				//raddra
	}
	else if (m_rSource1.GetLocation() == Register::kRb)
	{
		rFields.push_back(Field(muxa_offset, 7, (unsigned int)MuxEncoding::kRfB));
		rFields.push_back(Field(12, 63, m_rSource1.GetId()));				//raddrb
	}
	else if (m_rSource1.GetLocation() == Register::kAcc)
		rFields.push_back(Field(muxa_offset, 7, m_rSource1.GetId()));			//mux a
	else
		assert(0);

	//see if it's a register or small immediate
	Register *r = dynamic_cast<Register *>(&m_rSource2);
	SmallImm *i = dynamic_cast<SmallImm *>(&m_rSource2);

	if (r && !i)
	{
		if (r->GetLocation() == Register::kRa)
		{
			rFields.push_back(Field(muxb_offset, 7, (unsigned int)MuxEncoding::kRfA));
			rFields.push_back(Field(18, 63, r->GetId()));		//raddra
		}
		else if (r->GetLocation() == Register::kRb)
		{
			rFields.push_back(Field(muxb_offset, 7, (unsigned int)MuxEncoding::kRfB));
			rFields.push_back(Field(12, 63, r->GetId()));		//raddrb
		}
		else if (r->GetLocation() == Register::kAcc)
			rFields.push_back(Field(muxb_offset, 7, r->GetId()));
		else
			assert(0);
	}
	else if (!r && i)
	{
		//the value (small immed)
		rFields.push_back(Field(12, 63, i->GetEncodedValue()));
		//the small imm signal
		rFields.push_back(Field(60, 15, kSmallImmOrVecRot));
		//and we want to use 'b'
		rFields.push_back(Field(muxb_offset, 7, (unsigned int)MuxEncoding::kRfB));
	}
	else
		assert(0);

	//cond
	rFields.push_back(Field(cond_offset, 7, m_rCondition.GetEncodedValue()));
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
		rFields.push_back(Field(24, 31, (unsigned int)m_leftOp));
	else
		rFields.push_back(Field(29, 7, (unsigned int)m_rightOp));
}

unsigned int Opcode::GetEncodedValue(void)
{
	if (m_leftType)
		return (unsigned int)m_leftOp;
	else
		return (unsigned int)m_rightOp;
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

bool Assemblable::CombineFields(Fields &rFields, unsigned int &rSizeInBytes, uint64_t &rOutput)
{
	unsigned int totalSize = 0;
	//find the maximum size of the output
	for (auto it = rFields.begin(); it != rFields.end(); it++)
		if (it->m_totalSize)
		{
			//there should be only one
			if (totalSize != 0)
				return false;
			totalSize = it->m_totalSize;
		}

	assert(totalSize);

	uint64_t output = 0;
	uint64_t mask = 0;

	//walk element and commit to the output;
	for (auto it = rFields.begin(); it != rFields.end(); it++)
	{
		if (it->m_totalSize)
			continue;

//		printf("field: pos %ld mask %lx value %ld\n", it->m_position, it->m_mask, it->m_value);

		uint64_t new_mask = mask | (it->m_mask << it->m_position);
		uint64_t new_output = (output & ~(it->m_mask << it->m_position)) | ((it->m_mask & it->m_value) << it->m_position);

		if (new_mask == mask)
		{
			//uh-oh, overlap
			//hopefully there is no change
			if (new_output != output)
				return false;

			mask = new_mask;
		}
		else
		{
			//new data
			mask = new_mask;
			output = (output & ~(it->m_mask << it->m_position)) | ((it->m_mask & it->m_value) << it->m_position);
		}
	}

	//chop out anything over the max size
	if (totalSize != 8)
	{
		uint64_t masked = output & (((uint64_t)1 << (uint64_t)(totalSize * 8)) - 1);
		if (masked != output)
			return false;
	}

	rSizeInBytes = totalSize;
	rOutput = output;
	return true;
}

unsigned int BrCondition::GetEncodedValue(void)
{
	return (unsigned int)m_condition;
}

void Value::Assemble(Fields& rFields)
{
	assert(m_byteCount);
	assert(m_denom == 0);

	switch (m_byteCount)
	{
	case 1:
		rFields.push_back(Field(0, 0xff, m_value));
		break;
	case 2:
		rFields.push_back(Field(0, 0xffff, m_value));
		break;
	case 4:
		rFields.push_back(Field(0, 0xffffffff, m_value));
		break;
	default:
		assert(0);
	}

	rFields.push_back(Field(m_byteCount));
}
