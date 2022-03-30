/*!
 * \file CoreObjectsModule.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Modules/IModuleBase.h"
#include "CoreObjectsExports.h"

class COREOBJECTS_EXPORT CoreObjectsModule : public IModuleBase
{

public:
	/* IModuleBase overrides */
	void init() override;
	void release() override;

    /* Overrides ends */

	static CoreObjectsModule* get();
};