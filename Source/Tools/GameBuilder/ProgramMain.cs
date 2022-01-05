/*!
 * \file ProgramMain.cs
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

﻿using GameBuilder.BuildFileUtils;
using GameBuilder.Logger;
using GameBuilder.Modes;
using System.Collections.Generic;
using System.Diagnostics;

namespace GameBuilder
{
    enum ExitCode : int
    {
        Success = 0,
        MinorError = 1
    }

    class ProgramMain
    {
        private const string MODE = "mode";
        private const string COMPILE_SHADER = "shaderCompile";

        static void parseConsoleArgs(string[] args, ref Dictionary<string, List<string>> outArgs)
        {
            List<string> flags = new List<string>();
            for (int i = 0; i < args.Length; ++i)
            {
                if (args[i].StartsWith("-"))
                {
                    string currentArg = args[i];
                    do
                    {
                        currentArg = currentArg.Remove(0, 1);
                    }
                    while (currentArg.StartsWith("-"));

                    List<string> values = new List<string>();
                    int valueIndexOffset = 1;
                    while ((valueIndexOffset + i) < args.Length && !args[i + valueIndexOffset].StartsWith("-"))
                    {
                        string val = args[i + valueIndexOffset];
                        values.Add(val);
                        valueIndexOffset++;
                    }

                    if (values.Count == 0)
                    {
                        flags.Add(currentArg);
                    }
                    else
                    {
                        outArgs.Add(currentArg, values);
                    }
                    i += valueIndexOffset - 1;
                }
            }
            outArgs.Add("flags", flags);

            LoggerUtils.Log("Console args : ");
            foreach (KeyValuePair<string, List<string>> arg in outArgs)
            {
                LoggerUtils.Log("\t{0} :", arg.Key);
                foreach (string value in arg.Value)
                {
                    LoggerUtils.Log("\t\t{0}", value);
                }
            }
        }

        static int Main(string[] args)
        {
            LoggerUtils.Log("{0} using {1}", Directories.APP_NAME, Directories.BUILDER_EXE);
            LoggerUtils.Log("Engine root {0}", Directories.ENGINE_ROOT);

            Dictionary<string, List<string>> consoleArgs = new Dictionary<string, List<string>>();
            parseConsoleArgs(args, ref consoleArgs);

            ProgramMode mode = null;
            if (consoleArgs.ContainsKey(MODE))
            {
                switch (consoleArgs[MODE][0])
                {
                    case COMPILE_SHADER:
                        mode = new CompileShadersMode();
                        break;
                    default:
                        mode = new ProgramMode();
                        break;
                }

                Stopwatch stopwatch = new Stopwatch();
                stopwatch.Start();
                if (mode.execute(consoleArgs) != ModeExecutionResult.Success)
                {
                    LoggerUtils.Error("{0} failed", consoleArgs[MODE][0]);
                    return (int)ExitCode.MinorError;
                }
                else
                {
                    stopwatch.Stop();
                    LoggerUtils.Log("Success : {0} completed in {1:0.000} minutes", consoleArgs[MODE][0], stopwatch.Elapsed.TotalMinutes);
                }
            }
            else
            {
                LoggerUtils.Error("No mode selected exiting, expected -{0} with mode [{1}]", MODE, COMPILE_SHADER);
            }

            LoggerUtils.waitUntilTasksFinished();
            return (int)ExitCode.Success;
        }
    }
}
