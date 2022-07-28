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

    // Out from AssetImporter
    void *optionsStruct;
    CBEClass structType;
};

class META_ANNOTATE() AssetImporterBase : public cbe::Object
{
    GENERATED_CODES()
public:
    bool supportsImporting(ImportOption &inOutOptions) { return false; }
    cbe::Object *tryImporting(ImportOption &inOutOptions) const { return nullptr; }
};
