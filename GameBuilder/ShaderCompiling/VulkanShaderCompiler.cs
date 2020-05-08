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
    public struct TargetFile
    {
        public struct IntermediateReference
        {
            public string shaderType;
            public int intermediateIdx;
        }
        public TargetFile(string targetShader, string targetReflection)
        {
            intermediateTargetsRef = new List<IntermediateReference>();
            targetShaderFile = targetShader;
            targetReflectionFile = targetReflection;
        }

        public List<IntermediateReference> intermediateTargetsRef;
        public string targetShaderFile;
        public string targetReflectionFile;
    }
    public class VulkanShaderCompiler : ShaderCompiler
    {
        private string spirvReflectorDir;

        private string shaderSrcFolder;
        private string shaderTargetDir;
        private Dictionary<string, List<Tuple<string, IntermediateTargetFile>>> shaders;// Maps shader type to list of shaders of that type
        private Dictionary<string, TargetFile> shaderTargets;// Maps each individual pipelines to shaders of that pipeline


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
        private const string COMPILE_COMMAND = "@\"{0}\" -{1} {2} -{3} {4} --source-entrypoint main -H -o \"{5}\" \"{6}\"";
        private const string SPIRV_REFLECTION_EXE = "SpirvShaderReflection.exe";

        public VulkanShaderCompiler(string compilerPath, string intermediatePath, string targetPath) :
            base(compilerPath, intermediatePath)
        {
            if(compilerDir == null)
            {
                compilerDir = Path.Combine(System.Environment.GetEnvironmentVariable("VULKAN_SDK"),"bin");
            }
#if DEBUG
            spirvReflectorDir = Path.Combine(Directories.BINARIES_ROOT, "SpirvShaderReflection/x64/Debug");
#else
            spirvReflectorDir = Path.Combine(Directories.BINARIES_ROOT, "SpirvShaderReflection/x64/Release");
#endif
            shaderSrcFolder = Path.Combine(Directories.ENGINE_ROOT, "Source/EngineShaders");
            shaderTargetDir = Path.Combine(targetPath, "Shaders");
            FileUtils.GetOrCreateDir(shaderTargetDir);

            shaders = new Dictionary<string, List<Tuple<string, IntermediateTargetFile>>>();
            shaders.Add("frag", new List<Tuple<string, IntermediateTargetFile>>());
            shaders.Add("vert", new List<Tuple<string, IntermediateTargetFile>>());
            shaders.Add("tesc", new List<Tuple<string, IntermediateTargetFile>>());
            shaders.Add("tese", new List<Tuple<string, IntermediateTargetFile>>());
            shaders.Add("geom", new List<Tuple<string, IntermediateTargetFile>>());
            shaders.Add("comp", new List<Tuple<string, IntermediateTargetFile>>());

            shaderTargets = new Dictionary<string ,TargetFile>();

            foreach (KeyValuePair<string,List<Tuple<string,IntermediateTargetFile>>> shaderType in shaders)
            {
                string expr = string.Format("*.{0}.glsl",shaderType.Key);
                foreach (string shader in Directory.GetFiles(shaderSrcFolder,expr,
                                                SearchOption.AllDirectories))
                {
                    IntermediateTargetFile intermediateFileDesc = new IntermediateTargetFile();
                    intermediateFileDesc.targetOutputFile = Path.Combine(intermediateOutDir
                        , FileUtils.GetRelativePath(Path.GetDirectoryName(shader), shaderSrcFolder), Path.GetFileNameWithoutExtension(shader) + ".shader");
                    intermediateFileDesc.logFile = Path.Combine(intermediateOutDir
                        , FileUtils.GetRelativePath(Path.GetDirectoryName(shader), shaderSrcFolder), Path.GetFileNameWithoutExtension(shader) + ".log");

                    // To remove both extension and shader type from shader file name
                    string shaderName = Path.GetFileNameWithoutExtension(Path.GetFileNameWithoutExtension(shader));
                    if(!shaderTargets.ContainsKey(shaderName))
                    {
                        string targetBase = Path.Combine(shaderTargetDir, shaderName);
                        TargetFile targetFileDesc = new TargetFile(targetBase + ".shader",targetBase + ".ref");

                        shaderTargets.Add(shaderName, targetFileDesc);
                    }
                    TargetFile.IntermediateReference intermediateRef = new TargetFile.IntermediateReference();
                    intermediateRef.shaderType = shaderType.Key;
                    intermediateRef.intermediateIdx = shaderType.Value.Count;
                    shaderTargets[shaderName].intermediateTargetsRef.Add(intermediateRef);

                    FileUtils.GetOrCreateDir(intermediateFileDesc.targetOutputFile);
                    FileUtils.GetOrCreateDir(intermediateFileDesc.logFile);

                    shaderType.Value.Add(new Tuple<string, IntermediateTargetFile>(shader,intermediateFileDesc));
                }
            }
        }
        public override bool compile(Dictionary<string, List<string>> consoleArgs)
        {
            bool bSuccess = true;
            string compiler = Path.Combine(compilerDir, "glslangValidator.exe");
            string spirvReflector = Path.Combine(spirvReflectorDir, SPIRV_REFLECTION_EXE);
            if (!File.Exists(compiler) || !File.Exists(spirvReflector))
            {
                LoggerUtils.Error("Tools necessary for compiling does not exists compiler {0} and spirv reflection {1}", compilerDir, spirvReflectorDir);
                return false;
            }

            foreach (KeyValuePair<string, List<Tuple<string, IntermediateTargetFile>>> shaderType in shaders)
            {
                VulkanShaderArg shaderArg = SHADER_ARGS[shaderType.Key];
                foreach(Tuple<string,IntermediateTargetFile> shaderFile in shaderType.Value)
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

                    if (executionResult.Length > 0 && executionResult.Contains("ERROR:"))
                    {
                        LoggerUtils.Log(executionResult);
                        bSuccess = false;
                    }
                    File.WriteAllText(shaderFile.Item2.logFile, executionResult,Encoding.UTF8);
                }
            }

            if(bSuccess)
            {
                foreach(KeyValuePair<string,TargetFile> pipelinePair in  shaderTargets)
                {
                    LoggerUtils.Log("Reflecting shader pipeline {0}", pipelinePair.Key);

                    StringBuilder cmdBuilder = new StringBuilder($"@\"{spirvReflector}\"");
                    foreach(TargetFile.IntermediateReference intermediateTargetRef in pipelinePair.Value.intermediateTargetsRef)
                    {
                        IntermediateTargetFile intermediateShaderTarget = shaders[intermediateTargetRef.shaderType][intermediateTargetRef.intermediateIdx].Item2;
                        cmdBuilder.Append($" \"{intermediateShaderTarget.targetOutputFile}\"");
                    }
                    // at n-2th arg reflection file
                    cmdBuilder.Append($" \"{pipelinePair.Value.targetReflectionFile}\"");
                    // at n-1st arg combined shader file
                    cmdBuilder.Append($" \"{pipelinePair.Value.targetShaderFile}\"");

                    string cmd = cmdBuilder.ToString();
                    string executionResult;                    
                    if(!ProcessUtils.ExecuteCommand(out executionResult, cmd))
                    {
                        LoggerUtils.Error("Failed executing command {0}", cmd);
                        bSuccess = false;
                    }
                    LoggerUtils.Log("Execution log : \n{0}", executionResult);

                    if(executionResult.Contains("ERROR:"))
                    {
                        bSuccess = false;
                    }
                }
            }
            return bSuccess;
        }
    }
}
