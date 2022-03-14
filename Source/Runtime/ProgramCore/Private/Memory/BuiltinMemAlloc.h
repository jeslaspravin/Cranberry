/*!
 * \file BuiltinMemAlloc.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Memory/MemAllocator.h"

class CBBuiltinMemAlloc final : public CBMemAlloc
{
private:
	SizeT totalAllocation = 0;
public:
	void* tryMalloc(SizeT size, uint32 alignment = DEFAULT_ALIGNMENT) final;
	void* memAlloc(SizeT size, uint32 alignment = DEFAULT_ALIGNMENT) final;
	void* tryRealloc(void* currentPtr, SizeT size, uint32 alignment = DEFAULT_ALIGNMENT) final;
	void* memRealloc(void* currentPtr, SizeT size, uint32 alignment = DEFAULT_ALIGNMENT) final;
	void memFree(void* ptr) final;

	SizeT getAllocationSize() const final;
};