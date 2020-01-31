using System;
using System.IO;

namespace GameBuilder.BuildFileUtils
{
    static class Directories
    {
        public static string ENGINE_ROOT = "";
        public static string ENGINE_BINARIES = "";
        public static string BUILDER_EXE = "";
        public static string APP_NAME = "";

        static Directories()
        {
            ENGINE_BINARIES = Directory.GetCurrentDirectory();
            ENGINE_ROOT = Directory.GetParent(ENGINE_BINARIES).FullName;
            APP_NAME = System.Reflection.Assembly.GetExecutingAssembly().GetName().Name;
            BUILDER_EXE = Path.Combine(ENGINE_BINARIES, APP_NAME+".exe");
        }

    }
}
