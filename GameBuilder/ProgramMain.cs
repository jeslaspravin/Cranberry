using GameBuilder.BuildFileUtils;
using GameBuilder.Logger;
using System;
using System.IO;
using System.Threading;

namespace GameBuilder
{
    class ProgramMain
    {
        static void Main(string[] args)
        {
            LoggerUtils.Log("{0} using {1}",Directories.APP_NAME,Directories.BUILDER_EXE);



            LoggerUtils.waitUntilTasksFinished();
        }
    }
}
