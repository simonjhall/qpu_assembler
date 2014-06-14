/*
 * LoadStore.h
 *
 *  Created on: 14 Jun 2014
 *      Author: simon
 */

#ifndef LOADSTORE_H_
#define LOADSTORE_H_

#include <assert.h>
#include <list>

#include "InstructionTree.h"

class LoadStore
{
public:
	LoadStore();
	virtual ~LoadStore();

	inline static LoadStore &Get(void)
	{
		static LoadStore *p = 0;
		if (!p)
		{
			p = new LoadStore();
			assert(p);
		}

		return *p;
	}

	void LoadWordMem(std::list<Base *> &rStatements,
			Register *pDest, Register *pSource, Value *pImm);

	void LoadWordVpm(std::list<Base *> &rStatements,
			Register *pDest, Register *pSource, Value *pImm);

	void StoreWordMem(std::list<Base *> &rStatements,
			Register *pToStore, Register *pSource, Value *pImm, Value *pWordCount);

	void StoreWordVpm(std::list<Base *> &rStatements,
			Register *pToStore, Register *pSource, Value *pImm, Value *pWordCount);
};

#endif /* LOADSTORE_H_ */
