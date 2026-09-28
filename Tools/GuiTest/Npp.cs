// SPDX-License-Identifier: GPL-3.0-only
// 격리 데스크톱에서 띄운 Notepad++ 를 창 메시지로 조작하는 도우미.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Text;
using System.Threading;

class Npp : IDisposable
{
    public const uint NPPMSG = Win.WM_USER + 1000;
    public const uint NPPM_GETMENUHANDLE = NPPMSG + 25;
    public const uint NPPM_ACTIVATEDOC = NPPMSG + 28;
    public const uint NPPM_GETCURRENTBUFFERID = NPPMSG + 60;

    public Process Proc;
    public IntPtr Main;
    readonly Action<string> log;

    public Npp(Action<string> log) { this.log = log; }

    public void Start(string exe, string args)
    {
        var psi = new ProcessStartInfo(exe, args);
        psi.UseShellExecute = false;
        psi.WorkingDirectory = Path.GetDirectoryName(exe);
        Proc = Process.Start(psi);
        Main = WaitFor(delegate { return FindTop("Notepad++", null); }, 20000);
        if (Main == IntPtr.Zero) throw new Exception("Notepad++ main window not found");
        Thread.Sleep(1500);
    }

    public void Exit()
    {
        if (Proc == null) return;
        if (!Proc.HasExited)
        {
            Win.PostMessage(Main, Win.WM_CLOSE, IntPtr.Zero, IntPtr.Zero);
            if (!Proc.WaitForExit(10000)) { log("  (Notepad++ did not exit, killing)"); Proc.Kill(); Proc.WaitForExit(5000); }
        }
        Proc = null;
        Main = IntPtr.Zero;
    }

    public void Dispose()
    {
        try { if (Proc != null && !Proc.HasExited) { Proc.Kill(); Proc.WaitForExit(5000); } } catch { }
    }

    public static IntPtr WaitFor(Func<IntPtr> find, int ms)
    {
        var sw = Stopwatch.StartNew();
        while (sw.ElapsedMilliseconds < ms)
        {
            IntPtr h = find();
            if (h != IntPtr.Zero) return h;
            Thread.Sleep(100);
        }
        return IntPtr.Zero;
    }

    // 이 Notepad++ 프로세스의 보이는 최상위 창. cls, title 이 null 이면 따지지 않는다.
    public IntPtr FindTop(string cls, string titlePart)
    {
        IntPtr found = IntPtr.Zero;
        int pid = Proc.Id;
        Win.EnumWindows(delegate (IntPtr h, IntPtr l)
        {
            if (Win.PidOf(h) != pid || !Win.IsWindowVisible(h)) return true;
            if (cls != null && Win.ClassOf(h) != cls) return true;
            if (titlePart != null && !Win.TitleOf(h).Contains(titlePart)) return true;
            found = h;
            return false;
        }, IntPtr.Zero);
        return found;
    }

    public static List<IntPtr> Children(IntPtr parent, string cls)
    {
        var list = new List<IntPtr>();
        Win.EnumChildWindows(parent, delegate (IntPtr h, IntPtr l)
        {
            if (cls == null || Win.ClassOf(h) == cls) list.Add(h);
            return true;
        }, IntPtr.Zero);
        return list;
    }

    public static IntPtr ChildById(IntPtr parent, int id, string cls)
    {
        foreach (var h in Children(parent, cls))
            if (Win.GetDlgCtrlID(h) == id) return h;
        return IntPtr.Zero;
    }

    // 주 편집 보기: 보이는 Scintilla 가운데 가장 큰 것.
    public IntPtr MainSci()
    {
        IntPtr best = IntPtr.Zero;
        long bestArea = 0;
        foreach (var h in Children(Main, "Scintilla"))
        {
            if (!Win.IsWindowVisible(h)) continue;
            Win.RECT r;
            Win.GetWindowRect(h, out r);
            long area = (long)(r.Right - r.Left) * (r.Bottom - r.Top);
            if (area > bestArea) { bestArea = area; best = h; }
        }
        return best;
    }

    // 멈춘 곳을 찾으려고 시간 제한을 둔다. 제한을 넘으면 예외를 던진다.
    public static long Sci(IntPtr sci, uint msg, long w = 0, long l = 0)
    {
        return Send(sci, msg, w, l, 10000);
    }

    public static long Send(IntPtr h, uint msg, long w, long l, uint ms)
    {
        IntPtr result;
        if (Win.SendMessageTimeout(h, msg, new IntPtr(w), new IntPtr(l), Win.SMTO_ABORTIFHUNG, ms, out result) == IntPtr.Zero)
            throw new TimeoutException("SendMessage timed out: msg=" + msg + " w=" + w + " l=" + l);
        return (long)result;
    }

    public void Cmd(int id) { Send(Main, Win.WM_COMMAND, id, 0, 20000); }

    public long BufferId() { return Send(Main, NPPM_GETCURRENTBUFFERID, 0, 0, 5000); }

    // SCI_SETTEXT 는 문서 인코딩 (UTF-8) 의 바이트 포인터를 받는다. 그 프로세스 메모리에 써서 넘긴다.
    // WM_SETTEXT 는 쓰지 않는다: Scintilla 가 UTF-16 을 바이트로 읽어 첫 글자만 들어간다 (1 차 실행에서 확인).
    public void SetDocText(IntPtr sci, string text)
    {
        byte[] bytes = Encoding.UTF8.GetBytes(text + "\0");
        IntPtr mem = Win.VirtualAllocEx(Proc.Handle, IntPtr.Zero, new IntPtr(bytes.Length), 0x3000, 0x04);
        try
        {
            IntPtr written;
            Win.WriteProcessMemory(Proc.Handle, mem, bytes, new IntPtr(bytes.Length), out written);
            Sci(sci, 2181, 0, (long)mem);
        }
        finally { Win.VirtualFreeEx(Proc.Handle, mem, IntPtr.Zero, 0x8000); }
    }

    // 주 창 캡처 안에서 sci 클라이언트 영역 (0,0) 의 위치.
    public Point SciOrigin(IntPtr sci)
    {
        var pt = new Win.POINT();
        Win.ClientToScreen(sci, ref pt);
        Win.RECT r;
        Win.GetWindowRect(Main, out r);
        return new Point(pt.X - r.Left, pt.Y - r.Top);
    }

    public List<string> TopWindows()
    {
        var list = new List<string>();
        int pid = Proc.Id;
        Win.EnumWindows(delegate (IntPtr h, IntPtr l)
        {
            if (Win.PidOf(h) == pid && Win.IsWindowVisible(h)) list.Add(Win.ClassOf(h) + " '" + Win.TitleOf(h) + "'");
            return true;
        }, IntPtr.Zero);
        return list;
    }
    public void PostCmd(int id) { Win.PostMessage(Main, Win.WM_COMMAND, new IntPtr(id), IntPtr.Zero); }

    public static string Text(IntPtr h)
    {
        int len = (int)Send(h, Win.WM_GETTEXTLENGTH, 0, 0, 10000);
        var sb = new StringBuilder(len + 2);
        Win.SendMessage(h, Win.WM_GETTEXT, new IntPtr(len + 1), sb);
        return sb.ToString();
    }

    public static void SetText(IntPtr h, string text) { Win.SendMessage(h, Win.WM_SETTEXT, IntPtr.Zero, text); }

    public static string MenuText(IntPtr menu, int pos)
    {
        var sb = new StringBuilder(512);
        Win.GetMenuString(menu, (uint)pos, sb, sb.Capacity, Win.MF_BYPOSITION);
        return sb.ToString();
    }

    public IntPtr PluginsMenu() { return Win.SendMessage(Main, NPPM_GETMENUHANDLE, IntPtr.Zero, IntPtr.Zero); }

    // 플러그인 메뉴 아래 name 하위 메뉴의 (명령 ID, 글) 목록. 없으면 빈 목록.
    public List<KeyValuePair<int, string>> PluginItems(string name)
    {
        var items = new List<KeyValuePair<int, string>>();
        IntPtr pm = PluginsMenu();
        int n = Win.GetMenuItemCount(pm);
        for (int i = 0; i < n; i++)
        {
            if (MenuText(pm, i).Replace("&", "") != name) continue;
            IntPtr sub = Win.GetSubMenu(pm, i);
            int m = Win.GetMenuItemCount(sub);
            for (int j = 0; j < m; j++)
                items.Add(new KeyValuePair<int, string>((int)Win.GetMenuItemID(sub, j), MenuText(sub, j)));
        }
        return items;
    }

    // 모든 메뉴 항목 글 (주 메뉴 전체를 따라 내려간다. 플러그인 메뉴도 들어 있다).
    public List<string> AllMenuTexts()
    {
        var list = new List<string>();
        Walk(Win.GetMenu(Main), "", list, 0);
        return list;
    }

    static void Walk(IntPtr menu, string path, List<string> list, int depth)
    {
        if (menu == IntPtr.Zero || depth > 6) return;
        int n = Win.GetMenuItemCount(menu);
        for (int i = 0; i < n; i++)
        {
            string t = MenuText(menu, i);
            IntPtr sub = Win.GetSubMenu(menu, i);
            if (sub != IntPtr.Zero) Walk(sub, path + t.Replace("&", "") + " > ", list, depth + 1);
            else if (t.Length > 0) list.Add(path + t);
        }
    }

    // 상태 표시줄 칸의 글. 버퍼가 다른 프로세스에 있어서 그 프로세스 메모리를 빌려 읽는다.
    public string StatusText(int part)
    {
        IntPtr sb = IntPtr.Zero;
        foreach (var h in Children(Main, "msctls_statusbar32")) { sb = h; break; }
        if (sb == IntPtr.Zero) return "";
        int len = (int)Win.SendMessage(sb, Win.WM_USER + 12, new IntPtr(part), IntPtr.Zero) & 0xFFFF;
        int bytes = (len + 1) * 2;
        IntPtr mem = Win.VirtualAllocEx(Proc.Handle, IntPtr.Zero, new IntPtr(bytes), 0x3000, 0x04);
        try
        {
            Win.SendMessage(sb, Win.WM_USER + 13, new IntPtr(part), mem);
            var buf = new byte[bytes];
            IntPtr read;
            Win.ReadProcessMemory(Proc.Handle, mem, buf, new IntPtr(bytes), out read);
            return Encoding.Unicode.GetString(buf, 0, len * 2);
        }
        finally { Win.VirtualFreeEx(Proc.Handle, mem, IntPtr.Zero, 0x8000); }
    }

    // 창을 PNG 로 저장하고 그림을 돌려준다. clientOnly 면 클라이언트 영역만 (Scintilla 좌표와 같다).
    public static Bitmap Capture(IntPtr h, string path, bool clientOnly)
    {
        Win.RECT r;
        if (clientOnly) Win.GetClientRect(h, out r); else Win.GetWindowRect(h, out r);
        int w = Math.Max(1, r.Right - r.Left), hgt = Math.Max(1, r.Bottom - r.Top);
        var bmp = new Bitmap(w, hgt, PixelFormat.Format32bppArgb);
        using (var g = Graphics.FromImage(bmp))
        {
            IntPtr hdc = g.GetHdc();
            Win.PrintWindow(h, hdc, clientOnly ? Win.PW_CLIENTONLY : 0);
            g.ReleaseHdc(hdc);
        }
        bmp.Save(path, ImageFormat.Png);
        return bmp;
    }
}
