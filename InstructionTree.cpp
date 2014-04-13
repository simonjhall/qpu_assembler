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

static const char *pSignalNames[16] = {
		"Breakpoint",
		"NoSignal",
		"ThreadSwitch",
		"ProgramEnd",
		"ScoreboardWait",
		"ScoreboardUnlock",
		"LastThreadSwitch",
		"CoverageLoad",
		"ColourLoad",
		"ColourLoadPe",
		"LoadTmu0",
		"LoadTmu1",
		"AlphaMaskLoad",
		"SmallImmOrVecRot",
		"LoadImm",
		"Branch",
};

static const char *pAddOpNames[32] = {
		"addnop",
		"fadd",
		"fsub",
		"fmin",
		"fmax",
		"fminabs",
		"fmaxabs",
		"ftoi",
		"itof",
		"reserved",
		"reserved",
		"reserved",
		"add",
		"sub",
		"shr",
		"asr",
		"ror",
		"shl",
		"min",
		"max",
		"and",
		"or",
		"xor",
		"not",
		"clz",
		"reserved",
		"reserved",
		"reserved",
		"reserved",
		"reserved",
		"addv8adds",
		"addv8subs",
};

static const char *pMulOpNames[8] = {
		"mulnop",
		"fmul",
		"mul24",
		"v8muld",
		"v8min",
		"v8max",
		"mulv8adds",
		"mulv8subs",
};

static const char *pConditionCodeNames[8] = {
		"never",
		"al",
		"zs",
		"zc",
		"ns",
		"nc",
		"cs",
		"cc",
};

static const char *pBranchConditionNames[16] = {
	"allzs",
	"allzc",

	"anyzs",
	"anyzc",

	"allns",
	"allnc",

	"anyns",
	"anync",

	"allcs",
	"allcc",

	"anycs",
	"anycc",

	"RESERVED",
	"RESERVED",
	"RESERVED",

	""
};

Base::Base()
{
}

Base::~Base()
{
}

void Base::DebugPrint(int depth)
{
	printf("BASE DEBUG PRINT\n");
}

Value::Value(int v)
: m_value(v),
  m_byteCount(0)
{
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

void Register::DebugPrint(int depth)
{
	for (int count = 0; count < depth; count++)
		printf("\t");

	printf("%s%d\n",
			m_loc == kRa ? "ra" : (m_loc == kRb ? "rb" : "acc"),
			m_id);
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

void BasePipeInstruction::DebugPrint(int depth)
{
	for (int count = 0; count < depth; count++)
		printf("\t");

	printf("base pipe instruction: set flags: %s\n", m_setFlags ? "TRUE" : "FALSE");

	m_rOpcode.DebugPrint(depth + 1);
	m_rDest.DebugPrint(depth + 1);
	m_rSource1.DebugPrint(depth + 1);
	m_rSource2.DebugPrint(depth + 1);
	m_rCondition.DebugPrint(depth + 1);
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

void Value::DebugPrint(int depth)
{
	for (int count = 0; count < depth; count++)
		printf("\t");
	printf("value: %d, size: %d\n", m_value, m_byteCount);
}

void Opcode::DebugPrint(int depth)
{
	for (int count = 0; count < depth; count++)
		printf("\t");

	if (m_leftType)
		printf("opcode: %s\n", pAddOpNames[m_leftOp]);
	else
		printf("opcode: %s\n", pMulOpNames[m_rightOp]);
}

void AluSignal::DebugPrint(int depth)
{
	for (int count = 0; count < depth; count++)
		printf("\t");

	printf("signal: %s\n", pSignalNames[m_signal]);
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

void AluInstruction::DebugPrint(int depth)
{
	for (int count = 0; count < depth; count++)
		printf("\t");

	printf("alu instruction\n");
	m_rLeft.DebugPrint(depth + 1);
	m_rRight.DebugPrint(depth + 1);
	m_rSignal.DebugPrint(depth + 1);
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

void SmallImm::DebugPrint(int depth)
{
	for (int count = 0; count < depth; count++)
		printf("\t");

	printf("small imm\n");
	m_rValue.DebugPrint(depth + 1);
}

InstructionCondition::InstructionCondition(ConditionCode cc)
: m_condition(cc)
{
}

InstructionCondition::~InstructionCondition()
{
}

void InstructionCondition::DebugPrint(int depth)
{
	for (int count = 0; count < depth; count++)
		printf("\t");

	printf("condition: %s\n", pConditionCodeNames[m_condition]);
}

BrCondition::BrCondition(BranchCondition cc)
: m_condition(cc)
{
}

BrCondition::~BrCondition()
{
}

void BrCondition::DebugPrint(int depth)
{
	for (int count = 0; count < depth; count++)
		printf("\t");

	printf("condition: %s\n", pBranchConditionNames[m_condition]);
}

IlInstruction::IlInstruction(Register& rDest, Value& rImmediate,
		InstructionCondition& rCc, bool setFlags)
: m_rDest(rDest),
  m_rImmediate(rImmediate),
  m_rCondition(rCc),
  m_setFlags(setFlags)
{
}

IlInstruction::~IlInstruction()
{
}

void IlInstruction::DebugPrint(int depth)
{
	for (int count = 0; count < depth; count++)
		printf("\t");

	printf("immediate load: set flags: %s\n", m_setFlags ? "TRUE" : "FALSE");
	m_rDest.DebugPrint(depth + 1);
	m_rImmediate.DebugPrint(depth + 1);
	m_rCondition.DebugPrint(depth + 1);
}

SemInstruction::SemInstruction(Value& rSemId, Operation operation)
: m_rSemId(rSemId),
  m_op(operation)
{
}

SemInstruction::~SemInstruction()
{
}

void SemInstruction::DebugPrint(int depth)
{
	for (int count = 0; count < depth; count++)
		printf("\t");

	printf("semaphore %s\n", m_op == kInc ? "INCREMENT" : "DECREMENT");
	m_rSemId.DebugPrint(depth + 1);
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

void BranchInstruction::DebugPrint(int depth)
{
	for (int count = 0; count < depth; count++)
		printf("\t");

	printf("branch: absolute: %s, uses register: %s\n",
			m_absolute ? "TRUE" : "FALSE", m_pSource ? "TRUE" : "FALSE");

	m_rCondition.DebugPrint(depth + 1);
	m_rDestA.DebugPrint(depth + 1);
	m_rDestM.DebugPrint(depth + 1);

	if (m_pSource)
		m_pSource->DebugPrint(depth + 1);

	m_rImm.DebugPrint(depth + 1);
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

void Label::DebugPrint(int depth)
{
	for (int count = 0; count < depth; count++)
		printf("\t");

	printf("label name: %s\n", m_name);
}

void Value::SetSize(int byteCount)
{
	m_byteCount = byteCount;
}
