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
            FileStream fileStream = new FileStream(filePath, FileMode.CreateNew);
            writer = new StreamWriter(fileStream);
            //writer.AutoFlush = true;
        }
    }
}
