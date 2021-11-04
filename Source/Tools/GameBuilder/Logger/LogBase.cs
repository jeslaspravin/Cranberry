using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace GameBuilder.Logger
{
    class LogBase
    {
        protected StreamWriter writer = null;

        public void WriteLog(string message)
        {
            writer.WriteLine(message);
        }
        public void Flush()
        {
            if (writer != null && writer.BaseStream.CanWrite)
                writer.Flush();
        }

        ~LogBase()
        {
            if(writer != null && writer.BaseStream.CanWrite)
            {
                writer.Close();
            }
        }

    }
}
