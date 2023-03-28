/*!
 * \file ShaderCompiler.cs
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

﻿using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace GameBuilder.ShaderCompiling
{
    public struct IntermediateTargetFile
    {
        public string targetIntermFile;
        // Log output from individual shader stage's compilation
        public string logFile;
        // SPIRV shader disassembled output
        public string disasmFile;
    }

    public class ShaderCompiler
    {
        protected string compilerDir;
        protected string intermediateOutDir;
        protected string compileErrors;

        public ShaderCompiler(string compilerPath,string intermediatePath)
        {
            intermediateOutDir = Path.Combine(intermediatePath, "Shaders");
            compilerDir = compilerPath;
            compileErrors = null;
        }
        public virtual bool compile(Dictionary<string,List<string>> consoleArgs)
        {
            return false;
        }

        public string getError()
        {
            return compileErrors;
        }
    }
}
