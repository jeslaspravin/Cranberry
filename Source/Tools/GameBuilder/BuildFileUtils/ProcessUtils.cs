/*!
 * \file ProcessUtils.cs
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
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace GameBuilder.BuildFileUtils
{
    public struct CommandArgument
    {
        public string argumentName;
        public string argKey;
        public string argValue;
        public CommandArgument(string name,string key,string value)
        {
            argumentName = name;
            argKey = key;
            argValue = value;
        }
    }
    public class ProcessUtils
    {
        public static bool ExecuteCommand(out string output, string command, string workingDirectory = null)
        {
            try
            {

                ProcessStartInfo procStartInfo = new ProcessStartInfo("cmd", "/c " + command);

                procStartInfo.RedirectStandardError = procStartInfo.RedirectStandardInput = procStartInfo.RedirectStandardOutput = true;
                procStartInfo.UseShellExecute = false;
                procStartInfo.CreateNoWindow = true;
                if (null != workingDirectory)
                {
                    procStartInfo.WorkingDirectory = workingDirectory;
                }

                Process proc = new Process();
                proc.StartInfo = procStartInfo;
                proc.Start();

                StringBuilder sb = new StringBuilder();
                proc.OutputDataReceived += delegate (object sender, DataReceivedEventArgs e)
                {
                    lock (sb)
                    {
                        sb.AppendLine(e.Data);
                    }
                };
                proc.ErrorDataReceived += delegate (object sender, DataReceivedEventArgs e)
                {
                    lock (sb)
                    {
                        sb.AppendLine(e.Data);
                    }
                };

                proc.BeginOutputReadLine();
                proc.BeginErrorReadLine();
                proc.WaitForExit();
                output = sb.ToString();
                output.Trim();
                if (proc.ExitCode != 0)
                {
                    output += ($"\nExit code {proc.ExitCode}");
                    return false;
                }
                return true;
            }
            catch (Exception objException)
            {
                output = $"Error in command: {command}, {objException.Message}";
                return false;
            }
        }
    }
}
