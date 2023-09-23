/*!
 * \file FileUtils.cs
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

﻿using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace GameBuilder.BuildFileUtils
{
    static class FileUtils
    {
        public static string GetRelativePath(string fullPath, string basePath)
        {
            if (!basePath.EndsWith("\\"))
                basePath += "\\";

            Uri baseUri = new Uri(basePath);
            Uri fullUri = new Uri(fullPath);

            Uri relativeUri = baseUri.MakeRelativeUri(fullUri);

            return relativeUri.ToString().Replace("/", "\\");
        }
        // c:/xyz/abc/../../file.txt
        // gives c:/file.txt
        public static string RemoveParentRedirector(string absPath)
        {
            string outPath = absPath.Replace("/", "\\");
            string[] pathFolders = outPath.Split('\\');
            int parentDirNum = 0;
            StringBuilder sanitizedPath = new StringBuilder();
            for(int i = pathFolders.Length - 1; i >= 0; --i)
            {
                if(pathFolders[i].Equals(".."))
                {
                    ++parentDirNum;
                }
                else 
                {
                    if(parentDirNum == 0)
                    {
                        sanitizedPath.Insert(0, $"{pathFolders[i]}{Path.DirectorySeparatorChar}");
                    }
                    else
                    {
                        --parentDirNum;
                    }
                }
            }
            // Remove last "/"
            sanitizedPath.Remove(sanitizedPath.Length - 1, 1);

            outPath = sanitizedPath.ToString();
            return outPath;
        }

        public static string GetOrCreateDir(string path)
        {
            string directory;
            if(Path.HasExtension(path))
            {
                directory = Path.GetDirectoryName(path);
            }
            else
            {
                directory = path;
            }

            if(!Directory.Exists(directory))
            {
                Directory.CreateDirectory(directory);
            }
            return directory;
        }
    }
}
