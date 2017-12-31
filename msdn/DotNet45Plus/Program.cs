using Microsoft.Win32;
using System;


namespace DotNet45Plus
{
    class Program
    {
        static void Main(string[] args)
        {
           Get45PlusFromRegistry();
        }

        private static void Get45PlusFromRegistry()
        {
            const string subkey = @"SOFTWARE\Microsoft\NET Framework Setup\NDP\v4\Full\";

            using (RegistryKey ndpKey = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, RegistryView.Registry32).OpenSubKey(subkey))
            {
                if (ndpKey != null && ndpKey.GetValue("Release") != null)
                {
                    Console.Write(".NET Framework Version: " + CheckFor45PlusVersion((int)ndpKey.GetValue("Release")));
                }
                else
                {
                    Console.WriteLine(".NET Framework Version 4.5 or later is not detected.");
                }
            }
        }

        // Checking the version using >= will enable forward compatibility.
        private static string CheckFor45PlusVersion(int releaseKey)
        {
            var versions = "";
            if(releaseKey >= 461310)
                versions+= Environment.NewLine+"4.7.1 or later";
            if (releaseKey >= 461308)
                versions += Environment.NewLine + "4.7.1 Windows 10 Fall Creators Update";
            if (releaseKey >= 460805)
                versions += Environment.NewLine + "4.7";
            if (releaseKey >= 460798)
                versions += Environment.NewLine + "4.7 Windows 10 Creators Update";
            if (releaseKey >= 394806)
                versions += Environment.NewLine + "4.6.2";
            if (releaseKey >= 394802)
                versions += Environment.NewLine + "4.6.2 Windows 10 Anniversary Update";
            if (releaseKey >= 3394271)
                versions += Environment.NewLine + "4.6.1";
            if (releaseKey >= 394254)
                versions += Environment.NewLine + "4.6.1 Windows 10";
            if (releaseKey >= 393297)
                versions += Environment.NewLine + "4.6";
            if (releaseKey >= 393295)
                versions += Environment.NewLine + "4.6 Windows 10";
            if (releaseKey >= 379893)
                versions += Environment.NewLine + "4.5.2";
            if (releaseKey >= 378758)
                versions += Environment.NewLine + "4.5.1";
            if (releaseKey >= 378675)
                versions += Environment.NewLine + "4.5.1  Windows 8.1";
            if (releaseKey >= 378389)
                versions += Environment.NewLine + "4.5";
            // This code should never execute. A non-null release key should mean
            // that 4.5 or later is installed.
            if(versions.Length==0)
                versions += "No 4.5 or later version detected";
            return versions;
        }
    }
}
