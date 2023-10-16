/*!
 * \file ObjectArchive.h
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Serialization/ArchiveBase.h"
#include "Property/PropertyHelper.h"
#include "CoreObjectsExports.h"

namespace cbe
{
class Object;
}

/*!
 * \class ObjectArchive is just a shell around archive base to provide an interface for object serialization.
 *  Actual serialization will be done by innerArchive. So any options set on this will not be propagated to innerArchive
 *
 * \brief
 *
 * \author Jeslas
 * \date April 2022
 */
class COREOBJECTS_EXPORT ObjectArchive : public ArchiveBase
{
protected:
    ArchiveBase *innerArchive = nullptr;

public:
    ObjectArchive() = default;
    ObjectArchive(ArchiveBase *inner)
        : innerArchive(inner)
    {}

    void setInnerArchive(ArchiveBase *inner) { innerArchive = inner; }

    /**
     * This must be called if cbe::Object * is serialized manually, is not marked with META_ANNOTATE and is possible to be pointing at Transient
     * pointer.
     * Best place to call this function to fix up pointers in postSerialize(ar)
     * objPtr might be not null after serialization, but those values might not be valid pointer unless this function is called
     */
    virtual void relinkSerializedPtr(void **objPtrPtr) const;
    virtual void relinkSerializedPtr(const void **objPtrPtr) const;
    virtual ObjectArchive &serialize(cbe::Object *&obj);

    /* ArchiveBase overrides */

    bool ifSwapBytes() const override { return innerArchive->ifSwapBytes(); }
    bool isLoading() const override { return innerArchive->isLoading(); }
    ArchiveStream *stream() const override { return innerArchive->stream(); }
    uint32 getCustomVersion(uint32 customId) const override { return innerArchive->getCustomVersion(customId); }
    const std::map<uint32, uint32> &getCustomVersions() const override { return innerArchive->getCustomVersions(); }

    ObjectArchive &serialize(bool &value) override
    {
        innerArchive->serialize(value);
        return *this;
    }
    ObjectArchive &serialize(double &value) override
    {
        innerArchive->serialize(value);
        return *this;
    }
    ObjectArchive &serialize(float &value) override
    {
        innerArchive->serialize(value);
        return *this;
    }
    ObjectArchive &serialize(int64 &value) override
    {
        innerArchive->serialize(value);
        return *this;
    }
    ObjectArchive &serialize(int32 &value) override
    {
        innerArchive->serialize(value);
        return *this;
    }
    ObjectArchive &serialize(int16 &value) override
    {
        innerArchive->serialize(value);
        return *this;
    }
    ObjectArchive &serialize(int8 &value) override
    {
        innerArchive->serialize(value);
        return *this;
    }
    ObjectArchive &serialize(uint64 &value) override
    {
        innerArchive->serialize(value);
        return *this;
    }
    ObjectArchive &serialize(uint32 &value) override
    {
        innerArchive->serialize(value);
        return *this;
    }
    ObjectArchive &serialize(uint16 &value) override
    {
        innerArchive->serialize(value);
        return *this;
    }
    ObjectArchive &serialize(uint8 &value) override
    {
        innerArchive->serialize(value);
        return *this;
    }
    ObjectArchive &serialize(String &value) override
    {
        innerArchive->serialize(value);
        return *this;
    }
    ObjectArchive &serialize(TChar *value) override
    {
        innerArchive->serialize(value);
        return *this;
    }
    /* Overrides ends */
};

template <ArchiveTypeName ArchiveType, ReflectClassType ObjectType>
requires (std::derived_from<ArchiveType, ObjectArchive> && std::derived_from<ObjectType, cbe::Object>)
ArchiveType &operator<< (ArchiveType &archive, ObjectType *&value)
{
    ObjectArchive &objArchive = static_cast<ObjectArchive &>(archive);
    cbe::Object **objectPtrPtr = reinterpret_cast<cbe::Object **>(&value);
    objArchive.serialize(*objectPtrPtr);
    return archive;
}