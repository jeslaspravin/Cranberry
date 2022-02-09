/*!
 * \file FileChangesTracker.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "FileChangesTracker.h"
#include "Types/Platform/LFS/File/FileHelper.h"
#include "Types/Platform/LFS/PlatformLFS.h"

#include <unordered_set>

FileChangesTracker::FileChangesTracker(const String name, const String& directory, const String& intermediateDir)
    : trackerManifestName(name + FILE_NAME)
    , folderPath(directory)
    , writePath(intermediateDir)
{
    fatalAssert(PlatformFile(folderPath).exists(), "%s() : Tracking base directory %s is not valid", __func__, folderPath);
    String manifestContent;
    if (FileHelper::writeString(manifestContent, PathFunctions::combinePath(writePath, trackerManifestName)))
    {
        std::vector<StringView> readLines = manifestContent.splitLines();
        for(String line : readLines)
        {
            std::vector<String> fileEntry = String::split(line, TCHAR("="));
            fatalAssert(fileEntry.size() == 2, "%s() : Cannot parse file timestamp from %s", __func__, line);
            fileLastTimestamp[fileEntry[0]] = std::stoll(fileEntry[1]);
        }
    }
}

FileChangesTracker::~FileChangesTracker()
{
    std::vector<String> manifestEntries(fileLastTimestamp.size());
    int32 i = 0;
    for (const auto& fileEntry : fileLastTimestamp)
    {
        manifestEntries[i] = StringFormat::format(TCHAR("%s=%lld"), fileEntry.first, fileEntry.second);
        ++i;
    }

    String manifestFileContent = String::join(manifestEntries.cbegin(), manifestEntries.cend(), LINE_FEED_CHAR);
    FileHelper::writeString(manifestFileContent, PathFunctions::combinePath(writePath, trackerManifestName));
}

bool FileChangesTracker::isTargetOutdated(const String& absPath, const std::vector<String>& outputFiles) const
{
    PlatformFile srcFile(absPath);
    if (!srcFile.exists())
        return false;

    TickRep ts = srcFile.lastWriteTimeStamp();
    String relPath = PathFunctions::toRelativePath(absPath, folderPath);
    std::map<String, TickRep>::const_iterator itr = fileLastTimestamp.find(relPath);
    if (itr != fileLastTimestamp.end())
    {
        const std::pair<const String, TickRep>& fileEntry = (*itr);

        // Output file must be newer than src file
        bool bIsAllOutsValid = true;
        for (const String& targetFilePath : outputFiles)
        {
            PlatformFile targetFile(targetFilePath);
            bIsAllOutsValid = bIsAllOutsValid
                && targetFile.exists()
                && targetFile.lastWriteTimeStamp() > ts;
        }

        if (fileEntry.second >= ts && bIsAllOutsValid)
        {
            return false;
        }
    }
    return true;
}

bool FileChangesTracker::updateNewerFile(const String& absPath, const std::vector<String>& outputFiles)
{
    if (isTargetOutdated(absPath, outputFiles))
    {
        PlatformFile srcFile(absPath);
        TickRep ts = srcFile.lastWriteTimeStamp();
        String relPath = PathFunctions::toRelativePath(absPath, folderPath);
        fileLastTimestamp[relPath] = ts;
        return true;
    }
    return false;
}

void FileChangesTracker::intersectFiles(const std::vector<String>& srcfilePaths)
{
    std::unordered_set<String> relSrcFiles;
    relSrcFiles.reserve(srcfilePaths.size());
    for (const String& srcFilePath : srcfilePaths)
    {
        relSrcFiles.insert(PathFunctions::toRelativePath(srcFilePath, folderPath));
    }

    for (auto itr = fileLastTimestamp.begin(); itr != fileLastTimestamp.end(); )
    {
        if (relSrcFiles.contains(itr->first))
        {
            ++itr;
        }
        else
        {
            fileLastTimestamp.erase(itr++);
        }
    }
}
