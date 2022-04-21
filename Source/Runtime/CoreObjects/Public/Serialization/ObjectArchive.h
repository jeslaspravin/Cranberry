/*!
 * \file ObjectArchive.h
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Serialization/ArchiveBase.h"
#include "CoreObjectsExports.h"

namespace CBE
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
    ArchiveBase *innerArchive;

public:
    ObjectArchive() = default;
    ObjectArchive(ArchiveBase *inner)
        : innerArchive(inner)
    {}

    void setInnerArchive(ArchiveBase *inner) { innerArchive = inner; }

    virtual ObjectArchive &serialize(CBE::Object *&obj);

    /* ArchiveBase overrides */
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

template <IsArchiveType ArchiveType>
requires std::derived_from<ArchiveType, ObjectArchive> ArchiveType &operator<<(ArchiveType &archive, CBE::Object *&value)
{
    ObjectArchive &objArchive = static_cast<ObjectArchive &>(archive);
    objArchive.serialize(value);
    return archive;
}