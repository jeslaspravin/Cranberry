/*!
 * \file CBEObjectHelpers.cpp
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "CBEObjectHelpers.h"
#include "CBEPackage.h"

namespace CBE
{
void ObjectAllocatorBase::constructDefault(void *objPtr, AllocIdx allocIdx, CBEClass clazz) const
{
    // Direct call to object construction routine to skip getting allocator that happens when constructing using CBEObjectConstructionPolicy
    // Default ctor
    const GlobalFunctionWrapper *ctor = PropertyHelper::findMatchingCtor<void *>(clazz);
    alertIf(ctor, "Default constructor not found to construct defaul object");

    Object *object = reinterpret_cast<Object *>(objPtr);

    // Object's data must be populated even before constructor is called
    INTERNAL_ObjectCoreAccessors::setAllocIdx(object, allocIdx);
    INTERNAL_ObjectCoreAccessors::getFlags(object) |= EObjectFlagBits::Default | EObjectFlagBits::RootObject;
    INTERNAL_ObjectCoreAccessors::setOuterAndName(
        object, PropertyHelper::getValidSymbolName(clazz->nameString) + TCHAR("_Default"), nullptr, clazz
    );

    if (ctor)
    {
        object = ctor->invokeUnsafe<Object *, void *>(objPtr);
    }
}

void INTERNAL_destroyCBEObject(CBE::Object *obj)
{
    CBEClass clazz = obj->getType();
    obj->destroyObject();
    static_cast<const GlobalFunctionWrapper *>(clazz->destructor->funcPtr)->invokeUnsafe<void>(obj);
}
} // namespace CBE