using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace GameBuilder.ShaderCompiling
{
    public struct TargetFile
    {
        public string targetOutputFile;
        public string logFile;
    }

    public class ShaderCompiler
    {
        protected string compilerDir;
        protected string outputDir;

        public ShaderCompiler(string compilerPath,string targetPath)
        {
            outputDir = Path.Combine(targetPath, "Shaders");
            compilerDir = compilerPath;

            if(!Directory.Exists(outputDir))
            {
                Directory.CreateDirectory(outputDir);
            }
        }
        public virtual bool compile(Dictionary<string,List<string>> consoleArgs)
        {
            return false;
        }
    }
}
