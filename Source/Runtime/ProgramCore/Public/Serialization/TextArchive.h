/*!
 * \file TextArchive.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Serialization/ArchiveBase.h"

// Will be serialized with null terminated texts
class PROGRAMCORE_EXPORT TextArchive : public ArchiveBase
{
protected:

public:
    /* ArchiveBase overrides */
    ArchiveBase& serialize(bool& value) override;
    ArchiveBase& serialize(double& value) override;
    ArchiveBase& serialize(float& value) override;
    ArchiveBase& serialize(int64& value) override;
    ArchiveBase& serialize(int32& value) override;
    ArchiveBase& serialize(int16& value) override;
    ArchiveBase& serialize(int8& value) override;
    ArchiveBase& serialize(uint64& value) override;
    ArchiveBase& serialize(uint32& value) override;
    ArchiveBase& serialize(uint16& value) override;
    ArchiveBase& serialize(uint8& value) override;
    ArchiveBase& serialize(String& value) override;
    ArchiveBase& serialize(TChar* value) override;
    /* Overrides ends */
};