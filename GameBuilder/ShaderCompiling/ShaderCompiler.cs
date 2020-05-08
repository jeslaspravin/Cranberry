using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace GameBuilder.ShaderCompiling
{
    public struct IntermediateTargetFile
    {
        public string targetOutputFile;
        public string logFile;
    }

    public class ShaderCompiler
    {
        protected string compilerDir;
        protected string intermediateOutDir;

        public ShaderCompiler(string compilerPath,string intermediatePath)
        {
            intermediateOutDir = Path.Combine(intermediatePath, "Shaders");
            compilerDir = compilerPath;
        }
        public virtual bool compile(Dictionary<string,List<string>> consoleArgs)
        {
            return false;
        }
    }
}
