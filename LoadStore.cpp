/*
 * LoadStore.cpp
 *
 *  Created on: 14 Jun 2014
 *      Author: simon
 */

#include "LoadStore.h"

#define AT 27

LoadStore::LoadStore()
{
	// TODO Auto-generated constructor stub

}

LoadStore::~LoadStore()
{
	// TODO Auto-generated destructor stub
}

void LoadStore::LoadWordMem(std::list<Base *> &rStatements,
		Register* pDest, Register* pSource, Value* pImm)
{
	bool isLabel = dynamic_cast<Label *>(pImm) ? true : false;

	//create a barrier
	rStatements.push_back(
			new ReorderControl(false)
	);
	rStatements.push_back(
			new ReorderControl(true)
	);

	//configure dma read
	{
		rStatements.push_back(
			new IlInstruction(*new Register(Register::kRa, 49),			//write to vpm_vcd_rd_setup
				*new Value((1 << 31)			//vdr dma basic setup
						| (0 << 28)				//width=32-bit
						| (1 << 24)				//row-to-row pitch in memory is 8*2^xxx
						| (16 << 20)			//16 elements to a row
						| (1 << 16)				//one row
						| (0 << 12)				//row-to-row pitch in vpm (only loading one row anyway)
						| (0 << 11)				//horizontal
						| (0 << 0)),			//X=0, Y=0,
				*new InstructionCondition(kAlways),
				false)
		);
	}

	//write load address and trigger load
	if (!isLabel && pImm->GetIntValue() >= -16 && pImm->GetIntValue() <= 15)
	{
		Register *p = pSource;

		//we can't do dest = rb OP imm, only ra OP imm so go via AT-A
		if (pSource->GetLocation() == Register::kRb)
		{
			Register *pAt = new Register(Register::kRa, AT);

			AddPipeInstruction *pLeft = new AddPipeInstruction(*new Opcode(kOr),
					*pAt,
					*pSource,
					*pSource,
					*new InstructionCondition(kAlways),
					false);

			rStatements.push_back(new AluInstruction(*pLeft,
					BasePipeInstruction::GenerateCompatibleInstruction(*pLeft),
					AluSignal::DefaultSignal())
			);

			p = pAt;
		}

		AddPipeInstruction &rLeft = *new AddPipeInstruction(*new Opcode(kAdd),
				*new Register(Register::kRa, 50), *p, *new SmallImm(*pImm, false),				//vpm_ld_addr = pSource + imm
				*new InstructionCondition(kAlways), false);
		rStatements.push_back(
				new AluInstruction(rLeft,
						BasePipeInstruction::GenerateCompatibleInstruction(rLeft),
						AluSignal::DefaultSignal())
		);
	}
	else
	{
		Register *p = pSource;

		//go via AT-A if source is in rb
		if (pSource->GetLocation() == Register::kRb)
		{
			Register *pAtA = new Register(Register::kRa, AT);

			AddPipeInstruction *pLeft = new AddPipeInstruction(*new Opcode(kOr),
					*pAtA,
					*pSource,
					*pSource,
					*new InstructionCondition(kAlways),
					false);

			rStatements.push_back(new AluInstruction(*pLeft,
					BasePipeInstruction::GenerateCompatibleInstruction(*pLeft),
					AluSignal::DefaultSignal())
			);

			p = pAtA;
		}

		Register *pAtB = new Register(Register::kRb, AT);
		rStatements.push_back(
			new IlInstruction(*pAtB,			//write to AT
				*pImm,
				*new InstructionCondition(kAlways),
				false)
		);
		AddPipeInstruction &rLeft = *new AddPipeInstruction(*new Opcode(kAdd),
				*new Register(Register::kRa, 50), *p, *pAtB,				//vpm_ld_addr = pSource + AT
				*new InstructionCondition(kAlways), false);
		rStatements.push_back(
				new AluInstruction(rLeft,
						BasePipeInstruction::GenerateCompatibleInstruction(rLeft),
						AluSignal::DefaultSignal())
		);
	}

	//wait for the dma to complete
	{
		AddPipeInstruction &rLeft = *new AddPipeInstruction(*new Opcode(kOr),
				*new Register(Register::kRa, 39), *new Register(Register::kRa, 50), *new Register(Register::kRa, 50), //wra_nop = vpm_ld_wait | vpm_ld_wait
				*new InstructionCondition(kAlways), false);
		rStatements.push_back(
				new AluInstruction(rLeft,
						BasePipeInstruction::GenerateCompatibleInstruction(rLeft),
						AluSignal::DefaultSignal())
		);
	}

	//create a barrier
	rStatements.push_back(
			new ReorderControl(false)
	);
	rStatements.push_back(
			new ReorderControl(true)
	);

	//trigger the load from vpm to qpu
	{
		rStatements.push_back(
			new IlInstruction(*new Register(Register::kRa, 49),			//write to vpm_vcd_rd_setup
				*new Value((0 << 30)			//vpm generic block read setup
						| (1 << 20)				//one vector to read
						| (0 << 12)				//stride, only reading one vector anyway
						| (1 << 11)				//horizontal
						| (2 << 8)				//32-bit
						| (0 << 0)),			//X=0, Y=0
				*new InstructionCondition(kAlways),
				false)
		);
	}

	//move from vpm read dat to dest
	{
		//need to broadcast loads to lr
		if (pDest->GetLocation() == Register::kRa && pDest->GetId() == 31)
		{
			Register *pAcc5 = new Register(Register::kAcc, 5);

			MulPipeInstruction &rRight = *new MulPipeInstruction(*new Opcode(kV8min),
					*pAcc5, *new Register(Register::kRa, 48), *new Register(Register::kRa, 48), //acc5 = rda_vpm_dat | rda_vpm_dat
					0,
					*new InstructionCondition(kAlways), false);

			rStatements.push_back(
					new AluInstruction(BasePipeInstruction::GenerateCompatibleInstruction(rRight),
							rRight,
							AluSignal::DefaultSignal())
			);

			AddPipeInstruction &rLeft = *new AddPipeInstruction(*new Opcode(kOr),
					*pDest, *pAcc5, *pAcc5, 													//lr = acc5 | accc5
					*new InstructionCondition(kAlways), false);

			rStatements.push_back(
					new AluInstruction(rLeft,
							BasePipeInstruction::GenerateCompatibleInstruction(rLeft),
							AluSignal::DefaultSignal())
			);
		}
		else
		{
			AddPipeInstruction &rLeft = *new AddPipeInstruction(*new Opcode(kOr),
					*pDest, *new Register(Register::kRa, 48), *new Register(Register::kRa, 48), //dest = rda_vpm_dat | rda_vpm_dat
					*new InstructionCondition(kAlways), false);

			rStatements.push_back(
					new AluInstruction(rLeft,
							BasePipeInstruction::GenerateCompatibleInstruction(rLeft),
							AluSignal::DefaultSignal())
			);
		}
	}

	//create a barrier (to prevent something getting written into pDest)
/*	rStatements.push_back(
			new ReorderControl(false)
	);
	rStatements.push_back(
			new ReorderControl(true)
	);*/
}

void LoadStore::LoadWordVpm(std::list<Base*>& rStatements, Register* pDest,
		Register* pSource, Value* pImm)
{
	bool isLabel = dynamic_cast<Label *>(pImm) ? true : false;
	assert(!isLabel);

	Register *pAtA = new Register(Register::kRa, AT);
	Register *pAtB = new Register(Register::kRb, AT);

	//compute EA
	Register *pEa = pSource;

	if (pImm->GetIntValue() == 0)
	{
	}
	else if (pImm->GetIntValue() >= -16 && pImm->GetIntValue() <= 15)
	{
		//compute EA
		Register *p = pSource;

		//go via AT-A if source is in rb
		if (pSource->GetLocation() == Register::kRb)
		{
			AddPipeInstruction *pLeft = new AddPipeInstruction(*new Opcode(kOr),
					*pAtA,
					*pSource,
					*pSource,
					*new InstructionCondition(kAlways),
					false);

			rStatements.push_back(new AluInstruction(*pLeft,
					BasePipeInstruction::GenerateCompatibleInstruction(*pLeft),
					AluSignal::DefaultSignal())
			);

			p = pAtA;
		}

		AddPipeInstruction &rLeft = *new AddPipeInstruction(*new Opcode(kAdd),
				*pAtA, *p, *new SmallImm(*pImm, false),				//AT-A = pSource + imm
				*new InstructionCondition(kAlways), false);
		rStatements.push_back(
				new AluInstruction(rLeft,
						BasePipeInstruction::GenerateCompatibleInstruction(rLeft),
						AluSignal::DefaultSignal())
		);

		pEa = pAtA;
	}
	else
	{
		Register *p = pSource;

		//go via AT-A if source is in rb
		if (pSource->GetLocation() == Register::kRb)
		{
			AddPipeInstruction *pLeft = new AddPipeInstruction(*new Opcode(kOr),
					*pAtA,
					*pSource,
					*pSource,
					*new InstructionCondition(kAlways),
					false);

			rStatements.push_back(new AluInstruction(*pLeft,
					BasePipeInstruction::GenerateCompatibleInstruction(*pLeft),
					AluSignal::DefaultSignal())
			);

			p = pAtA;
		}

		Register *pAtB = new Register(Register::kRb, AT);
		rStatements.push_back(
			new IlInstruction(*pAtB,			//write to AT-B
				*pImm,
				*new InstructionCondition(kAlways),
				false)
		);
		AddPipeInstruction &rLeft = *new AddPipeInstruction(*new Opcode(kAdd),
				*pAtA, *p, *pAtB,				//AT-A = pSource + AT-B
				*new InstructionCondition(kAlways), false);
		rStatements.push_back(
				new AluInstruction(rLeft,
						BasePipeInstruction::GenerateCompatibleInstruction(rLeft),
						AluSignal::DefaultSignal())
		);

		pEa = pAtA;
	}

	//divide by 64
	AddPipeInstruction &rLeftShr = *new AddPipeInstruction(*new Opcode(kShr),
			*pAtA, *pEa, *new SmallImm(*new Value(6), false),				//AT-A = EA >> 6
			*new InstructionCondition(kAlways), false);
	rStatements.push_back(
			new AluInstruction(rLeftShr,
					BasePipeInstruction::GenerateCompatibleInstruction(rLeftShr),
					AluSignal::DefaultSignal())
	);

	//get the base part of the vpm config
	rStatements.push_back(
		new IlInstruction(*pAtB,			//write to AT-B
			*new Value((0 << 30)			//vpm generic block read setup
					| (1 << 20)				//one vector to read
					| (0 << 12)				//stride, only reading one vector anyway
					| (1 << 11)				//horizontal
					| (2 << 8)				//32-bit
					| (0 << 0)),			//X=0, Y=0
			*new InstructionCondition(kAlways),
			false)
	);

	//create a barrier
	rStatements.push_back(
			new ReorderControl(false)
	);
	rStatements.push_back(
			new ReorderControl(true)
	);

	//write to vpm_vcd_rd_setup
	AddPipeInstruction &rLeft = *new AddPipeInstruction(*new Opcode(kOr),
			*new Register(Register::kRa, 49), *pAtA, *pAtB,				//AT-A = EA | config
			*new InstructionCondition(kAlways), false);
	rStatements.push_back(
			new AluInstruction(rLeft,
					BasePipeInstruction::GenerateCompatibleInstruction(rLeft),
					AluSignal::DefaultSignal())
	);

	//dest can't be lr as we need broadcast
	assert (!(pDest->GetLocation() == Register::kRa && pDest->GetId() == 31));

	//move from vpm read dat to dest
	AddPipeInstruction &rLeftMov = *new AddPipeInstruction(*new Opcode(kOr),
			*pDest, *new Register(Register::kRa, 48), *new Register(Register::kRa, 48), //dest = rda_vpm_dat | rda_vpm_dat
			*new InstructionCondition(kAlways), false);

	rStatements.push_back(
			new AluInstruction(rLeftMov,
					BasePipeInstruction::GenerateCompatibleInstruction(rLeftMov),
					AluSignal::DefaultSignal())
	);
}

void LoadStore::StoreWordMem(std::list<Base*>& rStatements, Register* pToStore,
		Register* pSource, Value* pImm, Value *pWordCount)
{
	bool isLabel = dynamic_cast<Label *>(pImm) ? true : false;

	//create a barrier
	rStatements.push_back(
			new ReorderControl(false)
	);
	rStatements.push_back(
			new ReorderControl(true)
	);

	//trigger the store from qpu to vpm
	{
		rStatements.push_back(
			new IlInstruction(*new Register(Register::kRb, 49),			//write to vpm_vcd_wr_setup
				*new Value((0 << 30)			//vpm generic block write setup
						| (0 << 12)				//stride, only reading one vector anyway
						| (1 << 11)				//horizontal
						| (2 << 8)				//32-bit
						| (0 << 0)),			//X=0, Y=0
				*new InstructionCondition(kAlways),
				false)
		);
	}

	//move from vpm read dat to dest
	//wait for the dma to complete
	{
		AddPipeInstruction &rLeft = *new AddPipeInstruction(*new Opcode(kOr),
				*new Register(Register::kRa, 48), *pToStore, *pToStore,					//wra_vpm_dat = to store | to store
				*new InstructionCondition(kAlways), false);
		rStatements.push_back(
				new AluInstruction(rLeft,
						BasePipeInstruction::GenerateCompatibleInstruction(rLeft),
						AluSignal::DefaultSignal())
		);
	}

	//create a barrier
	rStatements.push_back(
			new ReorderControl(false)
	);
	rStatements.push_back(
			new ReorderControl(true)
	);


	//configure dma write
	{
		rStatements.push_back(
			new IlInstruction(*new Register(Register::kRb, 49),			//write to vpm_vcd_wd_setup
				*new Value((2 << 30)			//vdw dma basic setup
						| (1 << 23)				//one row
						| (pWordCount->GetIntValue() << 16)			//row length of 2d block in memory
						| (1 << 14)				//horizontal
						| (0 << 3)				//X=0, Y=0,
						| (0 << 0)),			//32-bit
				*new InstructionCondition(kAlways),
				false)
		);
	}

	//write store address and trigger load
	if (!isLabel && pImm->GetIntValue() >= -16 && pImm->GetIntValue() <= 15)
	{
		Register *p = pSource;
		//go via AT-A if source is in rb
		if (pSource->GetLocation() == Register::kRb)
		{
			Register *pAtA = new Register(Register::kRa, AT);

			AddPipeInstruction *pLeft = new AddPipeInstruction(*new Opcode(kOr),
					*pAtA,
					*pSource,
					*pSource,
					*new InstructionCondition(kAlways),
					false);

			rStatements.push_back(new AluInstruction(*pLeft,
					BasePipeInstruction::GenerateCompatibleInstruction(*pLeft),
					AluSignal::DefaultSignal())
			);

			p = pAtA;
		}

		AddPipeInstruction &rLeft = *new AddPipeInstruction(*new Opcode(kAdd),
				*new Register(Register::kRb, 50), *p, *new SmallImm(*pImm, false),				//vpm_st_addr = pSource + imm
				*new InstructionCondition(kAlways), false);
		rStatements.push_back(
				new AluInstruction(rLeft,
						BasePipeInstruction::GenerateCompatibleInstruction(rLeft),
						AluSignal::DefaultSignal())
		);
	}
	else
	{
		Register *p = pSource;
		//go via AT-A if source is in rb
		if (pSource->GetLocation() == Register::kRb)
		{
			Register *pAtA = new Register(Register::kRa, AT);

			AddPipeInstruction *pLeft = new AddPipeInstruction(*new Opcode(kOr),
					*pAtA,
					*pSource,
					*pSource,
					*new InstructionCondition(kAlways),
					false);

			rStatements.push_back(new AluInstruction(*pLeft,
					BasePipeInstruction::GenerateCompatibleInstruction(*pLeft),
					AluSignal::DefaultSignal())
			);

			p = pAtA;
		}

		Register *pAtB = new Register(Register::kRb, AT);
		rStatements.push_back(
			new IlInstruction(*pAtB,			//write to AT
				*pImm,
				*new InstructionCondition(kAlways),
				false)
		);
		AddPipeInstruction &rLeft = *new AddPipeInstruction(*new Opcode(kAdd),
				*new Register(Register::kRb, 50), *p, *pAtB,				//vpm_st_addr = pSource + AT
				*new InstructionCondition(kAlways), false);
		rStatements.push_back(
				new AluInstruction(rLeft,
						BasePipeInstruction::GenerateCompatibleInstruction(rLeft),
						AluSignal::DefaultSignal())
		);
	}

	//wait for the dma to complete
	{
		AddPipeInstruction &rLeft = *new AddPipeInstruction(*new Opcode(kOr),
				*new Register(Register::kRa, 39), *new Register(Register::kRb, 50), *new Register(Register::kRb, 50), //wra_nop = vpm_st_wait | vpm_st_wait
				*new InstructionCondition(kAlways), false);
		rStatements.push_back(
				new AluInstruction(rLeft,
						BasePipeInstruction::GenerateCompatibleInstruction(rLeft),
						AluSignal::DefaultSignal())
		);
	}

	//create a barrier (currently to prevent other registers stealing pSource)
	/*rStatements.push_back(
			new ReorderControl(false)
	);
	rStatements.push_back(
			new ReorderControl(true)
	);*/
}

void LoadStore::StoreWordVpm(std::list<Base*>& rStatements, Register* pToStore,
		Register* pSource, Value* pImm, Value* pWordCount)
{
	bool isLabel = dynamic_cast<Label *>(pImm) ? true : false;
	assert(!isLabel);

	Register *pAtA = new Register(Register::kRa, AT);
	Register *pAtB = new Register(Register::kRb, AT);

	//compute EA
	Register *pEa = pSource;

	if (pImm->GetIntValue() == 0)
	{
	}
	else if (pImm->GetIntValue() >= -16 && pImm->GetIntValue() <= 15)
	{
		//compute EA
		Register *p = pSource;

		//go via AT-A if source is in rb
		if (pSource->GetLocation() == Register::kRb)
		{
			AddPipeInstruction *pLeft = new AddPipeInstruction(*new Opcode(kOr),
					*pAtA,
					*pSource,
					*pSource,
					*new InstructionCondition(kAlways),
					false);

			rStatements.push_back(new AluInstruction(*pLeft,
					BasePipeInstruction::GenerateCompatibleInstruction(*pLeft),
					AluSignal::DefaultSignal())
			);

			p = pAtA;
		}

		AddPipeInstruction &rLeft = *new AddPipeInstruction(*new Opcode(kAdd),
				*pAtA, *p, *new SmallImm(*pImm, false),				//AT-A = pSource + imm
				*new InstructionCondition(kAlways), false);
		rStatements.push_back(
				new AluInstruction(rLeft,
						BasePipeInstruction::GenerateCompatibleInstruction(rLeft),
						AluSignal::DefaultSignal())
		);

		pEa = pAtA;
	}
	else
	{
		Register *p = pSource;

		//go via AT-A if source is in rb
		if (pSource->GetLocation() == Register::kRb)
		{
			AddPipeInstruction *pLeft = new AddPipeInstruction(*new Opcode(kOr),
					*pAtA,
					*pSource,
					*pSource,
					*new InstructionCondition(kAlways),
					false);

			rStatements.push_back(new AluInstruction(*pLeft,
					BasePipeInstruction::GenerateCompatibleInstruction(*pLeft),
					AluSignal::DefaultSignal())
			);

			p = pAtA;
		}

		Register *pAtB = new Register(Register::kRb, AT);
		rStatements.push_back(
			new IlInstruction(*pAtB,			//write to AT-B
				*pImm,
				*new InstructionCondition(kAlways),
				false)
		);
		AddPipeInstruction &rLeft = *new AddPipeInstruction(*new Opcode(kAdd),
				*pAtA, *p, *pAtB,				//AT-A = pSource + AT-B
				*new InstructionCondition(kAlways), false);
		rStatements.push_back(
				new AluInstruction(rLeft,
						BasePipeInstruction::GenerateCompatibleInstruction(rLeft),
						AluSignal::DefaultSignal())
		);

		pEa = pAtA;
	}

	//divide by 64
	AddPipeInstruction &rLeftShr = *new AddPipeInstruction(*new Opcode(kShr),
			*pAtA, *pEa, *new SmallImm(*new Value(6), false),				//AT-A = EA >> 6
			*new InstructionCondition(kAlways), false);
	rStatements.push_back(
			new AluInstruction(rLeftShr,
					BasePipeInstruction::GenerateCompatibleInstruction(rLeftShr),
					AluSignal::DefaultSignal())
	);

	//get the base part of the vpm config
	rStatements.push_back(
		new IlInstruction(*pAtB,			//write to AT-B
			*new Value((0 << 30)			//vpm generic block write setup
					| (0 << 12)				//stride, only reading one vector anyway
					| (1 << 11)				//horizontal
					| (2 << 8)				//32-bit
					| (0 << 0)),			//X=0, Y=0
			*new InstructionCondition(kAlways),
			false)
	);

	//create a barrier
	rStatements.push_back(
			new ReorderControl(false)
	);
	rStatements.push_back(
			new ReorderControl(true)
	);

	//write to vpm_vcd_wr_setup
	AddPipeInstruction &rLeft = *new AddPipeInstruction(*new Opcode(kOr),
			*new Register(Register::kRb, 49), *pAtA, *pAtB,				//AT-B = EA | config
			*new InstructionCondition(kAlways), false);
	rStatements.push_back(
			new AluInstruction(rLeft,
					BasePipeInstruction::GenerateCompatibleInstruction(rLeft),
					AluSignal::DefaultSignal())
	);


	//move from vpm read dat to dest
	AddPipeInstruction &rLeftMov = *new AddPipeInstruction(*new Opcode(kOr),
			*new Register(Register::kRa, 48), *pToStore, *pToStore,  //rda_vpm_dat = dest | dest
			*new InstructionCondition(kAlways), false);

	rStatements.push_back(
			new AluInstruction(rLeftMov,
					BasePipeInstruction::GenerateCompatibleInstruction(rLeftMov),
					AluSignal::DefaultSignal())
	);
}
