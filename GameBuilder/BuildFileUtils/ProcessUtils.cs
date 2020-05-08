using System;
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
        public CommandArgument(string n,string k,string v)
        {
            argumentName = n;
            argKey = k;
            argValue = v;
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
