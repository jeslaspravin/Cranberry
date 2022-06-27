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

namespace std
{
template <class _Kty, class _Hasher, class _Keyeq, class _Alloc>
class unordered_set;
}

class COREOBJECTS_EXPORT ObjectSerializationHelpers
{
private:
    ObjectSerializationHelpers() = default;

public:
    static ObjectArchive &serializeAllFields(CBE::Object *obj, ObjectArchive &ar);
    /**
     * fieldsToSerialize will be used only when writing/saving when reading object gets serialized just like serializeAllFields()
     */
    static ObjectArchive &serializeOnlyFields(CBE::Object *obj, ObjectArchive &ar, const std::unordered_set<StringID> &fieldsToSerialize);
};