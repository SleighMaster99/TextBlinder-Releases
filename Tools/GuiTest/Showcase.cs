// SPDX-License-Identifier: GPL-3.0-only
// 사용자에게 보여 줄 장면을 캡처한다. 기본 가림 색 (#808080) 으로 찍는다.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;

static partial class Program
{
    const string ShowText =
        "; production settings\r\n" +
        "[database]\r\n" +
        "host = 10.0.12.34\r\n" +
        "user = admin\r\n" +
        "password = Tr0ub4dor&3\r\n" +
        "Password = UpperCase1\r\n" +
        "\r\n" +
        "[service]\r\n" +
        "api_key = sk-live-9f2a71c0e4\r\n" +
        "endpoint = https://api.example.com\r\n" +
        "timeout = 30\r\n";

    static void Full2(string name) { Npp.Capture(npp.Main, Path.Combine(work, name), false).Dispose(); Log("shot " + name); }
    static void Ed(string name) { Npp.Capture(sci, Path.Combine(work, name), true).Dispose(); Log("shot " + name); }

    static void GdiAll()
    {
        foreach (var w in Npp.Children(npp.Main, "Scintilla")) Npp.Sci(w, 2630, 0);
        Thread.Sleep(300);
    }

    // 메뉴를 키보드 메시지로 연다. 격리 데스크톱 안에서만 가며, 사용자 입력은 쓰지 않는다.
    static void MenuShot()
    {
        IntPtr pm = npp.PluginsMenu();
        int n = Win.GetMenuItemCount(pm), steps = -1, k = 0;
        for (int i = 0; i < n; i++)
        {
            string t = Npp.MenuText(pm, i);
            if (t.Length == 0) continue;  // 구분선
            if (t.Replace("&", "") == "TextBlinder") { steps = k; break; }
            k++;
        }
        Log("TextBlinder is item " + steps + " (non-separator) in plugins menu");
        if (steps < 0) return;
        const uint WM_SYSCOMMAND = 0x0112, WM_KEYDOWN = 0x0100;
        const int SC_KEYMENU = 0xF100, VK_DOWN = 0x28, VK_RIGHT = 0x27, VK_ESCAPE = 0x1B;
        Win.PostMessage(npp.Main, WM_SYSCOMMAND, new IntPtr(SC_KEYMENU), IntPtr.Zero);
        Thread.Sleep(400);
        Action<int> key = delegate (int vk) { Win.PostMessage(npp.Main, WM_KEYDOWN, new IntPtr(vk), IntPtr.Zero); Thread.Sleep(150); };
        key(VK_DOWN);                               // 파일 메뉴 열기
        for (int i = 0; i < 10; i++) key(VK_RIGHT); // 플러그인 메뉴까지 이동
        // 방향키로 연 메뉴는 처음에 고른 항목이 없다. 첫 아래 키가 0 번 항목을 고른다.
        for (int i = 0; i <= steps; i++) key(VK_DOWN);
        key(VK_RIGHT);                              // TextBlinder 하위 메뉴 열기
        Thread.Sleep(500);
        var popups = new List<IntPtr>();
        Win.EnumWindows(delegate (IntPtr h, IntPtr l)
        {
            if (Win.PidOf(h) == npp.Proc.Id && Win.IsWindowVisible(h) && Win.ClassOf(h) == "#32768") popups.Add(h);
            return true;
        }, IntPtr.Zero);
        Log("menu popups: " + popups.Count);
        for (int i = 0; i < popups.Count; i++)
            Npp.Capture(popups[i], Path.Combine(work, "s-menu-" + i + ".png"), false).Dispose();
        Full2("s-menu-window.png");
        for (int i = 0; i < 4; i++) key(VK_ESCAPE);
        Thread.Sleep(300);
    }

    [System.Runtime.InteropServices.DllImport("user32.dll")]
    static extern int TrackPopupMenu(IntPtr menu, uint flags, int x, int y, int reserved, IntPtr hwnd, IntPtr rect);

    // Notepad++ 가 가진 TextBlinder 하위 메뉴 핸들을 이 프로세스에서 띄워 캡처한다 (같은 격리 데스크톱).
    static void SubMenuShot()
    {
        IntPtr pm = npp.PluginsMenu(), sub = IntPtr.Zero;
        for (int i = 0; i < Win.GetMenuItemCount(pm); i++)
            if (Npp.MenuText(pm, i).Replace("&", "") == "TextBlinder") sub = Win.GetSubMenu(pm, i);
        if (sub == IntPtr.Zero) return;
        var form = new System.Windows.Forms.Form();
        form.ShowInTaskbar = false;
        form.StartPosition = System.Windows.Forms.FormStartPosition.Manual;
        form.Location = new System.Drawing.Point(50, 50);
        form.Size = new System.Drawing.Size(40, 40);
        form.Show();
        IntPtr owner = form.Handle;
        int myPid = System.Diagnostics.Process.GetCurrentProcess().Id;
        var t = new Thread(delegate ()
        {
            Thread.Sleep(800);
            int n = 0;
            Win.EnumWindows(delegate (IntPtr h, IntPtr l)
            {
                if (Win.PidOf(h) == myPid && Win.IsWindowVisible(h) && Win.ClassOf(h) == "#32768")
                    Npp.Capture(h, Path.Combine(work, "s-submenu-" + (n++) + ".png"), false).Dispose();
                return true;
            }, IntPtr.Zero);
            Log("submenu popups captured: " + n);
            Win.PostMessage(owner, 0x0100, new IntPtr(0x1B), IntPtr.Zero);  // Esc
            Win.PostMessage(owner, 0x001F, IntPtr.Zero, IntPtr.Zero);       // WM_CANCELMODE
        });
        t.Start();
        TrackPopupMenu(sub, 0x0100 | 0x0080, 60, 60, 0, owner, IntPtr.Zero);  // TPM_RETURNCMD | TPM_NONOTIFY
        t.Join();
        form.Close();
    }

    static void Showcase(string workDir)
    {
        work = workDir;
        string cfg = Path.Combine(portable, "plugins", "Config");
        iniPath = Path.Combine(cfg, "TextBlinder.ini");
        masksPath = Path.Combine(cfg, "TextBlinder.masks");
        foreach (var f in new[] { iniPath, masksPath }) if (File.Exists(f)) File.Delete(f);
        string file = Path.Combine(work, "server.ini");
        File.WriteAllText(file, ShowText, new UTF8Encoding(false));

        StartNpp("\"" + file + "\"");
        try
        {
            WaitPluginReady();
            for (int i = 0; i < 36 && selInd < 0; i++) if (S(SCI_INDICGETSTYLE, i) == INDIC_FULLBOX) { selInd = i; ruleInd = i + 1; }
            GdiAll();
            Caret(0);
            Full2("s01-start.png");

            try { MenuShot(); } catch (Exception e) { Log("menu shot failed: " + e.Message); }
            try { SubMenuShot(); } catch (Exception e) { Log("submenu shot failed: " + e.Message); }

            long h = At("10.0.12.34");
            Sel(h, h + 10); Thread.Sleep(200);
            Ed("s02-selected.png");
            Toggle(); Caret(0); Thread.Sleep(200);
            Ed("s03-masked.png");

            long u = At("admin"), a = At("sk-live-9f2a71c0e4");
            S(SCI_SETSELECTION, u + 5, u);
            S(SCI_ADDSELECTION, a + 18, a);
            Thread.Sleep(200);
            Ed("s04-multi-selected.png");
            Toggle(); Caret(0); Thread.Sleep(200);
            Ed("s05-multi-masked.png");

            npp.Cmd(cmdVisible); Thread.Sleep(200);
            Ed("s06-hidden.png");
            npp.Cmd(cmdVisible); Thread.Sleep(200);

            npp.Cmd(cmdOpenSettings);
            sci = npp.MainSci();
            GdiAll();
            Caret(0);
            Full2("s07-settings-file.png");
            string ini = Npp.Text(sci);
            ini = ini.Replace(";Rule1=password", "Rule1=password")
                + "Rule3=(?i)api_key\\s*=\\s*\\K.+\r\n";
            npp.SetDocText(sci, ini);
            npp.Cmd(IDM_FILE_SAVE);
            Thread.Sleep(500);
            Caret(0);
            Full2("s08-settings-with-rules.png");

            ActivateByTitle("server.ini");
            npp.Cmd(cmdUnmaskAll);
            Caret(0); Thread.Sleep(400);
            Full2("s09-rules-on-open.png");

            // 선택 가림을 다시 하고 저장된 파일을 밖에서 바꾼 뒤 다시 연다
            Sel(h, h + 10); Toggle(); Caret(0);
            npp.Cmd(IDM_FILE_CLOSE);
            File.WriteAllText(file, File.ReadAllText(file).Replace("10.0.12.34", "10.0.12.99"), new UTF8Encoding(false));
            npp.Cmd(IDM_FILE_RESTORELASTCLOSEDFILE);
            sci = npp.MainSci();
            GdiAll();
            Caret(0);
            Thread.Sleep(700);
            Full2("s10-skipped-status.png");

            npp.Cmd(IDM_VIEW_DOC_MAP);
            Thread.Sleep(1800);
            GdiAll();
            Thread.Sleep(300);
            Full2("s11-document-map.png");
            npp.Cmd(IDM_VIEW_DOC_MAP);

            Log("done");
        }
        finally { npp.Dispose(); }
    }
}
