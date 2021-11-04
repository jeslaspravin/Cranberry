using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace GameBuilder.Logger
{
    class LogConsole:LogBase
    {
        public LogConsole()
        {
            writer = new StreamWriter(Console.OpenStandardOutput());
           // writer.AutoFlush = true;
        }
    }
}
