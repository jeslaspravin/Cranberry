/*!
 * \file FileChangesTracker.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "String/String.h"
#include "Types/Time.h"

#include <map>


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
    FileChangesTracker(const String name, const String& directory, const String& intermediateDir);

    ~FileChangesTracker();

    // return true if file is actually newer
    bool isTargetOutdated(const String& absPath, const std::vector<String>& outputFiles) const;
    bool updateNewerFile(const String& absPath, const std::vector<String>& outputFiles);
    // clears files not present in this list from tracked entry, and returns list of removed files
    std::vector<String> filterIntersects(const std::vector<String>& srcfilePaths);
};