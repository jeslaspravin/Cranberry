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

    static ObjectArchive &serializeStructFields(void *structObj, CBEClass structType, ObjectArchive &ar);

public:
    static ObjectArchive &serializeAllFields(cbe::Object *obj, ObjectArchive &ar);
    /**
     * fieldsToSerialize will be used only when writing/saving when reading object gets serialized just like serializeAllFields()
     */
    static ObjectArchive &serializeOnlyFields(cbe::Object *obj, ObjectArchive &ar, const std::unordered_set<StringID> &fieldsToSerialize);

    // Use only if you are having custom serialize implementation in your Object and want to serialize data struct without manual serialization
    template <typename StructType>
    static ObjectArchive &serializeStructFields(StructType &structObj, ObjectArchive &ar)
    {
        return serializeStructFields(&structObj, StructType::staticType(), ar);
    }
};