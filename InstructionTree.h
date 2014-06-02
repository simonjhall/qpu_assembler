/*
 * InstructionTree.h
 *
 *  Created on: 12 Apr 2014
 *      Author: simon
 */

#ifndef INSTRUCTIONTREE_H_
#define INSTRUCTIONTREE_H_

#include "shared.h"

#include <list>
#include <map>
#include <functional>
#include <stdint.h>

class Base
{
public:
	virtual void DebugPrint(int depth);

protected:
	Base();
	virtual ~Base();
};

class ReorderControl : public Base
{
public:
	ReorderControl(bool begin);
	virtual ~ReorderControl();

	bool IsBegin(void);
	bool IsEnd(void);

	virtual void DebugPrint(int depth);

private:
	bool m_begin;
};

class Assemblable
{
public:
	Assemblable();
	virtual ~Assemblable();

	struct Field
	{
		inline Field(unsigned int position, unsigned int mask, unsigned int value)
		: m_position(position),
		  m_mask(mask),
		  m_value(value),
		  m_totalSize(0)
		{
		};

		inline Field(unsigned int size)
		: m_position(0),
		  m_mask(0),
		  m_value(0),
		  m_totalSize(size)
		{
		};

		uint64_t m_position;
		uint64_t m_mask;
		uint64_t m_value;
		unsigned int m_totalSize;
	};

	typedef std::list<Field> Fields;

	virtual void Assemble(Fields &rFields);
	virtual unsigned int GetEncodedValue(void);

	virtual void DebugPrintAsm(void);

	static bool CombineFields(Fields &rFields, unsigned int &rSizeInBytes, uint64_t &rOutput);
};

class InstructionCondition : public Base, public Assemblable
{
public:
	InstructionCondition(ConditionCode);
	virtual ~InstructionCondition();

	virtual void DebugPrint(int depth);
	virtual unsigned int GetEncodedValue(void);

	virtual void DebugPrintAsm(void);

private:
	ConditionCode m_condition;
};

class BrCondition : public Base, public Assemblable
{
public:
	BrCondition(BranchCondition);
	virtual ~BrCondition();

	virtual void DebugPrint(int depth);
	virtual unsigned int GetEncodedValue(void);

	virtual void DebugPrintAsm(void);

private:
	BranchCondition m_condition;
};

class Value : public Base, public Assemblable
{
	friend class SmallImm;
public:
	Value(int);
	Value(int numerator, int denominator);
	virtual ~Value();

	virtual void DebugPrint(int depth);
	virtual void SetSize(int byteCount);

	virtual int GetIntValue(void);

	virtual void Assemble(Fields &rFields);

	void SetAsFloat(void);

protected:
	int m_value;
	int m_num, m_denom;

	int m_byteCount;
	bool m_isFloat;
};

class SecondSource : public Base
{
public:
	SecondSource();
	virtual ~SecondSource();

	virtual void DebugPrintAsm(void) = 0;

private:
};

class Register : public SecondSource
{
public:
	enum Location
	{
		kRa,
		kRb,
		kAcc,
	};
	Register(Location, int);
	virtual ~Register();
	virtual void DebugPrint(int depth);

	virtual void DebugPrintAsm(void);
	void DebugPrintAsm(bool reading);

	inline Location GetLocation(void) { return m_loc; }
	inline int GetId(void) { return m_id; }

	typedef std::list<Register> Registers;

	inline bool IsZero(void)
	{
		if (m_loc != kAcc && m_id == 39)
			return true;
		else
			return false;
	}

	inline bool operator <(const Register &rhs) const
	{
		if (m_loc == rhs.m_loc)
			return m_id > rhs.m_id;
		else
			return (int)m_loc > (int)rhs.m_loc;
	}

private:
	Location m_loc;
	int m_id;
};

class DependencyBase;

class Dependee
{
public:
	virtual bool SatisfiesThis(DependencyBase &rDep) = 0;
	virtual void DebugPrint(int depth) const = 0;
	virtual DependencyBase &CreateDummySatisficer(void) = 0;

	typedef std::list<Dependee *> Dependencies;
protected:
	virtual ~Dependee() {};
};

class RegisterDependee : public Dependee
{
public:
	RegisterDependee(Register &rReg);
	virtual ~RegisterDependee() {};

	virtual bool SatisfiesThis(DependencyBase &rDep);
	virtual void DebugPrint(int depth) const;
	virtual DependencyBase &CreateDummySatisficer(void);

protected:
	Register &m_rReg;
};

class FlagsDependee : public Dependee
{
public:
	FlagsDependee(void);
	virtual ~FlagsDependee() {};

	virtual bool SatisfiesThis(DependencyBase &rDep);
	virtual void DebugPrint(int depth) const;
	virtual DependencyBase &CreateDummySatisficer(void);
};

class DependencyProvider;

class DependencyBase
{
public:
	typedef std::list<DependencyBase *> Dependencies;

	virtual bool ProvidesSameThing(DependencyBase &) = 0;
	virtual bool CanRun(std::map<DependencyBase *, int> &rScoreboard, int &rNopsNeeds, int currentCycle) = 0;

	virtual void DebugPrint(int depth);

protected:
	inline DependencyBase(int minCycles, bool hardDependency, const Dependee &rDep, DependencyProvider *pProvider)
	: m_minCycles(minCycles),
	  m_hardDependency(hardDependency),
	  m_rDep(rDep),
	  m_pProvider(pProvider)
	{
	};

	inline virtual ~DependencyBase() {};

	int m_minCycles;
	bool m_hardDependency;
	const Dependee &m_rDep;
	DependencyProvider *m_pProvider;
};

class DependencyWithoutInterlock : public DependencyBase
{
	virtual bool CanRun(std::map<DependencyBase *, int> &rScoreboard, int &rNopsNeeds, int currentCycle);

protected:
	inline DependencyWithoutInterlock(int minCycles, bool hardDependency, const Dependee &rDep, DependencyProvider *pProvider)
	: DependencyBase(minCycles, hardDependency, rDep, pProvider)
	{
	}

	virtual void DebugPrint(int depth);
};

class DependencyWithStall : public DependencyBase
{
	virtual bool CanRun(std::map<DependencyBase *, int> &rScoreboard, int &rNopsNeeds, int currentCycle);

protected:
	inline DependencyWithStall(int minCycles, bool hardDependency, const Dependee &rDep, DependencyProvider *pProvider)
	: DependencyBase(minCycles, hardDependency, rDep, pProvider)
	{
	}

	virtual void DebugPrint(int depth);
};

class RaRbDependency : public DependencyWithoutInterlock
{
public:
	RaRbDependency(Register &rReg, DependencyProvider *pProvider);

	virtual bool ProvidesSameThing(DependencyBase &);

	virtual void DebugPrint(int depth);

	Register &GetReg();

protected:
	static int CyclesStalled(Register &rReg);
	Register &m_rReg;
};

class AccDependency : public DependencyWithoutInterlock
{
public:
	AccDependency(Register &rReg, DependencyProvider *pProvider);

	virtual bool ProvidesSameThing(DependencyBase &);

	virtual void DebugPrint(int depth);

	Register &GetReg();
protected:
	Register &m_rReg;
};

class FlagsDependency : public DependencyWithoutInterlock
{
public:
	FlagsDependency(DependencyProvider *pProvider);

	virtual bool ProvidesSameThing(DependencyBase &);

	virtual void DebugPrint(int depth);
};

/*class BranchDependency : public DependencyWithoutInterlock
{
public:
	inline BranchDependency(void)
	: DependencyWithoutInterlock(3, false)
	{
	}
protected:
};*/

class DependencyProvider
{
public:
	inline virtual ~DependencyProvider() {};

	virtual void GetOutputDeps(DependencyBase::Dependencies &) = 0;
};

class BbDepProvider : public DependencyProvider
{
public:
	inline virtual ~BbDepProvider() {};

	inline virtual void GetOutputDeps(DependencyBase::Dependencies &rDeps)
	{
		rDeps.clear();
	}
};

class DependencyConsumer
{
public:
	inline virtual ~DependencyConsumer() {};

	virtual void GetInputDeps(Dependee::Dependencies &rDeps) = 0;
	virtual void AddResolvedInputDep(DependencyBase &rDep) = 0;
	virtual DependencyBase::Dependencies &GetResolvedInputDeps(void) = 0;

	virtual void DebugPrintResolvedDeps(void) = 0;
};

class Instruction : public Base, public Assemblable, public DependencyProvider, public DependencyConsumer
{
public:
	Instruction();
	virtual ~Instruction();

	virtual void Assemble(Fields &rFields);

	virtual void GetOutputDeps(DependencyBase::Dependencies &);
	virtual void GetInputDeps(Dependee::Dependencies &rDeps);
	virtual void AddResolvedInputDep(DependencyBase &rDep);
	virtual DependencyBase::Dependencies &GetResolvedInputDeps(void);

	virtual void DebugPrintResolvedDeps(void);

protected:
	DependencyBase::Dependencies m_deps;
	DependencyBase::Dependencies m_outputDeps;
	Dependee::Dependencies m_inputDeps;
};

class SmallImm : public SecondSource, public Assemblable
{
public:
	SmallImm(Value &, bool vecrot);
	virtual ~SmallImm();

	virtual void DebugPrint(int depth);
	virtual void DebugPrintAsm(void);
	virtual unsigned int GetEncodedValue(void);

private:
	Value &m_rValue;
	bool m_vecrot;
};

class Opcode : public Base, public Assemblable
{
public:
	Opcode(AddOp);
	Opcode(MulOp);

	virtual ~Opcode();

	virtual void DebugPrint(int depth);
	virtual void DebugPrintAsm(void);
	virtual void Assemble(Fields &rFields);
	virtual unsigned int GetEncodedValue(void);

private:
	AddOp m_leftOp;
	MulOp m_rightOp;

	bool m_leftType;
};

class AddPipeInstruction;
class MulPipeInstruction;
class AluSignal;

class BasePipeInstruction : public Base, public Assemblable
{
public:
	BasePipeInstruction(Opcode &, Register &, Register &, SecondSource &, SmallImm *, InstructionCondition &, bool setFlags);
	virtual ~BasePipeInstruction();

	virtual void DebugPrint(int depth);
	virtual void DebugPrintAsm(void);

	inline bool IsNopOrNever(void)
	{
		//todo do a nicer job here
		if (m_rOpcode.GetEncodedValue() == 0 || m_rCondition.GetEncodedValue() == 0)
			return true;
		else
			return false;
	}

	static MulPipeInstruction &GenerateCompatibleInstruction(AddPipeInstruction &);
	static AddPipeInstruction &GenerateCompatibleInstruction(MulPipeInstruction &);

	static bool AreCompatible(AddPipeInstruction &, MulPipeInstruction &, AluSignal &);

	inline Register *GetDestReg(bool ifNopIgnore)
	{
		if (ifNopIgnore && (m_rDest.GetId() == 39 || IsNopOrNever()))
			return 0;

		return &m_rDest;
	}

	inline Register *GetSourceRegA(bool ifNopIgnore)
	{
		if (ifNopIgnore && (m_rSource1.GetId() == 39 || IsNopOrNever()))
			return 0;

		return &m_rSource1;
	}

	inline Register *GetSourceRegB(bool ifNopIgnore)
	{
		Register *r = dynamic_cast<Register *>(&m_rSource2);
		if (!r)
			return 0;

		if (ifNopIgnore && (r->GetId() == 39 || IsNopOrNever()))
			return 0;

		return r;
	}

	inline bool GetSetsFlags(void)
	{
		return m_setFlags;
	}

	inline bool GetUsesFlags(void)
	{
		return !(m_rCondition.GetEncodedValue() == kAlways || m_rCondition.GetEncodedValue() == kNever);
	}

protected:
	void AssembleAs(Fields &rFields, bool aluPipe);

	Opcode &m_rOpcode;
	Register &m_rDest;
	Register &m_rSource1;
	SecondSource &m_rSource2;
	InstructionCondition &m_rCondition;
	SmallImm *m_pVecrot;
	bool m_setFlags;
};

class AddPipeInstruction : public BasePipeInstruction
{
public:
	AddPipeInstruction(Opcode &, Register &, Register &, SecondSource &, InstructionCondition &, bool setFlags);
	virtual ~AddPipeInstruction();

	virtual void Assemble(Fields &rFields);

	static AddPipeInstruction &Nop(void);
private:
};

class MulPipeInstruction : public BasePipeInstruction
{
public:
	MulPipeInstruction(Opcode &, Register &, Register &, SecondSource &, SmallImm *pVecrot, InstructionCondition&, bool setFlags);
	virtual ~MulPipeInstruction();

	virtual void Assemble(Fields &rFields);

	static MulPipeInstruction &Nop(void);
private:
};


class AluSignal : public Base, public Assemblable
{
public:
	AluSignal(Signal);
	virtual ~AluSignal();

	virtual void DebugPrint(int depth);
	virtual void DebugPrintAsm(void);
	virtual unsigned int GetEncodedValue(void);

	static AluSignal &DefaultSignal(void);

private:
	Signal m_signal;
};

class AluInstruction : public Instruction
{
public:
	AluInstruction(AddPipeInstruction &, MulPipeInstruction &, AluSignal &, bool force = false);
	virtual ~AluInstruction();

	virtual void Assemble(Fields &rFields);

	virtual void DebugPrint(int depth);
	virtual void DebugPrintAsm(void);

	virtual void GetOutputDeps(DependencyBase::Dependencies &);
	virtual void GetInputDeps(Dependee::Dependencies &rDeps);

	static Instruction &Nop(void);

private:
	AddPipeInstruction &m_rLeft;
	MulPipeInstruction &m_rRight;
	AluSignal &m_rSignal;
};

class IlInstruction : public Instruction
{
public:
	IlInstruction(Register &, Value &, InstructionCondition &, bool setFlags);
	virtual ~IlInstruction();

	virtual void DebugPrint(int depth);
	virtual void Assemble(Fields &rFields);

	virtual void DebugPrintAsm(void);

	virtual void GetOutputDeps(DependencyBase::Dependencies &);
	virtual void GetInputDeps(Dependee::Dependencies &rDeps);

private:
	Register &m_rDest;
	Value &m_rImmediate;
	InstructionCondition &m_rCondition;
	bool m_setFlags;
};

class SemInstruction : public Instruction
{
public:
	enum Operation
	{
		kInc,
		kDec,
	};
	SemInstruction(Value &, Operation);
	virtual ~SemInstruction();

	virtual void DebugPrint(int depth);
	virtual void Assemble(Fields &rFields);

private:
	Value &m_rSemId;
	Operation m_op;
};

class BranchInstruction : public Instruction
{
public:
	BranchInstruction(bool absolute, BrCondition &, Register &, Register &, Register &, Value &);
	BranchInstruction(bool absolute, BrCondition &, Register &, Register &, Value &);
	virtual ~BranchInstruction();

	virtual void DebugPrint(int depth);
	virtual void Assemble(Fields &rFields);
	virtual void DebugPrintAsm(void);

	virtual void GetOutputDeps(DependencyBase::Dependencies &);
	virtual void GetInputDeps(Dependee::Dependencies &rDeps);

private:
	bool m_absolute;
	BrCondition &m_rCondition;
	Register &m_rDestA;
	Register &m_rDestM;
	Register *m_pSource;
	Value &m_rImm;
};

class Label : public Value
{
public:
	Label(const char *, bool isDefinition);
	virtual ~Label();

	virtual void DebugPrint(int depth);
	virtual void Assemble(Fields &rFields);

	virtual void SetAddress(unsigned int a);
	virtual const char *GetName(void);
	virtual void Link(Label *pDeclared);

private:

	Label *m_pDeclaredLabel;

	static const int sm_maxLengthIncNull = 30;
	char m_name[sm_maxLengthIncNull];
};

#endif /* INSTRUCTIONTREE_H_ */
