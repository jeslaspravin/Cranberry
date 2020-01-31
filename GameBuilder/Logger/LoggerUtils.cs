using GameBuilder.BuildFileUtils;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace GameBuilder.Logger
{
    static class LoggerUtils
    {
        static LoggerUtils()
        {
            string logDirectory = Path.Combine(Directories.ENGINE_ROOT, "Generated\\BuildLogs\\");

            if (!Directory.Exists(logDirectory))
            {
                Directory.CreateDirectory(logDirectory);
            }

            IEnumerable<string> logFiles=Directory.EnumerateFiles(logDirectory, "*.log", SearchOption.TopDirectoryOnly);

            // Clearing extra files
            if (logFiles.Count() > MAX_ALLOWED_FILE)
            {
                logFiles=logFiles.OrderByDescending(fileName => File.GetCreationTime(fileName).Ticks);
                IEnumerable<string> filesToDelete = logFiles.Where((fileName, index) => (index >= MAX_ALLOWED_FILE));
                foreach(string file in filesToDelete)
                {
                    File.Delete(file);
                }
            }

            // Finding last log file for renaming
            logFiles = logFiles.Where(fileName => fileName.Contains(Directories.APP_NAME+".Log"));

            string fileToRename = "";
            if (logFiles.Count()>0)
            {
                fileToRename = logFiles.First();
                string renameTo = Path.GetFileNameWithoutExtension(fileToRename) + "-" +
                    File.GetCreationTime(fileToRename).ToString("yyyy'-'MM'-'dd'-'HHmmssfff") + ".log";

                File.Move(fileToRename, Path.Combine(logDirectory,renameTo));
            }
            else
            {
                fileToRename = Path.Combine(logDirectory, Directories.APP_NAME+".Log");
            }
            loggers = new List<LogBase>();
            loggers.Add(new LogFile(fileToRename));
            loggers.Add(new LogConsole());

            tasksPending = new Queue<Task>();
        }

        private static int MAX_ALLOWED_FILE = 5;

        private static List<LogBase> loggers;
        private static Queue<Task> tasksPending;

        private static int indentCounter = 0;
        

        private static void WriteLog(string message)
        {
            while (tasksPending.Count>0 && tasksPending.Peek().IsCompleted)
            {
                tasksPending.Dequeue();
            }
            foreach (LogBase logger in loggers)
            {
                logger.WriteLog(message);
                logger.Flush();
            }
        }
        
        public static void Log(string message, params object[] args)
        {
            StringBuilder sb = new StringBuilder(message);

            if (indentCounter != 0)
                sb.Insert(0, "\t", indentCounter);


            string finalMessage = string.Format(sb.ToString(), args);

            tasksPending.Enqueue(Task.Run(() => WriteLog(finalMessage)));

        }


        // Logs and then increments the indent
        public static void LogBeforeIndent(bool forward, string message, params object[] args)
        {
            StringBuilder sb = new StringBuilder(message);

            if (indentCounter != 0)
                sb.Insert(0, "\t", indentCounter);

            string finalMessage = string.Format(sb.ToString(), args);
            tasksPending.Enqueue(Task.Run(() => WriteLog(finalMessage)));

            if (forward)
            {
                Interlocked.Increment(ref indentCounter);
            }
            else
            {
                Interlocked.Decrement(ref indentCounter);
                if (indentCounter < 0)
                {
                    Interlocked.Exchange(ref indentCounter, 0);
                }
            }
        }

        public static void Warn(string message, params object[] args)
        {

            StringBuilder sb = new StringBuilder(message);
            sb.Insert(0, "WARN : ", 1);

            if (indentCounter != 0)
                sb.Insert(0, "\t", indentCounter);

            StackTrace st = new StackTrace(true);
            StackFrame frameOfInterest = st.GetFrame(1);

            sb.Append(string.Format("-- {0}({1}::{2} Line:{3})", frameOfInterest.GetFileName(), frameOfInterest.GetType().ToString()
                , frameOfInterest.GetMethod().ToString(), frameOfInterest.GetFileLineNumber()));

            string finalMessage = string.Format(sb.ToString(), args);
            tasksPending.Enqueue(Task.Run(() => WriteLog(finalMessage)));
        }

        public static void Error(string message, params object[] args)
        {
            StringBuilder sb = new StringBuilder(message);
            sb.Insert(0, "ERROR : ", 1);

            if (indentCounter != 0)
                sb.Insert(0, "\t", indentCounter);

            StackTrace st = new StackTrace(true);
            StackFrame frameOfInterest = st.GetFrame(1);
            sb.Append(string.Format("\n -- {0}({1}::{2} Line:{3})", frameOfInterest.GetFileName(), frameOfInterest.GetType().ToString()
                , frameOfInterest.GetMethod().ToString(), frameOfInterest.GetFileLineNumber()));

            string finalMessage = string.Format(sb.ToString(), args);
            tasksPending.Enqueue(Task.Run(() => WriteLog(finalMessage)));

        }

        public static void Error(Exception e, string message, params object[] args)
        {
            StringBuilder sb = new StringBuilder(message);
            sb.Insert(0, "ERROR : ", 1);

            if (indentCounter != 0)
                sb.Insert(0, "\t", indentCounter);

            StackTrace st = new StackTrace(true);
            StackFrame frameOfInterest = st.GetFrame(1);
            sb.Append(string.Format("\n -- {0}({1}::{2} Line:{3})", frameOfInterest.GetFileName(), frameOfInterest.GetType().ToString()
                , frameOfInterest.GetMethod().ToString(), frameOfInterest.GetFileLineNumber()));
            sb.Append("\nException : ");
            sb.Append(e.ToString());


            string finalMessage = string.Format(sb.ToString(), args);
            tasksPending.Enqueue(Task.Run(() => WriteLog(finalMessage)));
        }

        public static void waitUntilTasksFinished()
        {
            while(tasksPending.Count > 0)
            {
                Task task = tasksPending.Dequeue();
                task.Wait();
            }
        }
    }
}
