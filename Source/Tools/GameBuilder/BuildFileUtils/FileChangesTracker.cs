/*!
 * \file FileChangesTracker.cs
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

﻿using GameBuilder.Logger;
using System;
using System.Collections.Generic;
using System.IO;

namespace GameBuilder.BuildFileUtils
{
    class FileChangesTracker
    {
        private const string FILE_NAME = "FileManifest.manifest";

        private string trackerManifestName;
        private string folderPath;
        private string writePath;
        // Folder relative path to 
        private Dictionary<string, Int64> fileLastTimestamp;

        // Files in directory to check
        public FileChangesTracker(string name, string directory, string intermediateDir)
        {
            trackerManifestName = $"{name}{FILE_NAME}";
            folderPath = FileUtils.GetOrCreateDir(directory);
            writePath = FileUtils.GetOrCreateDir(intermediateDir);
            fileLastTimestamp = new Dictionary<string, Int64>();
            if (File.Exists(Path.Combine(writePath, trackerManifestName)))
            {
                string[] readLines = File.ReadAllLines(Path.Combine(writePath, trackerManifestName));
                foreach(string line in readLines)
                {
                    string[] keyVal = line.Split('=');
                    if(keyVal.Length != 2)
                    {
                        LoggerUtils.Error("Cannot parse file timestamp from {0}", line);
                    }
                    else
                    {
                        fileLastTimestamp.Add(keyVal[0], Int64.Parse(keyVal[1]));
                    }
                }
            }                
        }

        ~FileChangesTracker()
        {
            string manifestFile = Path.Combine(writePath, trackerManifestName);
            if (File.Exists(manifestFile))
                File.Delete(manifestFile);

            string[] linesToWrite = new string[fileLastTimestamp.Count];
            int idx = 0;
            foreach(KeyValuePair<string, Int64> timestamps in fileLastTimestamp)
            {
                linesToWrite[idx] = $"{timestamps.Key}={timestamps.Value}";
                ++idx;
            }
            File.WriteAllLines(manifestFile, linesToWrite);
        }

        // return true if file is actually newer
        public bool updateNewerFile(string absPath, List<string> outputFiles = null)
        {
            if (!File.Exists(absPath))
                return false;

            string relPath = FileUtils.GetRelativePath(FileUtils.RemoveParentRedirector(absPath), folderPath);
            Int64 ts = File.GetLastWriteTime(absPath).Ticks;
            if (fileLastTimestamp.ContainsKey(relPath))
            {
                Int64 currTs = fileLastTimestamp[relPath];

                // Output file must be newer than src file
                bool bIsAllOutsValid = true;
                if (outputFiles != null)
                {
                    foreach (string targetFile in outputFiles)
                    {
                        bIsAllOutsValid = bIsAllOutsValid
                            && File.Exists(targetFile)
                            && File.GetLastWriteTime(targetFile).Ticks > ts;
                    }
                }

                if (currTs >= ts && bIsAllOutsValid)
                {
                    return false;
                }
                fileLastTimestamp[relPath] = ts;
            }
            else
            { 
                fileLastTimestamp.Add(relPath, ts);
            }
            return true;
        }
    }
}
