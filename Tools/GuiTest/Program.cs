// SPDX-License-Identifier: GPL-3.0-only
// TextBlinder GUI 자동 검증 드라이버. Run-GuiTest.ps1 이 빌드하고 실행한다.
//
// launch 단계: 격리 데스크톱을 만들고, 그 데스크톱에서 자신을 run 단계로 띄운 뒤 끝나기를 기다린다.
// run 단계: 같은 데스크톱에 휴대용 Notepad++ 를 띄워 창 메시지로 조작한다.
// 사용자 화면, 입력, 클립보드는 쓰지 않는다.
//
// usage: GuiTest.exe <full|stress|showcase> <workDir> <portableNppDir> <TextBlinder.dll>
using System;
using System.IO;
using System.Text;

static partial class Program
{
    const string DesktopName = "TextBlinderGuiTest";

    static string DllSource;

    static int Main(string[] args)
    {
        if (args.Length >= 1 && args[0] == "run") return Run(args);
        if (args.Length < 4)
        {
            Console.WriteLine("usage: GuiTest.exe <full|stress|showcase> <workDir> <portableNppDir> <TextBlinder.dll>");
            return 2;
        }
        return Launch(args[0], args[1], args[2], args[3]);
    }

    static int Launch(string mode, string workDir, string portableDir, string dll)
    {
        Directory.CreateDirectory(workDir);
        string logPath = Path.Combine(workDir, "log-" + mode + ".txt");
        if (File.Exists(logPath)) File.Delete(logPath);

        IntPtr desk = Win.CreateDesktop(DesktopName, IntPtr.Zero, IntPtr.Zero, 0, Win.GENERIC_ALL, IntPtr.Zero);
        if (desk == IntPtr.Zero) { Console.WriteLine("CreateDesktop failed: " + System.Runtime.InteropServices.Marshal.GetLastWin32Error()); return 2; }
        try
        {
            var si = new Win.STARTUPINFO();
            si.cb = System.Runtime.InteropServices.Marshal.SizeOf(si);
            si.lpDesktop = "WinSta0\\" + DesktopName;
            string self = System.Reflection.Assembly.GetExecutingAssembly().Location;
            var cmd = new StringBuilder("\"" + self + "\" run " + mode + " \"" + workDir + "\" \"" + portableDir + "\" \"" + dll + "\" \"" + logPath + "\"");
            Win.PROCESS_INFORMATION pi;
            if (!Win.CreateProcess(null, cmd, IntPtr.Zero, IntPtr.Zero, false, Win.CREATE_NO_WINDOW, IntPtr.Zero, workDir, ref si, out pi))
            {
                Console.WriteLine("CreateProcess failed: " + System.Runtime.InteropServices.Marshal.GetLastWin32Error());
                return 2;
            }
            if (Win.WaitForSingleObject(pi.hProcess, 8 * 60 * 1000) != 0)
            {
                Console.WriteLine("run step timed out, stopping it");
                System.Diagnostics.Process.GetProcessById(pi.dwProcessId).Kill();
            }
            uint code;
            Win.GetExitCodeProcess(pi.hProcess, out code);
            Win.CloseHandle(pi.hProcess);
            Win.CloseHandle(pi.hThread);

            // 테스트용 복사본 Notepad++ 만 정리한다. 설치본 (사용자 것) 은 경로가 달라서 건드리지 않는다.
            foreach (var p in System.Diagnostics.Process.GetProcessesByName("notepad++"))
            {
                try
                {
                    if (p.MainModule.FileName.StartsWith(portableDir, StringComparison.OrdinalIgnoreCase))
                    {
                        Console.WriteLine("cleanup: killing test Notepad++ pid " + p.Id);
                        p.Kill();
                        p.WaitForExit(5000);
                    }
                }
                catch { }
            }
            if (File.Exists(logPath)) Console.Write(File.ReadAllText(logPath, Encoding.UTF8));
            Console.WriteLine("exit=" + code);
            return (int)code;
        }
        finally { Win.CloseDesktop(desk); }
    }

    static StreamWriter logFile;
    static int pass, fail;

    static void Log(string s)
    {
        logFile.WriteLine(s);
        logFile.Flush();
    }

    static void Check(string name, bool ok, string detail)
    {
        if (ok) pass++; else fail++;
        Log((ok ? "PASS " : "FAIL ") + name + (string.IsNullOrEmpty(detail) ? "" : "  -- " + detail));
    }

    // args: run <mode> <workDir> <portableNppDir> <dll> <logPath>
    static int Run(string[] args)
    {
        string mode = args[1];
        work = args[2];
        portable = args[3];
        DllSource = args[4];
        logFile = new StreamWriter(args[5], false, new UTF8Encoding(false));
        try
        {
            if (mode == "full") Full(work);
            else if (mode == "stress") StressReopen(work);
            else if (mode == "showcase") Showcase(work);
            else Log("unknown mode: " + mode);
            Log(string.Format("RESULT pass={0} fail={1}", pass, fail));
            return fail == 0 ? 0 : 1;
        }
        catch (Exception e)
        {
            Log("ERROR " + e);
            return 3;
        }
        finally { logFile.Dispose(); }
    }
}
