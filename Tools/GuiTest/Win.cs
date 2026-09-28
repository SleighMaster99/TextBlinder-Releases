// SPDX-License-Identifier: GPL-3.0-only
// Win32 선언. GUI 자동 검증 드라이버 전용.
using System;
using System.Runtime.InteropServices;
using System.Text;

static class Win
{
    public const uint WM_USER = 0x0400;
    public const uint WM_SETTEXT = 0x000C;
    public const uint WM_GETTEXT = 0x000D;
    public const uint WM_GETTEXTLENGTH = 0x000E;
    public const uint WM_CLOSE = 0x0010;
    public const uint WM_COMMAND = 0x0111;
    public const uint WM_CHAR = 0x0102;
    public const uint BM_CLICK = 0x00F5;
    public const uint GENERIC_ALL = 0x10000000;
    public const uint CREATE_NO_WINDOW = 0x08000000;
    public const uint MF_BYPOSITION = 0x0400;
    public const uint PW_CLIENTONLY = 1;
    public const uint PW_RENDERFULLCONTENT = 2;

    public delegate bool EnumProc(IntPtr hwnd, IntPtr lParam);

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public struct STARTUPINFO
    {
        public int cb;
        public string lpReserved;
        public string lpDesktop;
        public string lpTitle;
        public int dwX, dwY, dwXSize, dwYSize, dwXCountChars, dwYCountChars, dwFillAttribute, dwFlags;
        public short wShowWindow, cbReserved2;
        public IntPtr lpReserved2, hStdInput, hStdOutput, hStdError;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct PROCESS_INFORMATION
    {
        public IntPtr hProcess, hThread;
        public int dwProcessId, dwThreadId;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }

    [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern IntPtr CreateDesktop(string name, IntPtr device, IntPtr devmode, uint flags, uint access, IntPtr sa);
    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool CloseDesktop(IntPtr desk);
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern bool CreateProcess(string app, StringBuilder cmd, IntPtr pa, IntPtr ta, bool inherit, uint flags,
        IntPtr env, string cwd, ref STARTUPINFO si, out PROCESS_INFORMATION pi);
    [DllImport("kernel32.dll")]
    public static extern uint WaitForSingleObject(IntPtr h, uint ms);
    [DllImport("kernel32.dll")]
    public static extern bool GetExitCodeProcess(IntPtr h, out uint code);
    [DllImport("kernel32.dll")]
    public static extern bool CloseHandle(IntPtr h);

    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumProc proc, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern bool EnumChildWindows(IntPtr parent, EnumProc proc, IntPtr lParam);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetClassName(IntPtr hwnd, StringBuilder sb, int max);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetWindowText(IntPtr hwnd, StringBuilder sb, int max);
    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hwnd, out int pid);
    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr hwnd);
    [DllImport("user32.dll")]
    public static extern bool IsWindow(IntPtr hwnd);
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hwnd, out RECT r);
    [DllImport("user32.dll")]
    public static extern bool GetClientRect(IntPtr hwnd, out RECT r);
    [DllImport("user32.dll")]
    public static extern int GetDlgCtrlID(IntPtr hwnd);
    [DllImport("user32.dll")]
    public static extern IntPtr GetParent(IntPtr hwnd);

    [DllImport("user32.dll")]
    public static extern IntPtr SendMessage(IntPtr hwnd, uint msg, IntPtr w, IntPtr l);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern IntPtr SendMessage(IntPtr hwnd, uint msg, IntPtr w, StringBuilder l);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern IntPtr SendMessage(IntPtr hwnd, uint msg, IntPtr w, string l);
    [DllImport("user32.dll")]
    public static extern bool PostMessage(IntPtr hwnd, uint msg, IntPtr w, IntPtr l);

    [DllImport("user32.dll")]
    public static extern int GetMenuItemCount(IntPtr menu);
    [DllImport("user32.dll")]
    public static extern IntPtr GetSubMenu(IntPtr menu, int pos);
    [DllImport("user32.dll")]
    public static extern uint GetMenuItemID(IntPtr menu, int pos);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetMenuString(IntPtr menu, uint item, StringBuilder sb, int max, uint flags);
    [DllImport("user32.dll")]
    public static extern IntPtr GetMenu(IntPtr hwnd);

    [DllImport("user32.dll")]
    public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdc, uint flags);

    [StructLayout(LayoutKind.Sequential)]
    public struct POINT { public int X, Y; }

    [DllImport("user32.dll")]
    public static extern bool ClientToScreen(IntPtr hwnd, ref POINT p);
    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr SendMessageTimeout(IntPtr hwnd, uint msg, IntPtr w, IntPtr l, uint flags, uint timeout, out IntPtr result);
    public const uint SMTO_ABORTIFHUNG = 0x0002;
    [DllImport("kernel32.dll")]
    public static extern bool WriteProcessMemory(IntPtr proc, IntPtr addr, byte[] buf, IntPtr size, out IntPtr written);

    [DllImport("kernel32.dll")]
    public static extern IntPtr VirtualAllocEx(IntPtr proc, IntPtr addr, IntPtr size, uint type, uint protect);
    [DllImport("kernel32.dll")]
    public static extern bool VirtualFreeEx(IntPtr proc, IntPtr addr, IntPtr size, uint type);
    [DllImport("kernel32.dll")]
    public static extern bool ReadProcessMemory(IntPtr proc, IntPtr addr, byte[] buf, IntPtr size, out IntPtr read);

    public static string ClassOf(IntPtr h)
    {
        var sb = new StringBuilder(256);
        GetClassName(h, sb, sb.Capacity);
        return sb.ToString();
    }

    public static string TitleOf(IntPtr h)
    {
        var sb = new StringBuilder(512);
        GetWindowText(h, sb, sb.Capacity);
        return sb.ToString();
    }

    public static int PidOf(IntPtr h)
    {
        int pid;
        GetWindowThreadProcessId(h, out pid);
        return pid;
    }
}
