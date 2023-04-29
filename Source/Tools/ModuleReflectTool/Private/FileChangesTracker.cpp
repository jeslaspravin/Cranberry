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
#include "Types/Platform/LFS/PathFunctions.h"

#include <unordered_set>

FileChangesTracker::FileChangesTracker(const String name, const String &directory, const String &intermediateDir)
    : trackerManifestName(name + FILE_NAME)
    , folderPath(directory)
    , writePath(intermediateDir)
{
    fatalAssertf(FileSystemFunctions::dirExists(folderPath.getChar()), "Tracking base directory {} is not valid", folderPath);
    String manifestContent;
    String manifestFile = PathFunctions::combinePath(writePath, trackerManifestName);
    if (FileSystemFunctions::fileExists(manifestFile.getChar()) && FileHelper::readString(manifestContent, manifestFile))
    {
        std::vector<StringView> readLines = manifestContent.splitLines();
        for (StringView line : readLines)
        {
            std::vector<StringView> fileEntry = String::split(line, TCHAR("="));
            fatalAssertf(fileEntry.size() == 2, "Cannot parse file timestamp from {}", line);

            fileLastTimestamp[fileEntry[0]] = std::stoll(String(fileEntry[1]));
        }
    }
}

FileChangesTracker::~FileChangesTracker()
{
    std::vector<String> manifestEntries(fileLastTimestamp.size());
    int32 i = 0;
    for (const auto &fileEntry : fileLastTimestamp)
    {
        manifestEntries[i] = STR_FORMAT(TCHAR("{}={}"), fileEntry.first, fileEntry.second);
        ++i;
    }

    String manifestFileContent = String::join(manifestEntries.cbegin(), manifestEntries.cend(), LINE_FEED_TCHAR);
    FileHelper::writeString(manifestFileContent, PathFunctions::combinePath(writePath, trackerManifestName));
}

bool FileChangesTracker::isTargetOutdated(StringView absPath, ArrayView<StringView> outputFiles) const
{
    PlatformFile srcFile(absPath);
    if (!srcFile.exists())
    {
        return false;
    }

    TickRep ts = srcFile.lastWriteTimeStamp();
    String relPath = PathFunctions::toRelativePath(absPath, folderPath);
    std::map<String, TickRep>::const_iterator itr = fileLastTimestamp.find(relPath);
    if (itr != fileLastTimestamp.end())
    {
        const std::pair<const String, TickRep> &fileEntry = (*itr);

        // Output file must be newer than src file
        bool bIsAllOutsValid = true;
        for (const StringView &targetFilePath : outputFiles)
        {
            PlatformFile targetFile(targetFilePath);
            bIsAllOutsValid = bIsAllOutsValid && targetFile.exists() && targetFile.lastWriteTimeStamp() > ts;
        }

        // If outputs are valid however It is not recorded we still consider that outdated
        if (fileEntry.second >= ts && bIsAllOutsValid)
        {
            return false;
        }
    }
    return true;
}

bool FileChangesTracker::updateNewerFile(StringView absPath, ArrayView<StringView> outputFiles)
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

std::vector<String> FileChangesTracker::filterIntersects(ArrayView<String> srcfilePaths)
{
    std::vector<String> deletedSrcs;
    deletedSrcs.reserve(srcfilePaths.size());
    std::unordered_set<String> relSrcFiles;
    relSrcFiles.reserve(srcfilePaths.size());
    for (const String &srcFilePath : srcfilePaths)
    {
        relSrcFiles.insert(PathFunctions::toRelativePath(srcFilePath, folderPath));
    }

    for (auto itr = fileLastTimestamp.begin(); itr != fileLastTimestamp.end();)
    {
        if (relSrcFiles.contains(itr->first))
        {
            ++itr;
        }
        else
        {
            deletedSrcs.emplace_back(PathFunctions::toAbsolutePath(itr->first, folderPath));
            fileLastTimestamp.erase(itr++);
        }
    }
    return deletedSrcs;
}
