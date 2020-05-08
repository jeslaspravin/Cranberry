using GameBuilder.ShaderCompiling;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace GameBuilder.Modes
{
    enum ReleaseConfigurations
    {
        Release,
        Debug
    }
    class CompileShadersMode : ProgramMode
    {
        private const string RENDER_API = "api";
        private const string COMPILER_DIRECTORY = "binPath";
        private const string INTERMEDIATE_DIR = "intermediatePath";
        private const string TARGET_DIR = "targetPath";

        private const string API_VULKAN = "vulkan";
        public override ModeExecutionResult execute(Dictionary<string,List<string>> consoleArgs)
        {
            if(consoleArgs.ContainsKey(RENDER_API) && consoleArgs.ContainsKey(TARGET_DIR) && consoleArgs.ContainsKey(INTERMEDIATE_DIR))
            {
                ShaderCompiler compiler = null;
                string compilerPath = null;
                if(consoleArgs.ContainsKey(COMPILER_DIRECTORY))
                {
                    compilerPath = consoleArgs[COMPILER_DIRECTORY][0];
                }

                switch(consoleArgs[RENDER_API][0])
                {
                    case API_VULKAN:
                        compiler = new VulkanShaderCompiler(compilerPath, consoleArgs[INTERMEDIATE_DIR][0], consoleArgs[TARGET_DIR][0]);
                        break;
                    default:
                        return ModeExecutionResult.Failure;
                }

                if(compiler.compile(consoleArgs))
                {
                    return ModeExecutionResult.Success;
                }
            }
            return ModeExecutionResult.Failure;
        }
    }
}
