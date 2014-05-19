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
#include <stdint.h>

class Base
{
public:
	Base();
	virtual ~Base();
	virtual void DebugPrint(int depth);
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
		: m_totalSize(size)
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

	static bool CombineFields(Fields &rFields, unsigned int &rSizeInBytes, uint64_t &rOutput);
};

class InstructionCondition : public Base, public Assemblable
{
public:
	InstructionCondition(ConditionCode);
	virtual ~InstructionCondition();

	virtual void DebugPrint(int depth);
	virtual unsigned int GetEncodedValue(void);

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

	inline Location GetLocation(void) { return m_loc; }
	inline int GetId(void) { return m_id; }

private:
	Location m_loc;
	int m_id;
};

class DependencyBase
{
protected:
	inline DependencyBase(int minCycles, bool hardDependency)
	: m_minCycles(minCycles),
	  m_hardDependency(hardDependency)
	{
	};

	int m_minCycles;
	bool m_hardDependency;
};

class DependencyWithoutInterlock : public DependencyBase
{
protected:
	inline DependencyWithoutInterlock(int minCycles, bool hardDependency)
	: DependencyBase(minCycles, hardDependency)
	{
	}
};

class DependencyWithStall : public DependencyBase
{
protected:
	inline DependencyWithStall(int minCycles, bool hardDependency)
	: DependencyBase(minCycles, hardDependency)
	{
	}
};

class RaRbDependency : public DependencyWithoutInterlock
{
public:
	RaRbDependency(Register &rReg);
protected:
	Register &m_rReg;
};

class AccDependency : public DependencyWithoutInterlock
{
public:
	AccDependency(Register &rReg);
protected:
	Register &m_rReg;
};

class BranchDependency : public DependencyWithoutInterlock
{
public:
	inline BranchDependency(void)
	: DependencyWithoutInterlock(3, false)
	{
	}
protected:
};

class DependencyProvider
{
public:
	typedef std::list<DependencyBase *> Dependencies;

	inline virtual ~DependencyProvider() {};

	virtual Dependencies &GetDeps(void) = 0;
	virtual void AddDeps(void) = 0;
};

class Instruction : public Base, public Assemblable, public DependencyProvider
{
public:
	typedef std::list<Register> Registers;


	Instruction();
	virtual ~Instruction();

	virtual void Assemble(Fields &rFields);
	virtual void GetOutputs(Registers &rOutputs);
	virtual void GetInputs(Registers &rInputs);

protected:
	Dependencies m_deps;
};

class SmallImm : public SecondSource, public Assemblable
{
public:
	SmallImm(Value &);
	virtual ~SmallImm();

	virtual void DebugPrint(int depth);
	virtual unsigned int GetEncodedValue(void);

private:
	Value &m_rValue;
};

class Opcode : public Base, public Assemblable
{
public:
	Opcode(AddOp);
	Opcode(MulOp);

	virtual ~Opcode();

	virtual void DebugPrint(int depth);
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
	BasePipeInstruction(Opcode &, Register &, Register &, SecondSource &, InstructionCondition &, bool setFlags);
	virtual ~BasePipeInstruction();

	virtual void DebugPrint(int depth);
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

protected:
	void AssembleAs(Fields &rFields, bool aluPipe);

	Opcode &m_rOpcode;
	Register &m_rDest;
	Register &m_rSource1;
	SecondSource &m_rSource2;
	InstructionCondition &m_rCondition;
	bool m_setFlags;
};

class AddPipeInstruction : public BasePipeInstruction
{
public:
	AddPipeInstruction(Opcode &, Register &, Register &, SecondSource &, InstructionCondition &, bool setFlags);
	virtual ~AddPipeInstruction();

	virtual void Assemble(Fields &rFields);
private:
};

class MulPipeInstruction : public BasePipeInstruction
{
public:
	MulPipeInstruction(Opcode &, Register &, Register &, SecondSource &, InstructionCondition&, bool setFlags);
	virtual ~MulPipeInstruction();

	virtual void Assemble(Fields &rFields);
private:
};


class AluSignal : public Base, public Assemblable
{
public:
	AluSignal(Signal);
	virtual ~AluSignal();

	virtual void DebugPrint(int depth);
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
