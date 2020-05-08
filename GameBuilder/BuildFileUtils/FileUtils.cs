using System;
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
