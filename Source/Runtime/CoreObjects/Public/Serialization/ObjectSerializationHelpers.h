/*!
 * \file ObjectSerializationHelpers.h
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "CBEObject.h"

class COREOBJECTS_EXPORT ObjectSerializationHelpers
{
private:
    ObjectSerializationHelpers() = default;

public:
    static ObjectArchive &serializeAllFields(CBE::Object *obj, ObjectArchive &ar);
};