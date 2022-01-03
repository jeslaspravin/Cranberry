#pragma once
#include "String/String.h"
#include "Types/Time.h"

#include <map>


class FileChangesTracker
{
private:
    const String FILE_NAME = "FileManifest.manifest";

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
    // clears files not present in this list from tracked entry
    void intersectFiles(const std::vector<String>& srcfilePaths);
};