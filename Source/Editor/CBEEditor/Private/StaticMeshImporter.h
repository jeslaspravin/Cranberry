/*!
 * \file StaticMeshImporter.h
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "AssetImporter.h"

#include "StaticMeshImporter.gen.h"

struct StaticMeshImportOptions
{
    GENERATED_CODES()

    META_ANNOTATE()
    bool bImportAsScene = false;

    META_ANNOTATE()
    bool bImportAllMesh = false;

    META_ANNOTATE()
    bool bLoadSmoothed = false;

    META_ANNOTATE()
    float smoothingAngle = 35.0f;

    META_ANNOTATE()
    bool bFromYUp = false;

} META_ANNOTATE(NoExport);

class ObjStaticMeshImporter : public AssetImporterBase
{
    GENERATED_CODES()
public:
    StaticMeshImportOptions options;

    bool supportsImporting(ImportOption &inOutOptions) override;
    std::vector<cbe::Object *> tryImporting(const ImportOption &importOptions) const override;

private:
    void makePackageUnique(String &inOutPackageDir) const;

} META_ANNOTATE(NoExport);
