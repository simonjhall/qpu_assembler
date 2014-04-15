/*
 * InstructionTree.cpp
 *
 *  Created on: 12 Apr 2014
 *      Author: simon
 */

#include "InstructionTree.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

Base::Base()
{
}

Base::~Base()
{
}

Value::Value(int v)
: m_value(v),
  m_num(0),
  m_denom(0),
  m_byteCount(0)
{
}

Value::Value(int numerator, int denominator)
: m_value(0),
  m_num(numerator),
  m_denom(denominator),
  m_byteCount(0)
{
}

int Value::GetIntValue(void)
{
	assert(m_denom == 0);
	return m_value;
}

void Value::SetSize(int byteCount)
{
	assert(m_denom == 0 || byteCount == 4);
	m_byteCount = byteCount;
}

Instruction::Instruction()
{
}

Instruction::~Instruction()
{
}

Register::Register(Location loc, int id)
: m_loc(loc),
  m_id(id)
{
	if (m_loc == kAcc)
		assert(m_id < 6);
	else
		assert(m_id < 63);
}

Register::~Register()
{
}

Opcode::Opcode(AddOp op)
: m_leftOp(op),
  m_leftType(true)
{
}

Opcode::Opcode(MulOp op)
: m_rightOp(op),
  m_leftType(false)
{
}

BasePipeInstruction::BasePipeInstruction(Opcode& rOp, Register& rDest,
		Register& rSource1, SecondSource& rSource2, InstructionCondition &rCondition, bool setFlags)
: m_rOpcode(rOp),
  m_rDest(rDest),
  m_rSource1(rSource1),
  m_rSource2(rSource2),
  m_rCondition(rCondition),
  m_setFlags(setFlags)
{
}

BasePipeInstruction::~BasePipeInstruction()
{
}

AddPipeInstruction::AddPipeInstruction(Opcode& rOp, Register& rDest,
		Register& rSource1, SecondSource& rSource2, InstructionCondition &rCondition, bool setFlags)
: BasePipeInstruction(rOp, rDest, rSource1, rSource2, rCondition, setFlags)
{
}

AddPipeInstruction::~AddPipeInstruction()
{
}

MulPipeInstruction::MulPipeInstruction(Opcode& rOp, Register& rDest,
		Register& rSource1, SecondSource& rSource2, InstructionCondition &rCondition, bool setFlags)
: BasePipeInstruction(rOp, rDest, rSource1, rSource2, rCondition, setFlags)
{
}

MulPipeInstruction::~MulPipeInstruction()
{
}

AluSignal::AluSignal(Signal sig)
: m_signal(sig)
{
}


AluInstruction::AluInstruction(AddPipeInstruction& rLeft,
		MulPipeInstruction& rRight, AluSignal& rSignal)
: m_rLeft(rLeft),
  m_rRight(rRight),
  m_rSignal(rSignal)
{
	assert(BasePipeInstruction::AreCompatible(rLeft, rRight, rSignal));
}

AluInstruction::~AluInstruction()
{
}

Value::~Value()
{
}

Opcode::~Opcode()
{
}

AluSignal::~AluSignal()
{
}

MulPipeInstruction& BasePipeInstruction::GenerateCompatibleInstruction(
		AddPipeInstruction& rValid)
{
	return *new MulPipeInstruction(*new Opcode(kMulNop), rValid.m_rDest, rValid.m_rSource1, rValid.m_rSource2,
			*new InstructionCondition(kAlways), false);
}

AddPipeInstruction& BasePipeInstruction::GenerateCompatibleInstruction(
		MulPipeInstruction& rValid)
{
	return *new AddPipeInstruction(*new Opcode(kAddNop), rValid.m_rDest, rValid.m_rSource1, rValid.m_rSource2,
			*new InstructionCondition(kAlways), false);
}

AluSignal& AluSignal::DefaultSignal(void)
{
	return *new AluSignal(kNoSignal);
}

bool BasePipeInstruction::AreCompatible(AddPipeInstruction& rLeft,
		MulPipeInstruction& rRight, AluSignal &rSignal)
{
	return true;
}

SecondSource::SecondSource()
{
}

SecondSource::~SecondSource()
{
}

SmallImm::SmallImm(Value& rValue)
: m_rValue(rValue)
{
}

SmallImm::~SmallImm()
{
}

InstructionCondition::InstructionCondition(ConditionCode cc)
: m_condition(cc)
{
}

InstructionCondition::~InstructionCondition()
{
}

BrCondition::BrCondition(BranchCondition cc)
: m_condition(cc)
{
}

BrCondition::~BrCondition()
{
}

IlInstruction::IlInstruction(Register& rDest, Value& rImmediate,
		InstructionCondition& rCc, bool setFlags)
: m_rDest(rDest),
  m_rImmediate(rImmediate),
  m_rCondition(rCc),
  m_setFlags(setFlags)
{
	m_rImmediate.SetSize(4);
}

IlInstruction::~IlInstruction()
{
}

SemInstruction::SemInstruction(Value& rSemId, Operation operation)
: m_rSemId(rSemId),
  m_op(operation)
{
}

SemInstruction::~SemInstruction()
{
}

BranchInstruction::BranchInstruction(bool absolute, BrCondition& rCond,
		Register& rDestA, Register& rDestM, Register& rSource, Value& rImm)
: m_absolute(absolute),
  m_rCondition(rCond),
  m_rDestA(rDestA),
  m_rDestM(rDestM),
  m_pSource(&rSource),
  m_rImm(rImm)
{
}

BranchInstruction::BranchInstruction(bool absolute, BrCondition& rCond,
		Register& rDestA, Register& rDestM, Value& rImm)
: m_absolute(absolute),
  m_rCondition(rCond),
  m_rDestA(rDestA),
  m_rDestM(rDestM),
  m_pSource(0),
  m_rImm(rImm)
{
}

BranchInstruction::~BranchInstruction()
{
}

Label::Label(const char* pName, bool definition)
: Value(0)
{
	if (definition)
	{
		strncpy(m_name, pName, sm_maxLengthIncNull);
		//null terminate
		m_name[sm_maxLengthIncNull - 1] = 0;

		//find the colon and replace with a null
		for (int count = 0; count < sm_maxLengthIncNull; count++)
			if (m_name[count] == ':')
			{
				m_name[count] = 0;
				break;
			}

		if (strlen(pName) + 1 > sm_maxLengthIncNull)
			printf("label %s exceeds max length, truncating\n", pName);
	}
	else
	{
		strncpy(m_name, pName + 1, sm_maxLengthIncNull);
		//null terminate
		m_name[sm_maxLengthIncNull - 1] = 0;

		//find the second hash and replace with a null
		for (int count = 0; count < sm_maxLengthIncNull; count++)
			if (m_name[count] == '#')
			{
				m_name[count] = 0;
				break;
			}

		if (strlen(pName) > sm_maxLengthIncNull)
			printf("label %s exceeds max length, truncating\n", pName);
	}
}

Label::~Label()
{
}

