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
		inline Field(int position, int size, int value)
		: m_position(position),
		  m_size(size),
		  m_value(value)
		{
		};

		int m_position;
		int m_size;
		int m_value;
	};

	typedef std::list<Field> Fields;

	virtual void Assemble(Fields &rFields);
};

class InstructionCondition : public Base
{
public:
	InstructionCondition(ConditionCode);
	virtual ~InstructionCondition();

	virtual void DebugPrint(int depth);

private:
	ConditionCode m_condition;
};

class BrCondition : public Base
{
public:
	BrCondition(BranchCondition);
	virtual ~BrCondition();

	virtual void DebugPrint(int depth);

private:
	BranchCondition m_condition;
};

class Value : public Base
{
public:
	Value(int);
	Value(int numerator, int denominator);
	virtual ~Value();

	virtual void DebugPrint(int depth);
	virtual void SetSize(int byteCount);

	virtual int GetIntValue(void);

protected:
	int m_value;
	int m_num, m_denom;

	int m_byteCount;
};

class Instruction : public Base, public Assemblable
{
public:
	Instruction();
	virtual ~Instruction();
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

private:
	Location m_loc;
	int m_id;
};

class SmallImm : public SecondSource
{
public:
	SmallImm(Value &);
	virtual ~SmallImm();

	virtual void DebugPrint(int depth);

private:
	Value &m_rValue;
};

class Opcode : public Base
{
public:
	Opcode(AddOp);
	Opcode(MulOp);

	virtual ~Opcode();

	virtual void DebugPrint(int depth);

private:
	AddOp m_leftOp;
	MulOp m_rightOp;

	bool m_leftType;
};

class AddPipeInstruction;
class MulPipeInstruction;
class AluSignal;

class BasePipeInstruction : public Base
{
public:
	BasePipeInstruction(Opcode &, Register &, Register &, SecondSource &, InstructionCondition &, bool setFlags);
	virtual ~BasePipeInstruction();

	virtual void DebugPrint(int depth);

	static MulPipeInstruction &GenerateCompatibleInstruction(AddPipeInstruction &);
	static AddPipeInstruction &GenerateCompatibleInstruction(MulPipeInstruction &);

	static bool AreCompatible(AddPipeInstruction &, MulPipeInstruction &, AluSignal &);

protected:
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

private:
};

class MulPipeInstruction : public BasePipeInstruction
{
public:
	MulPipeInstruction(Opcode &, Register &, Register &, SecondSource &, InstructionCondition&, bool setFlags);
	virtual ~MulPipeInstruction();

private:
};


class AluSignal : public Base
{
public:
	AluSignal(Signal);
	virtual ~AluSignal();

	virtual void DebugPrint(int depth);

	static AluSignal &DefaultSignal(void);

private:
	Signal m_signal;
};

class AluInstruction : public Instruction
{
public:
	AluInstruction(AddPipeInstruction &, MulPipeInstruction &, AluSignal &);
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

private:
	Register &m_rDest;
	Value &m_rImmediate;
	InstructionCondition &m_rCondition;
	bool m_setFlags;
};

class SemInstruction : public Base
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

private:
	Value &m_rSemId;
	Operation m_op;
};

class BranchInstruction : public Base
{
public:
	BranchInstruction(bool absolute, BrCondition &, Register &, Register &, Register &, Value &);
	BranchInstruction(bool absolute, BrCondition &, Register &, Register &, Value &);
	virtual ~BranchInstruction();

	virtual void DebugPrint(int depth);

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
	Label(const char *, bool);
	virtual ~Label();

	virtual void DebugPrint(int depth);

private:
	static const int sm_maxLengthIncNull = 30;
	char m_name[sm_maxLengthIncNull];
};

#endif /* INSTRUCTIONTREE_H_ */
