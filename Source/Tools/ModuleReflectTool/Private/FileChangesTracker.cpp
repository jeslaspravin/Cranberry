#include "FileChangesTracker.h"
#include "Types/Platform/LFS/PlatformLFS.h"

#include <unordered_set>

FileChangesTracker::FileChangesTracker(const String name, const String& directory, const String& intermediateDir)
    : trackerManifestName(name + FILE_NAME)
    , folderPath(directory)
    , writePath(intermediateDir)
{
    fatalAssert(PlatformFile(folderPath).exists(), "%s() : Tracking base directory %s is not valid", __func__, folderPath);
    PlatformFile existingManifestFile(PathFunctions::combinePath(writePath, trackerManifestName));
    existingManifestFile.setFileFlags(EFileFlags::Read);
    existingManifestFile.setSharingMode(EFileSharing::ReadOnly);
    existingManifestFile.setCreationAction(EFileFlags::OpenExisting);
    if (existingManifestFile.exists() && existingManifestFile.openFile())
    {
        String manifestContent;
        existingManifestFile.read(manifestContent);
        std::vector<std::string_view> readLines = manifestContent.splitLines();
        for(String line : readLines)
        {
            std::vector<String> fileEntry = String::split(line, "=");
            fatalAssert(fileEntry.size() == 2, "%s() : Cannot parse file timestamp from %s", __func__, line);
            fileLastTimestamp[fileEntry[0]] = std::stoll(fileEntry[1]);
        }
        existingManifestFile.closeFile();
    }
}

FileChangesTracker::~FileChangesTracker()
{
    std::vector<String> manifestEntries(fileLastTimestamp.size());
    int32 i = 0;
    for (const auto& fileEntry : fileLastTimestamp)
    {
        manifestEntries[i] = StringFormat::format("%s=%lld", fileEntry.first, fileEntry.second);
        ++i;
    }

    PlatformFile manifestFile(PathFunctions::combinePath(writePath, trackerManifestName));
    manifestFile.setFileFlags(EFileFlags::Write);
    manifestFile.setSharingMode(EFileSharing::ReadOnly);
    manifestFile.setCreationAction(EFileFlags::OpenAlways);
    if (manifestFile.openOrCreate())
    {
        String manifestFileContent = String::join(manifestEntries.cbegin(), manifestEntries.cend(), LINE_FEED_CHAR);
        manifestFile.write(ArrayView<uint8>(reinterpret_cast<uint8*>(manifestFileContent.data()), manifestFileContent.length()));
        manifestFile.closeFile();
    }
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
