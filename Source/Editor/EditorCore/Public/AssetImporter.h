/*!
 * \file AssetImporter.h
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "CBEObject.h"
#include "EditorCoreExports.h"

#include "AssetImporter.gen.h"

struct ImportOption
{
    String filePath;
    String fileExt;
    // w/o extension
    String fileName;
    String fileDirectory;

    String importContentPath;
    // Path to directory in which to store the package relative to importContentPath
    String relativeDirPath;

    // Out from AssetImporter
    void *optionsStruct;
    CBEClass structType;
};

class META_ANNOTATE() AssetImporterBase : public cbe::Object
{
    GENERATED_CODES()
public:
    constexpr static const uint32 AllocSlotCount = 2;

public:
    virtual bool supportsImporting(ImportOption & /*inOutOptions*/) { return false; }
    virtual std::vector<cbe::Object *> tryImporting(const ImportOption & /*importOptions*/) const { return {}; }
};
