using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace GameBuilder.Logger
{
    class LogFile:LogBase
    {
        public LogFile(string filePath)
        {
            FileStream fileStream = File.Create(filePath);
            fileStream.Close();
            File.SetCreationTime(filePath, DateTime.Now);

            fileStream = File.OpenWrite(filePath);
            writer = new StreamWriter(fileStream);
            //writer.AutoFlush = true;
        }
    }
}
