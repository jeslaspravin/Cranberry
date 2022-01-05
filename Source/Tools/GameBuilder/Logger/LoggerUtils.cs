/*!
 * \file LoggerUtils.cs
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

﻿using GameBuilder.BuildFileUtils;
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
            string logDirectory = Path.Combine(Directories.TOOLS_BINARIES_ROOT, "Saved\\Logs\\");

            if (!Directory.Exists(logDirectory))
            {
                Directory.CreateDirectory(logDirectory);
            }

            IEnumerable<string> logFiles = Directory.EnumerateFiles(logDirectory, "*.log", SearchOption.TopDirectoryOnly);

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
            logFiles = logFiles.Where(fileName => fileName.Contains(Directories.APP_NAME+LOG_EXTENSION));

            string fileToRename = "";
            if (logFiles.Count()>0)
            {
                fileToRename = logFiles.First();
                string renameTo = Path.GetFileNameWithoutExtension(fileToRename) + "-" +
                    File.GetCreationTime(fileToRename).ToString("yyyy'-'MM'-'dd'-'HHmmssfff");
                {
                    int fileCount = 1;
                    string finalRenameTo = renameTo + LOG_EXTENSION;
                    while (File.Exists(Path.Combine(logDirectory, finalRenameTo)))
                    {
                        finalRenameTo = $"{renameTo}_{fileCount}{LOG_EXTENSION}";
                        fileCount++;
                    }
                    renameTo = finalRenameTo;
                }
                File.Move(fileToRename, Path.Combine(logDirectory, renameTo));
            }
            else
            {
                fileToRename = Path.Combine(logDirectory, Directories.APP_NAME+".log");
            }
            loggers = new List<LogBase>();
            loggers.Add(new LogFile(fileToRename));
            loggers.Add(new LogConsole());

            logsToWrite = new List<Queue<Action>>();
            logsToWrite.Add(new Queue<Action>());
            logsToWrite.Add(new Queue<Action>());

            logWriterTask = Task.Run(logTaskExecute);
        }

        private static int MAX_ALLOWED_FILE = 5;
        private static string LOG_EXTENSION = ".log";

        private static List<LogBase> loggers;
        // Will alway be > 1 count
        private static List<Queue<Action>> logsToWrite;
        private static int queueToWrite = 0;

        private static int exitTask = 0;
        private static Task logWriterTask;
        
        private static void logTaskExecute()
        {
            while(exitTask == 0)
            {
                int qIndex = queueToWrite;
                Interlocked.Exchange(ref queueToWrite, (qIndex + 1) % logsToWrite.Count);

                while (logsToWrite[qIndex].Count > 0)
                {
                    logsToWrite[qIndex].Peek().Invoke();
                    logsToWrite[qIndex].Dequeue();
                }
            }

            foreach(Queue<Action> q in logsToWrite)
            {
                while (q.Count > 0)
                {
                    q.Peek().Invoke();
                    q.Dequeue();
                }
            }
        }

        private static void WriteLog(string message)
        {
            foreach (LogBase logger in loggers)
            {
                logger.WriteLog(message);
                logger.Flush();
            }
        }
        
        public static void Log(string message, params object[] args)
        {
            StringBuilder sb = new StringBuilder(message);
            string finalMessage = message;
            if (args.Length != 0)
            {
                finalMessage = string.Format(sb.ToString(), args);
            }
            logsToWrite[queueToWrite].Enqueue(() => WriteLog(finalMessage));

        }

        public static void Warn(string message, params object[] args)
        {

            StringBuilder sb = new StringBuilder(message);
            sb.Insert(0, "WARN : ", 1);

            StackTrace st = new StackTrace(true);
            StackFrame frameOfInterest = st.GetFrame(1);

            sb.Append(string.Format("-- {0}({1}::{2} Line:{3})", frameOfInterest.GetFileName(), frameOfInterest.GetType().ToString()
                , frameOfInterest.GetMethod().ToString(), frameOfInterest.GetFileLineNumber()));

            if(args.Length != 0)
            {
                string finalMessage = string.Format(sb.ToString(), args);
                logsToWrite[queueToWrite].Enqueue(() => WriteLog(finalMessage));
            }
            else
            {
                logsToWrite[queueToWrite].Enqueue(() => WriteLog(sb.ToString()));
            }
        }

        public static void Error(string message, params object[] args)
        {
            StringBuilder sb = new StringBuilder(message);

            StackTrace st = new StackTrace(true);
            StackFrame frameOfInterest = st.GetFrame(1);
            sb.Append(string.Format("\n -- {0}({1}::{2} Line:{3})", frameOfInterest.GetFileName(), frameOfInterest.GetType().ToString()
                , frameOfInterest.GetMethod().ToString(), frameOfInterest.GetFileLineNumber()));

            if (args.Length != 0)
            {
                string finalMessage = string.Format(sb.ToString(), args);
                logsToWrite[queueToWrite].Enqueue(() => WriteLog(finalMessage));
            }
            else
            {
                logsToWrite[queueToWrite].Enqueue(() => WriteLog(sb.ToString()));
            }
        }

        public static void Error(Exception e, string message, params object[] args)
        {
            StringBuilder sb = new StringBuilder(message);
            sb.Insert(0, "ERROR : ", 1);

            StackTrace st = new StackTrace(true);
            StackFrame frameOfInterest = st.GetFrame(1);
            sb.Append(string.Format("\n -- {0}({1}::{2} Line:{3})", frameOfInterest.GetFileName(), frameOfInterest.GetType().ToString()
                , frameOfInterest.GetMethod().ToString(), frameOfInterest.GetFileLineNumber()));
            sb.Append("\nException : ");
            sb.Append(e.ToString());

            if (args.Length != 0)
            {
                string finalMessage = string.Format(sb.ToString(), args);
                logsToWrite[queueToWrite].Enqueue(() => WriteLog(finalMessage));
            }
            else
            {
                logsToWrite[queueToWrite].Enqueue(() => WriteLog(sb.ToString()));
            }
        }

        public static void waitUntilTasksFinished()
        {
            Interlocked.Exchange(ref exitTask, 1);
            logWriterTask.Wait();
        }
    }
}
