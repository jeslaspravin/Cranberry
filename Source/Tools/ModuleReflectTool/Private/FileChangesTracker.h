/*!
 * \file FileChangesTracker.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "String/String.h"
#include "Types/Containers/ArrayView.h"
#include "Types/Time.h"

#include <map>

/*!
 * \class FileChangesTracker - Tracks a list of source files and its last processed timestamp. If a target output is old or if timestamp
 * recorded is old then target will be considered outdated
 *
 * \brief
 *
 * \author Jeslas
 * \date January 2022
 */
class FileChangesTracker
{
private:
    const String FILE_NAME{ TCHAR("FileManifest.manifest") };

    String trackerManifestName;
    String folderPath;
    String writePath;
    // Folder relative path to
    std::map<String, TickRep> fileLastTimestamp;

public:
    // Files in directory to check
    FileChangesTracker(const String name, const String &directory, const String &intermediateDir);

    ~FileChangesTracker();

    // return true if file is actually newer
    bool isTargetOutdated(StringView absPath, ArrayView<StringView> outputFiles) const;
    bool updateNewerFile(StringView absPath, ArrayView<StringView> outputFiles);
    // clears files not present in this list from tracked entry, and returns list of removed files
    std::vector<String> filterIntersects(ArrayView<String> srcfilePaths) noexcept;
};