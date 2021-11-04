using GameBuilder.BuildFileUtils;
using GameBuilder.Logger;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

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
        private FileChangesTracker compiledFilesManifest;

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

        public VulkanShaderCompiler(string compilerPath, string intermediatePath, string targetPath) 
            : base(compilerPath, intermediatePath)
        {
            if (compilerDir == null)
            {
                compilerDir = Path.Combine(System.Environment.GetEnvironmentVariable("VULKAN_SDK"), "bin");
            }
            spirvReflectorDir = Directories.TOOLS_BINARIES_ROOT;
            shaderSrcFolder = Path.Combine(Directories.ENGINE_ROOT, "Source/Runtime/EngineShaders");
            shaderTargetDir = Path.Combine(targetPath, "Shaders");
            FileUtils.GetOrCreateDir(shaderTargetDir);

            compiledFilesManifest = new FileChangesTracker("ShaderIntmdt", shaderSrcFolder, intermediatePath);

            shaders = new Dictionary<string, List<Tuple<string, IntermediateTargetFile>>>();
            shaders.Add("frag", new List<Tuple<string, IntermediateTargetFile>>());
            shaders.Add("vert", new List<Tuple<string, IntermediateTargetFile>>());
            shaders.Add("tesc", new List<Tuple<string, IntermediateTargetFile>>());
            shaders.Add("tese", new List<Tuple<string, IntermediateTargetFile>>());
            shaders.Add("geom", new List<Tuple<string, IntermediateTargetFile>>());
            shaders.Add("comp", new List<Tuple<string, IntermediateTargetFile>>());

            shaderTargets = new Dictionary<string, TargetFile>();

            foreach (KeyValuePair<string, List<Tuple<string, IntermediateTargetFile>>> shaderType in shaders)
            {
                string expr = string.Format("*.{0}.glsl", shaderType.Key);
                foreach (string shader in Directory.GetFiles(shaderSrcFolder, expr,
                                                SearchOption.AllDirectories))
                {
                    IntermediateTargetFile intermediateFileDesc = new IntermediateTargetFile();
                    intermediateFileDesc.targetOutputFile = Path.Combine(intermediateOutDir
                        , FileUtils.GetRelativePath(Path.GetDirectoryName(shader), shaderSrcFolder), Path.GetFileNameWithoutExtension(shader) + ".shader");
                    intermediateFileDesc.logFile = Path.Combine(intermediateOutDir
                        , FileUtils.GetRelativePath(Path.GetDirectoryName(shader), shaderSrcFolder), Path.GetFileNameWithoutExtension(shader) + ".log");

                    // To remove both extension and shader type from shader file name
                    string shaderName = Path.GetFileNameWithoutExtension(Path.GetFileNameWithoutExtension(shader));
                    if (!shaderTargets.ContainsKey(shaderName))
                    {
                        string targetBase = Path.Combine(shaderTargetDir, shaderName);
                        TargetFile targetFileDesc = new TargetFile(targetBase + ".shader", targetBase + ".ref");

                        shaderTargets.Add(shaderName, targetFileDesc);
                    }
                    TargetFile.IntermediateReference intermediateRef = new TargetFile.IntermediateReference();
                    intermediateRef.shaderType = shaderType.Key;
                    intermediateRef.intermediateIdx = shaderType.Value.Count;
                    shaderTargets[shaderName].intermediateTargetsRef.Add(intermediateRef);

                    FileUtils.GetOrCreateDir(intermediateFileDesc.targetOutputFile);
                    FileUtils.GetOrCreateDir(intermediateFileDesc.logFile);

                    shaderType.Value.Add(new Tuple<string, IntermediateTargetFile>(shader, intermediateFileDesc));
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

            HashSet<string> compiledShaders = new HashSet<string>();
            bSuccess = compileAllShadersToSpirV(compiler, ref compiledShaders);

            if (compiledShaders.Count != 0)
            {
                bSuccess = reflectCompiledShaders(spirvReflector, compiledShaders) && bSuccess;
            }
            return bSuccess;
        }

        bool compileAllShadersToSpirV(in string compiler, ref HashSet<string> outCompiledShaders)
        {
            bool bSuccess = true;
            StringBuilder errors = new StringBuilder();
            foreach (KeyValuePair<string, List<Tuple<string, IntermediateTargetFile>>> shaderType in shaders)
            {
                VulkanShaderArg shaderArg = SHADER_ARGS[shaderType.Key];
                foreach (Tuple<string, IntermediateTargetFile> shaderFile in shaderType.Value)
                {
                    // To remove both extension and shader type from shader file name
                    string shaderName = Path.GetFileNameWithoutExtension(Path.GetFileNameWithoutExtension(shaderFile.Item1));

                    // Check if needs recompile
                    List<string> targetFiles = new List<string>();
                    targetFiles.Add(shaderFile.Item2.targetOutputFile);
                    // Check target from intermediate as well
                    if(shaderTargets.ContainsKey(shaderName))
                    {
                        targetFiles.Add(shaderTargets[shaderName].targetReflectionFile);
                        targetFiles.Add(shaderTargets[shaderName].targetShaderFile);
                    }
                    if (!compiledFilesManifest.updateNewerFile(shaderFile.Item1, targetFiles))
                    {
                        continue;
                    }

                    string cmd = string.Format(COMPILE_COMMAND
                        , compiler
                        , shaderArg.stageType.argKey, shaderArg.stageType.argValue
                        , shaderArg.entry.argKey, shaderArg.entry.argValue
                        , shaderFile.Item2.targetOutputFile
                        , shaderFile.Item1
                    );

                    LoggerUtils.Log("Compiling shader {0}", shaderFile.Item1);
                    LoggerUtils.Log("Root : {0}\nCompile cmd : {1}", Directories.TOOLS_BINARIES_ROOT, cmd);

                    string executionResult;

                    if (!ProcessUtils.ExecuteCommand(out executionResult, cmd, Directories.TOOLS_BINARIES_ROOT)
                        || (executionResult.Length > 0 && executionResult.Contains("ERROR:")))
                    {
                        LoggerUtils.Log(executionResult);
                        bSuccess = false;
                        errors.Append(executionResult);
                    }
                    else
                    {
                        outCompiledShaders.Add(shaderName);
                    }
                    File.WriteAllText(shaderFile.Item2.logFile, executionResult, Encoding.UTF8);
                }
            }
            if(!bSuccess)
            {
                compileErrors = errors.ToString();
            }
            return bSuccess;
        }

        bool reflectCompiledShaders(in string spirvReflector, in HashSet<string> compiledShaders)
        {
            bool bSuccess = true;
            foreach (string shaderName in compiledShaders)
            {
                if (!shaderTargets.ContainsKey(shaderName))
                {
                    LoggerUtils.Error("Targets not found for shader {0}", shaderName);
                }

                TargetFile targetInfo = shaderTargets[shaderName];
                LoggerUtils.Log("Reflecting shader pipeline {0}", shaderName);

                StringBuilder cmdBuilder = new StringBuilder($"@\"{spirvReflector}\"");
                foreach (TargetFile.IntermediateReference intermediateTargetRef in targetInfo.intermediateTargetsRef)
                {
                    IntermediateTargetFile intermediateShaderTarget = shaders[intermediateTargetRef.shaderType][intermediateTargetRef.intermediateIdx].Item2;
                    cmdBuilder.Append($" \"{intermediateShaderTarget.targetOutputFile}\"");
                }
                // at n-2th arg reflection file
                cmdBuilder.Append($" \"{targetInfo.targetReflectionFile}\"");
                // at n-1st arg combined shader file
                cmdBuilder.Append($" \"{targetInfo.targetShaderFile}\"");

                string cmd = cmdBuilder.ToString();
                string executionResult;
                LoggerUtils.Log("Reflecting cmd : {0}", cmd);
                if (!ProcessUtils.ExecuteCommand(out executionResult, cmd))
                {
                    LoggerUtils.Error("Failed executing command {0}", cmd);
                    bSuccess = false;
                }
                LoggerUtils.Log("Execution log : \n{0}", executionResult);

                if (executionResult.Contains("ERROR:"))
                {
                    bSuccess = false;
                }
            }
            return bSuccess;
        }
    }
}
