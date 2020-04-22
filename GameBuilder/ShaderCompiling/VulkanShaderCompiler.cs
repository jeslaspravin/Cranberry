using GameBuilder.BuildFileUtils;
using GameBuilder.Logger;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace GameBuilder.ShaderCompiling
{
    public class VulkanShaderCompiler : ShaderCompiler
    {
        private string shaderSrcFolder;
        private Dictionary<string, List<Tuple<string, TargetFile>>> shaders;

        private struct VulkanShaderArg
        {
            public CommandArgument stageType;
            public CommandArgument entry;
            public VulkanShaderArg(CommandArgument stage, CommandArgument entryPt)
            {
                stageType = stage;
                entry = entryPt;
            }
        }

        private static readonly Dictionary<string, VulkanShaderArg> SHADER_ARGS = new Dictionary<string, VulkanShaderArg>{
            { "frag" , new VulkanShaderArg(new CommandArgument("stage", "S", "frag"), new CommandArgument("entry", "e", "mainFS")) },
            { "vert" , new VulkanShaderArg(new CommandArgument("stage", "S", "vert"), new CommandArgument("entry", "e", "mainVS")) },
            { "tese" , new VulkanShaderArg(new CommandArgument("stage", "S", "tese"), new CommandArgument("entry", "e", "mainTE")) },
            { "tesc" , new VulkanShaderArg(new CommandArgument("stage", "S", "tesc"), new CommandArgument("entry", "e", "mainTC")) },
            { "geom" , new VulkanShaderArg(new CommandArgument("stage", "S", "geom"), new CommandArgument("entry", "e", "mainGeo")) },
            { "comp" , new VulkanShaderArg(new CommandArgument("stage", "S", "comp"), new CommandArgument("entry", "e", "mainComp")) }
        };
        private const string COMPILE_COMMAND = "{0} -{1} {2} -{3} {4} --source-entrypoint main -H -o \"{5}\" \"{6}\"";

        public VulkanShaderCompiler(string compilerPath, string targetPath) :
            base(compilerPath, targetPath)
        {
            if(compilerDir == null)
            {
                compilerDir = Path.Combine(System.Environment.GetEnvironmentVariable("VULKAN_SDK"),"bin");
            }
            shaderSrcFolder = Path.Combine(Directories.ENGINE_ROOT, "Source\\EngineShaders");

            shaders = new Dictionary<string, List<Tuple<string, TargetFile>>>();
            shaders.Add("frag", new List<Tuple<string, TargetFile>>());
            shaders.Add("vert", new List<Tuple<string, TargetFile>>());
            shaders.Add("tesc", new List<Tuple<string, TargetFile>>());
            shaders.Add("tese", new List<Tuple<string, TargetFile>>());
            shaders.Add("geom", new List<Tuple<string, TargetFile>>());
            shaders.Add("comp", new List<Tuple<string, TargetFile>>());

            foreach(KeyValuePair<string,List<Tuple<string,TargetFile>>> shaderType in shaders)
            {
                string expr = string.Format("*.{0}.glsl",shaderType.Key);
                foreach (string shader in Directory.GetFiles(shaderSrcFolder,expr,
                                                SearchOption.AllDirectories))
                {
                    TargetFile targetFile = new TargetFile();
                    targetFile.targetOutputFile = Path.Combine(outputDir, Path.GetFileNameWithoutExtension(shader) + ".shader");
                    targetFile.logFile = Path.Combine(outputDir, Path.GetFileNameWithoutExtension(shader) + ".log");

                    shaderType.Value.Add(new Tuple<string, TargetFile>(shader,targetFile));
                }
            }
        }
        public override bool compile(Dictionary<string, List<string>> consoleArgs)
        {
            if(compilerDir == null)
            {
                return false;
            }
            string compiler = Path.Combine(compilerDir, "glslangValidator.exe");

            foreach (KeyValuePair<string, List<Tuple<string, TargetFile>>> shaderType in shaders)
            {
                VulkanShaderArg shaderArg = SHADER_ARGS[shaderType.Key];
                foreach(Tuple<string,TargetFile> shaderFile in shaderType.Value)
                {
                    string cmd = string.Format(COMPILE_COMMAND
                        , compiler
                        , shaderArg.stageType.argKey, shaderArg.stageType.argValue
                        , shaderArg.entry.argKey, shaderArg.entry.argValue
                        , shaderFile.Item2.targetOutputFile
                        , shaderFile.Item1
                    );

                    LoggerUtils.Log("Compiling shader {0}", shaderFile.Item1);

                    string executionResult;
                    ProcessUtils.ExecuteCommand(out executionResult, cmd, Directories.BUILDER_ROOT);

                    executionResult = executionResult.Trim();
                    if (executionResult.Length > 0 && executionResult.Contains("ERROR:"))
                    {
                        LoggerUtils.Log(executionResult);
                    }
                    File.WriteAllText(shaderFile.Item2.logFile, executionResult,Encoding.UTF8);
                }
            }
            return true;
        }
    }
}
