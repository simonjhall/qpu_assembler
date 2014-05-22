/*
 * DebugPrint.cpp
 *
 *  Created on: 14 Apr 2014
 *      Author: simon
 */

#include <stdio.h>
#include "InstructionTree.h"

void Base::DebugPrint(int depth)
{
	printf("BASE DEBUG PRINT\n");
}

void Register::DebugPrint(int depth)
{
	for (int count = 0; count < depth; count++)
		printf("\t");

	printf("%s%d\n",
			m_loc == kRa ? "ra" : (m_loc == kRb ? "rb" : "acc"),
			m_id);
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

void Value::DebugPrint(int depth)
{
	for (int count = 0; count < depth; count++)
		printf("\t");

	if (m_denom == 0)
		printf("value: %d, size: %d\n", m_value, m_byteCount);
	else
		printf("value: %d / %d, size: %d\n", m_num, m_denom, m_byteCount);
}

void Opcode::DebugPrint(int depth)
{
	for (int count = 0; count < depth; count++)
		printf("\t");

	if (m_leftType)
		printf("opcode: %s\n", GetAddOpName(m_leftOp));
	else
		printf("opcode: %s\n", GetMulOpName(m_rightOp));
}

void AluSignal::DebugPrint(int depth)
{
	for (int count = 0; count < depth; count++)
		printf("\t");

	printf("signal: %s\n", GetSignalName(m_signal));
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

void SmallImm::DebugPrint(int depth)
{
	for (int count = 0; count < depth; count++)
		printf("\t");

	printf("small imm\n");
	m_rValue.DebugPrint(depth + 1);
}

void InstructionCondition::DebugPrint(int depth)
{
	for (int count = 0; count < depth; count++)
		printf("\t");

	printf("condition: %s\n", GetConditionCodeName(m_condition));
}

void BrCondition::DebugPrint(int depth)
{
	for (int count = 0; count < depth; count++)
		printf("\t");

	printf("condition: %s\n", GetBranchConditionName(m_condition));
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

void SemInstruction::DebugPrint(int depth)
{
	for (int count = 0; count < depth; count++)
		printf("\t");

	printf("semaphore %s\n", m_op == kInc ? "INCREMENT" : "DECREMENT");
	m_rSemId.DebugPrint(depth + 1);
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

void Label::DebugPrint(int depth)
{
	for (int count = 0; count < depth; count++)
		printf("\t");

	printf("label name: %s\n", m_name);
}

void ReorderControl::DebugPrint(int depth)
{
	for (int count = 0; count < depth; count++)
		printf("\t");

	printf("reordering: %s\n", m_begin ? "TRUE" : "FALSE");
}

void Instruction::DebugPrintDeps(void)
{
	for (auto it = m_deps.begin(); it != m_deps.end(); it++)
	{
		(*it)->DebugPrint(0);
	}
}

void DependencyBase::DebugPrint(int depth)
{
	for (int count = 0; count < depth; count++)
		printf("\t");
	printf("min cycles: %d\n", m_minCycles);

	for (int count = 0; count < depth; count++)
		printf("\t");
	printf("hard dep: %s\n", m_hardDependency ? "TRUE" : "FALSE");

	for (int count = 0; count < depth; count++)
		printf("\t");
	printf("dep provider: %p\n", m_pProvider);

	m_rDep.DebugPrint(depth + 1);
}

void DependencyWithoutInterlock::DebugPrint(int depth)
{
	for (int count = 0; count < depth; count++)
		printf("\t");
	printf("dep without interlock\n");

	DependencyBase::DebugPrint(depth + 1);
}

void DependencyWithStall::DebugPrint(int depth)
{
	for (int count = 0; count < depth; count++)
		printf("\t");
	printf("dep with stall\n");

	DependencyBase::DebugPrint(depth + 1);
}

void RaRbDependency::DebugPrint(int depth)
{
	DependencyWithoutInterlock::DebugPrint(depth + 1);
}

void AccDependency::DebugPrint(int depth)
{
	DependencyWithoutInterlock::DebugPrint(depth + 1);
}

void RegisterDependee::DebugPrint(int depth) const
{
	for (int count = 0; count < depth; count++)
		printf("\t");

	printf("RegisterDependee\n");

	m_rReg.DebugPrint(depth + 1);
}

void FlagsDependency::DebugPrint(int depth)
{
	DependencyWithoutInterlock::DebugPrint(depth + 1);
}

void FlagsDependee::DebugPrint(int depth) const
{
	for (int count = 0; count < depth; count++)
		printf("\t");

	printf("FlagsDependee\n");
}
