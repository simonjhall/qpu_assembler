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

extern std::list<Label *> s_declaredLabels;
extern std::list<Label *> s_usedLabels;


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
  m_byteCount(0),
  m_isFloat(false)
{
}

Value::Value(int numerator, int denominator)
: m_value(0),
  m_num(numerator),
  m_denom(denominator),
  m_byteCount(0),
  m_isFloat(false)
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

	if (m_isFloat)
		assert(m_byteCount == 4);
}

void Value::SetAsFloat(void)
{
	m_isFloat = true;
	SetSize(4);
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
		assert(m_id < 64);
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
		MulPipeInstruction& rRight, AluSignal& rSignal,
		bool force)
: m_rLeft(rLeft),
  m_rRight(rRight),
  m_rSignal(rSignal)
{
	if (!force)
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
	return *new MulPipeInstruction(*new Opcode(kMulNop), *new Register(Register::kAcc, 0), rValid.m_rSource1, rValid.m_rSource2,
			*new InstructionCondition(kNever), false);
}

AddPipeInstruction& BasePipeInstruction::GenerateCompatibleInstruction(
		MulPipeInstruction& rValid)
{
	return *new AddPipeInstruction(*new Opcode(kAddNop), *new Register(Register::kAcc, 0), rValid.m_rSource1, rValid.m_rSource2,
			*new InstructionCondition(kNever), false);
}

AluSignal& AluSignal::DefaultSignal(void)
{
	return *new AluSignal(kNoSignal);
}

bool BasePipeInstruction::AreCompatible(AddPipeInstruction& rLeft,
		MulPipeInstruction& rRight, AluSignal &rSignal)
{
	AluInstruction i(rLeft, rRight, rSignal, true);

	Fields f;
	i.Assemble(f);

	unsigned int sizeInBytes;
	uint64_t output;
	return Assemblable::CombineFields(f, sizeInBytes, output);
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
: Value(0),
  m_pDeclaredLabel(0)
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

		s_declaredLabels.push_back(this);
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

		s_usedLabels.push_back(this);
	}
}

Label::~Label()
{
}

RaRbDependency::RaRbDependency(Register &rReg, DependencyProvider *pProvider)
: DependencyWithoutInterlock(2, false, *new RegisterDependee(rReg), pProvider),
  m_rReg(rReg)
{
	assert(rReg.GetLocation() != Register::kAcc);
}

AccDependency::AccDependency(Register &rReg, DependencyProvider *pProvider)
: DependencyWithoutInterlock(1, false, *new RegisterDependee(rReg), pProvider),
  m_rReg(rReg)
{
	assert(rReg.GetLocation() == Register::kAcc);
}

void Instruction::GetOutputDeps(DependencyBase::Dependencies &rDeps)
{
	//should be just sem
	assert(dynamic_cast<SemInstruction *>(this));
	rDeps.clear();
}

void Instruction::GetInputDeps(Dependee::Dependencies &rDeps)
{
	rDeps.clear();
}

void Instruction::AddInputDep(DependencyBase& rDep)
{
	m_deps.push_back(&rDep);
}

void AluInstruction::GetOutputDeps(DependencyBase::Dependencies &rDeps)
{
	rDeps.clear();

	Register *pDestA = m_rLeft.GetDestReg(true);
	Register *pDestM = m_rRight.GetDestReg(true);

	if (pDestA)
	{
		if (pDestA->GetLocation() == Register::kAcc)
			rDeps.push_back(new AccDependency(*pDestA, this));
		else
		{
			assert(pDestA->GetLocation() == Register::kRa || pDestA->GetLocation() == Register::kRb);
			rDeps.push_back(new RaRbDependency(*pDestA, this));
		}
	}

	if (pDestM)
	{
		if (pDestM->GetLocation() == Register::kAcc)
			rDeps.push_back(new AccDependency(*pDestM, this));
		else
		{
			assert(pDestM->GetLocation() == Register::kRa || pDestM->GetLocation() == Register::kRb);
			rDeps.push_back(new RaRbDependency(*pDestM, this));
		}
	}

	if (m_rLeft.GetSetsFlags() || m_rRight.GetSetsFlags())
		rDeps.push_back(new FlagsDependency(this));
}

void IlInstruction::GetOutputDeps(DependencyBase::Dependencies &rDeps)
{
	rDeps.clear();

	if (m_rDest.GetLocation() == Register::kAcc)
		rDeps.push_back(new AccDependency(m_rDest, this));
	else
	{
		assert(m_rDest.GetLocation() == Register::kRa || m_rDest.GetLocation() == Register::kRb);
		rDeps.push_back(new RaRbDependency(m_rDest, this));
	}

	if (m_setFlags)
		rDeps.push_back(new FlagsDependency(this));
}

void BranchInstruction::GetOutputDeps(DependencyBase::Dependencies &rDeps)
{
	rDeps.clear();

	assert(m_rDestA.GetLocation() == Register::kRa || m_rDestA.GetLocation() == Register::kRb);
	assert(m_rDestM.GetLocation() == Register::kRa || m_rDestM.GetLocation() == Register::kRb);

	rDeps.push_back(new RaRbDependency(m_rDestA, this));
	rDeps.push_back(new RaRbDependency(m_rDestM, this));
}

ReorderControl::ReorderControl(bool begin)
: Base(),
  m_begin(begin)
{
}

bool ReorderControl::IsBegin(void)
{
	return m_begin;
}

ReorderControl::~ReorderControl()
{
}

bool ReorderControl::IsEnd(void)
{
	return !m_begin;
}

void AluInstruction::GetInputDeps(Dependee::Dependencies &rDeps)
{
	rDeps.clear();

	Register *pSourceA1 = m_rLeft.GetSourceRegA(true);
	Register *pSourceA2 = m_rLeft.GetSourceRegB(true);

	Register *pSourceM1 = m_rRight.GetSourceRegA(true);
	Register *pSourceM2 = m_rRight.GetSourceRegB(true);

	if (pSourceA1)
		rDeps.push_back(new RegisterDependee(*pSourceA1));
	if (pSourceA2)
		rDeps.push_back(new RegisterDependee(*pSourceA2));
	if (pSourceM1)
		rDeps.push_back(new RegisterDependee(*pSourceM1));
	if (pSourceM2)
		rDeps.push_back(new RegisterDependee(*pSourceM2));

	if (m_rLeft.GetUsesFlags() || m_rRight.GetUsesFlags())
		rDeps.push_back(new FlagsDependee());
}

void BranchInstruction::GetInputDeps(Dependee::Dependencies &rDeps)
{
	rDeps.clear();

	if (m_pSource)
	{
		assert(m_pSource->GetLocation() == Register::kRa);
		rDeps.push_back(new RegisterDependee(*m_pSource));
	}

	if (m_rCondition.GetEncodedValue() != kAlwaysBr)
		rDeps.push_back(new FlagsDependee());
}

RegisterDependee::RegisterDependee(Register& rReg)
: m_rReg(rReg)
{
}

bool RegisterDependee::SatisfiesThis(DependencyBase& rDep)
{
	AccDependency *a = dynamic_cast<AccDependency *>(&rDep);

	if (a && a->GetReg().GetLocation() == m_rReg.GetLocation() && a->GetReg().GetId() == m_rReg.GetId())
		return true;

	RaRbDependency *r = dynamic_cast<RaRbDependency *>(&rDep);

	if (r && r->GetReg().GetLocation() == m_rReg.GetLocation() && r->GetReg().GetId() == m_rReg.GetId())
		return true;

	return false;
}

Register& RaRbDependency::GetReg()
{
	return m_rReg;
}

Register& AccDependency::GetReg()
{
	return m_rReg;
}

bool RaRbDependency::ProvidesSameThing(DependencyBase &rOther)
{
	RaRbDependency *pOther = dynamic_cast<RaRbDependency *>(&rOther);

	if (pOther)
	{
		if (pOther->GetReg().GetLocation() == GetReg().GetLocation()
				&& pOther->GetReg().GetId() == GetReg().GetId())
			return true;
	}

	return false;
}

bool AccDependency::ProvidesSameThing(DependencyBase &rOther)
{
	AccDependency *pOther = dynamic_cast<AccDependency *>(&rOther);

	if (pOther)
	{
		assert(pOther->GetReg().GetLocation() == Register::kAcc);

		if (pOther->GetReg().GetId() == GetReg().GetId())
			return true;
	}

	return false;
}

FlagsDependency::FlagsDependency(DependencyProvider* pProvider)
: DependencyWithoutInterlock(1, false, *new FlagsDependee, pProvider)
{
}

bool FlagsDependency::ProvidesSameThing(DependencyBase &rOther)
{
	FlagsDependency *pOther = dynamic_cast<FlagsDependency *>(&rOther);

	if (pOther)
		return true;
	else
		return false;
}

FlagsDependee::FlagsDependee(void)
: Dependee()
{
}

bool FlagsDependee::SatisfiesThis(DependencyBase& rDep)
{
	FlagsDependency *a = dynamic_cast<FlagsDependency *>(&rDep);

	if (a)
		return true;
	else
		return false;
}
