using System;
using System.IO;

namespace GameBuilder.BuildFileUtils
{
    static class Directories
    {
        public static string ENGINE_ROOT = "";
        public static string BINARIES_ROOT = "";
        public static string BUILDER_ROOT = "";
        public static string BUILDER_EXE = "";
        public static string APP_NAME = "";

        static Directories()
        {
            BUILDER_ROOT = Directory.GetCurrentDirectory();
            BINARIES_ROOT = Directory.GetParent(BUILDER_ROOT).FullName;
            ENGINE_ROOT = Directory.GetParent(BINARIES_ROOT).FullName;
            APP_NAME = System.Reflection.Assembly.GetExecutingAssembly().GetName().Name;
            BUILDER_EXE = Path.Combine(BUILDER_ROOT, APP_NAME+".exe");
        }

    }
}
