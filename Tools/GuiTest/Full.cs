// SPDX-License-Identifier: GPL-3.0-only
// 수동 검증 8 개를 자동으로 돌린다. 결과는 PASS / FAIL / OBSERVE 줄로 남긴다.
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;

static partial class Program
{
    // Notepad++ 메뉴 명령 (menuCmdID.h)
    const int IDM_FILE_CLOSE = 41003, IDM_FILE_SAVE = 41006, IDM_FILE_RESTORELASTCLOSEDFILE = 41021;
    const int IDM_SEARCH_FIND = 43001, IDM_VIEW_DOC_MAP = 44080, IDM_SETTING_IMPORTPLUGIN = 48005;

    // Scintilla 메시지 (Scintilla.h)
    const uint SCI_GETLENGTH = 2006, SCI_GOTOPOS = 2025, SCI_INDICGETSTYLE = 2081, SCI_INDICGETFORE = 2083;
    const uint SCI_GETMODIFY = 2159, SCI_SETSEL = 2160, SCI_POINTXFROMPOSITION = 2164, SCI_POINTYFROMPOSITION = 2165;
    const uint SCI_UNDO = 2176, SCI_TEXTHEIGHT = 2279, SCI_DOCUMENTEND = 2318, SCI_INDICATORVALUEAT = 2507;
    const uint SCI_INDICATOREND = 2509, SCI_INDICGETUNDER = 2511, SCI_INDICGETALPHA = 2524, SCI_SETEMPTYSELECTION = 2556;
    const uint SCI_INDICGETOUTLINEALPHA = 2559, SCI_GETSELECTIONS = 2570, SCI_SETSELECTION = 2572, SCI_ADDSELECTION = 2573;
    const uint SCI_SETRECTANGULARSELECTIONCARET = 2588, SCI_SETRECTANGULARSELECTIONANCHOR = 2590, SCI_DELETERANGE = 2645;
    const uint SCI_GETSELECTIONNSTART = 2585, SCI_GETSELECTIONNEND = 2587;
    const int INDIC_FULLBOX = 16, SMART_HIGHLIGHT = 29;

    const string TestText =
        "[database]\r\n" +
        "host = 10.0.12.34\r\n" +
        "user = admin\r\n" +
        "password = Tr0ub4dor&3\r\n" +
        "Password = UpperCase1\r\n" +
        "api_key = sk-live-9f2a71c0e4\r\n" +
        "note = admin\r\n" +
        "timeout = 30\r\n";

    static Npp npp;
    static IntPtr sci;
    static int selInd = -1, ruleInd = -1;
    static int cmdToggle, cmdVisible, cmdUnmaskAll, cmdOpenSettings;
    static string work, portable, t1Path, iniPath, masksPath;

    static void Observe(string name, string detail) { Log("OBSERVE " + name + "  -- " + detail); }

    static long S(uint msg, long w = 0, long l = 0) { return Npp.Sci(sci, msg, w, l); }

    static List<long[]> Ranges(int ind)
    {
        var list = new List<long[]>();
        long len = S(SCI_GETLENGTH), pos = 0;
        while (pos < len)
        {
            long end = S(SCI_INDICATOREND, ind, pos);
            if (end <= pos) break;
            if (S(SCI_INDICATORVALUEAT, ind, pos) != 0) list.Add(new[] { pos, end });
            pos = end;
        }
        return list;
    }

    static string Fmt(List<long[]> r) { return r.Count == 0 ? "(none)" : string.Join(" ", r.Select(x => "[" + x[0] + "," + x[1] + ")")); }

    static void ExpectRanges(string name, int ind, params long[] pairs)
    {
        var exp = new List<long[]>();
        for (int i = 0; i + 1 < pairs.Length; i += 2) exp.Add(new[] { pairs[i], pairs[i + 1] });
        var act = Ranges(ind);
        Check(name, Fmt(act) == Fmt(exp), "expected " + Fmt(exp) + ", actual " + Fmt(act));
    }

    static long At(string needle, int nth = 1)
    {
        string text = Npp.Text(sci);
        int idx = -1;
        for (int i = 0; i < nth; i++) { idx = text.IndexOf(needle, idx + 1, StringComparison.Ordinal); if (idx < 0) break; }
        if (idx < 0) throw new Exception("text not found: " + needle);
        return idx;  // 시험 파일은 ASCII 라서 글자 위치 = 바이트 위치
    }

    static void Sel(long a, long b) { S(SCI_SETSEL, a, b); }
    static void Toggle() { npp.Cmd(cmdToggle); }
    static void Caret(long p) { S(SCI_SETEMPTYSELECTION, p); }

    static void Type(string s)
    {
        foreach (char c in s) Win.PostMessage(sci, Win.WM_CHAR, new IntPtr(c), IntPtr.Zero);
        Thread.Sleep(300);
    }

    static void UndoAll()
    {
        for (int i = 0; i < 200 && S(SCI_GETMODIFY) != 0; i++) S(SCI_UNDO);
    }

    // 글자 한 칸 안에서 가장 많은 색과 그 비율. 한 점만 보면 글자 획에 걸릴 수 있어서 칸 전체를 센다.
    static string Cell(Bitmap bmp, long pos)
    {
        Point o = new Point(0, 0);  // Shot 은 편집 영역 클라이언트만 캡처한다
        int x0 = o.X + (int)S(SCI_POINTXFROMPOSITION, 0, pos);
        int x1 = o.X + (int)S(SCI_POINTXFROMPOSITION, 0, pos + 1);
        int y0 = o.Y + (int)S(SCI_POINTYFROMPOSITION, 0, pos);
        int y1 = y0 + (int)S(SCI_TEXTHEIGHT, 0);
        var count = new Dictionary<int, int>();
        int total = 0;
        for (int y = Math.Max(0, y0); y < Math.Min(bmp.Height, y1); y++)
            for (int x = Math.Max(0, x0); x < Math.Min(bmp.Width, x1); x++)
            {
                int c = bmp.GetPixel(x, y).ToArgb() & 0xFFFFFF;
                int n;
                count.TryGetValue(c, out n);
                count[c] = n + 1;
                total++;
            }
        if (total == 0) return "(outside capture)";
        var top = count.OrderByDescending(kv => kv.Value).First();
        return string.Format("#{0:X6} {1}% of cell ({2}x{3} at {4},{5})", top.Key, top.Value * 100 / total, x1 - x0, y1 - y0, x0, y0);
    }

    // 주 창을 캡처한다. Scintilla 창만 따로 캡처하면 검게 나온다 (1 차 실행에서 확인).
    // 편집 영역 클라이언트를 캡처한다. 격리 데스크톱에서는 DirectWrite 로 그린 화면이 캡처되지 않아서
    // 테스트용 복사본만 GDI 로 그리게 바꾼다 (WaitPluginReady). 사용자 설치본은 바꾸지 않는다.
    static Bitmap Shot(string name) { return Npp.Capture(sci, Path.Combine(work, name), true); }

    static void StartNpp(string args)
    {
        npp = new Npp(Log);
        npp.Start(Path.Combine(portable, "notepad++.exe"), "-multiInst -nosession " + args);
    }

    static void WaitPluginReady()
    {
        IntPtr ok = Npp.WaitFor(delegate
        {
            var items = npp.PluginItems("TextBlinder");
            return items.Count == 5 && items[0].Value.StartsWith("선택 영역") ? new IntPtr(1) : IntPtr.Zero;
        }, 15000);
        if (ok == IntPtr.Zero) Log("  (TextBlinder menu did not turn Korean in time)");
        var list = npp.PluginItems("TextBlinder");
        if (list.Count == 5)
        {
            cmdToggle = list[0].Key; cmdVisible = list[1].Key; cmdUnmaskAll = list[2].Key; cmdOpenSettings = list[3].Key;
        }
        sci = npp.MainSci();
        S(2630, 0);  // SCI_SETTECHNOLOGY = GDI. 테스트용 복사본에서만 캡처를 위해 바꾼다
    }

    static void ActivateByTitle(string namePart)
    {
        int n = (int)Win.SendMessage(npp.Main, Npp.NPPMSG + 7, IntPtr.Zero, new IntPtr(1));
        for (int i = 0; i < n; i++)
        {
            Win.SendMessage(npp.Main, Npp.NPPM_ACTIVATEDOC, IntPtr.Zero, new IntPtr(i));
            if (Win.TitleOf(npp.Main).Contains(namePart)) { sci = npp.MainSci(); return; }
        }
        throw new Exception("tab not found: " + namePart);
    }

    static void Full(string workDir)
    {
        work = workDir;
        t1Path = Path.Combine(work, "t1.ini");
        string cfg = Path.Combine(portable, "plugins", "Config");
        iniPath = Path.Combine(cfg, "TextBlinder.ini");
        masksPath = Path.Combine(cfg, "TextBlinder.masks");
        string pluginDir = Path.Combine(portable, "plugins", "TextBlinder");
        if (Directory.Exists(pluginDir)) Directory.Delete(pluginDir, true);
        foreach (var f in new[] { iniPath, masksPath }) if (File.Exists(f)) File.Delete(f);
        File.WriteAllText(t1Path, TestText, new UTF8Encoding(false));
        Log("portable: " + portable);
        Log("dll: " + DllSource + " (" + File.GetLastWriteTime(DllSource) + ")");

        try
        {
            Item1Install();
            StartNpp("\"" + t1Path + "\"");
            WaitPluginReady();
            Item1Menu();
            Item2Toggle();
            Item3Original();
            Item4Persist();
            Item5VisibleAndColor();
            Item6Rules();
            Item7Screens();
            Item8EditAndKeys();
            UndoAll();
            npp.Exit();
        }
        catch (Exception e)
        {
            Log("ERROR " + e.GetType().Name + ": " + e.Message);
            try
            {
                Log("  visible windows: " + string.Join(" | ", npp.TopWindows()));
                Npp.Capture(npp.Main, Path.Combine(work, "error-main.png"), false).Dispose();
            }
            catch { }
            fail++;
        }
        finally
        {
            if (npp != null) npp.Dispose();
        }
    }

    // ---- 1. 메뉴로 가져오기 설치, 메뉴와 한국어 ----
    static void Item1Install()
    {
        Log("== 1. install via Settings > Import > Import plugin(s)");
        StartNpp("");
        Check("1a TextBlinder menu absent before import", npp.PluginItems("TextBlinder").Count == 0, "");
        npp.PostCmd(IDM_SETTING_IMPORTPLUGIN);
        IntPtr dlg = Npp.WaitFor(delegate { return npp.FindTop("#32770", null); }, 10000);
        Check("1b import file dialog opened", dlg != IntPtr.Zero, dlg == IntPtr.Zero ? "" : "title=" + Win.TitleOf(dlg));
        if (dlg == IntPtr.Zero) return;
        Thread.Sleep(800);
        Npp.Capture(dlg, Path.Combine(work, "1-import-dialog.png"), false).Dispose();

        IntPtr edit = IntPtr.Zero;
        foreach (var e in Npp.Children(dlg, "Edit"))
        {
            for (IntPtr p = e; p != IntPtr.Zero && p != dlg; p = Win.GetParent(p))
                if (Win.GetDlgCtrlID(p) == 1148) { edit = e; break; }
            if (edit != IntPtr.Zero) break;
        }
        Check("1c file name box found", edit != IntPtr.Zero, "");
        Npp.SetText(edit, DllSource);
        IntPtr open = Npp.ChildById(dlg, 1, "Button");
        Win.PostMessage(open, Win.BM_CLICK, IntPtr.Zero, IntPtr.Zero);
        IntPtr box = Npp.WaitFor(delegate
        {
            IntPtr h = npp.FindTop("#32770", null);
            return h != IntPtr.Zero && h != dlg ? h : IntPtr.Zero;
        }, 10000);
        string boxText = box == IntPtr.Zero ? "" : string.Join(" / ", Npp.Children(box, "Static").Select(Npp.Text).Where(s => s.Length > 0));
        Check("1d restart message shown", box != IntPtr.Zero, box == IntPtr.Zero ? "" : Win.TitleOf(box) + " : " + boxText);
        if (box != IntPtr.Zero)
        {
            Npp.Capture(box, Path.Combine(work, "1-restart-message.png"), false).Dispose();
            foreach (var b in Npp.Children(box, "Button")) { Win.PostMessage(b, Win.BM_CLICK, IntPtr.Zero, IntPtr.Zero); break; }
            Thread.Sleep(500);
        }
        string installed = Path.Combine(portable, "plugins", "TextBlinder", "TextBlinder.dll");
        bool same = File.Exists(installed) && File.ReadAllBytes(installed).SequenceEqual(File.ReadAllBytes(DllSource));
        Check("1e DLL copied to plugins\\TextBlinder\\TextBlinder.dll", same, installed);
        npp.Exit();
    }

    static void Item1Menu()
    {
        var items = npp.PluginItems("TextBlinder");
        Log("  menu: " + string.Join(" | ", items.Select(x => x.Value.Replace("\t", " <TAB> "))));
        string[] names = { "선택 영역 가리기 / 풀기", "가림 보이기 / 숨기기", "모든 가림 풀기", "설정 파일 열기", "단축키 설정..." };
        bool ok = items.Count == 5;
        for (int i = 0; ok && i < 5; i++) ok = items[i].Value.Split('\t')[0] == names[i];
        Check("1f TextBlinder menu shown in Korean after restart", ok, "");
        Check("1g default shortcuts on menu", items.Count == 5 && items[0].Value.EndsWith("\tCtrl+Shift+C") && items[1].Value.EndsWith("\tCtrl+Shift+V"),
            items.Count == 5 ? items[0].Value.Split('\t').Last() + ", " + items[1].Value.Split('\t').Last() : "");
        string ini = File.Exists(iniPath) ? File.ReadAllText(iniPath) : "";
        Check("1h settings file created with DetectedLanguage=ko", ini.Contains("DetectedLanguage=ko"), iniPath);

        for (int i = 0; i < 36; i++)
        {
            if (S(SCI_INDICGETSTYLE, i) != INDIC_FULLBOX) continue;
            Log(string.Format("  indicator {0}: FULLBOX fore={1:X6} alpha={2} outline={3} under={4}", i, S(SCI_INDICGETFORE, i),
                S(SCI_INDICGETALPHA, i), S(SCI_INDICGETOUTLINEALPHA, i), S(SCI_INDICGETUNDER, i)));
            if (selInd < 0 && S(SCI_INDICGETFORE, i) == 0x808080 && S(SCI_INDICGETALPHA, i) == 255) { selInd = i; ruleInd = i + 1; }
        }
        Check("1i two mask indicators set up (FULLBOX, #808080, alpha 255, over text)",
            selInd >= 0 && S(SCI_INDICGETSTYLE, ruleInd) == INDIC_FULLBOX && S(SCI_INDICGETUNDER, selInd) == 0,
            "select=" + selInd + " rule=" + ruleInd);
    }

    // ---- 2. 가리기 / 풀기 규칙 1 ~ 4 ----
    static void Item2Toggle()
    {
        Log("== 2. toggle rules 1-4");
        long h = At("10.0.12.34"), p = At("Tr0ub4dor&3"), a = At("sk-live-9f2a71c0e4");

        Sel(h, h + 10); Toggle();
        ExpectRanges("2a rule 2: unmasked selection gets masked", selInd, h, h + 10);
        Sel(h, h + 10); Toggle();
        ExpectRanges("2b rule 1: fully masked selection gets unmasked", selInd);

        Sel(p, p + 11); Toggle();
        Sel(p - 2, p + 5); Toggle();
        ExpectRanges("2c rule 2: partly masked selection is masked whole", selInd, p - 2, p + 11);

        Caret(p + 3); Toggle();
        ExpectRanges("2d rule 3: caret inside a mask unmasks the whole mask", selInd);
        Sel(p, p + 11); Toggle();
        Caret(p + 11); Toggle();
        ExpectRanges("2e rule 3: caret right after a mask counts as inside", selInd);

        Sel(h, h + 10); Toggle();
        S(SCI_SETSELECTION, h + 10, h);
        S(SCI_ADDSELECTION, a + 18, a);
        long n = S(SCI_GETSELECTIONS);
        Toggle();
        ExpectRanges("2f rule 4: multi-selection decided per selection (" + n + " selections)", selInd, a, a + 18);

        npp.Cmd(cmdUnmaskAll);
        long pw = At("password = "), Pw = At("Password = ");
        Sel(p, p + 11); Toggle();
        S(SCI_SETRECTANGULARSELECTIONANCHOR, pw + 11);
        S(SCI_SETRECTANGULARSELECTIONCARET, Pw + 21);
        n = S(SCI_GETSELECTIONS);
        var pieces = new List<string>();
        for (int i = 0; i < n; i++) pieces.Add("[" + S(SCI_GETSELECTIONNSTART, i) + "," + S(SCI_GETSELECTIONNEND, i) + ")");
        Toggle();
        ExpectRanges("2g rule 4: rectangular selection = one selection per line (" + n + " lines " + string.Join(" ", pieces) + ")",
            selInd, pw + 21, pw + 22, Pw + 11, Pw + 21);

        npp.Cmd(cmdUnmaskAll);
        ExpectRanges("2h unmask all clears selection masks", selInd);
    }

    // ---- 3. 가린 뒤 저장해도 원문이 남는다 ----
    static void Item3Original()
    {
        Log("== 3. original text kept on save");
        long h = At("10.0.12.34"), p = At("Tr0ub4dor&3");
        Sel(h, h + 10); Toggle();
        Sel(p, p + 11); Toggle();
        S(SCI_DOCUMENTEND);
        Type("extra = Secret99\r");
        long x = At("Secret99");
        Sel(x, x + 8); Toggle();
        npp.Cmd(IDM_FILE_SAVE);
        Thread.Sleep(300);
        string disk = File.ReadAllText(t1Path);
        Check("3a saved file keeps every masked text", disk.Contains("10.0.12.34") && disk.Contains("Tr0ub4dor&3") && disk.Contains("Secret99"), "");
        Check("3b document text (what copy uses) equals saved file", Npp.Text(sci) == disk, "");
        Check("3c document is not modified after save", S(SCI_GETMODIFY) == 0, "");
        ExpectRanges("3d masks stay after save", selInd, h, h + 10, p, p + 11, x, x + 8);
    }

    // ---- 4. 닫았다 다시 열어도 가림이 남는다 ----
    static void Item4Persist()
    {
        Log("== 4. masks survive close and reopen");
        long h = At("10.0.12.34"), p = At("Tr0ub4dor&3"), x = At("Secret99");
        string masks = File.Exists(masksPath) ? File.ReadAllText(masksPath) : "";
        int lines = masks.Split('\n').Count(l => l.Contains("t1.ini"));
        Check("4a record file has 3 entries for t1.ini and no plain text",
            lines == 3 && !masks.Contains("Tr0ub4dor") && !masks.Contains("Secret99") && !masks.Contains("10.0.12.34"), lines + " entries");

        Log(string.Format("  buffer ids: t1 before close={0:X}", npp.BufferId()));
        npp.Cmd(IDM_FILE_CLOSE);
        Log(string.Format("  buffer ids: after close (new empty doc)={0:X}", npp.BufferId()));
        npp.Cmd(IDM_FILE_RESTORELASTCLOSEDFILE);
        sci = npp.MainSci();
        Log(string.Format("  buffer ids: t1 reopened={0:X}", npp.BufferId()));
        Check("4b reopened t1.ini", Win.TitleOf(npp.Main).Contains("t1.ini"), Win.TitleOf(npp.Main));
        ExpectRanges("4c masks restored after reopen", selInd, h, h + 10, p, p + 11, x, x + 8);

        npp.Cmd(IDM_FILE_CLOSE);
        Log(string.Format("  buffer ids: after 2nd close (new empty doc)={0:X}", npp.BufferId()));
        File.WriteAllText(t1Path, File.ReadAllText(t1Path).Replace("Tr0ub4dor&3", "Tr0ub4dor&4"), new UTF8Encoding(false));
        Log("  records before 2nd reopen: " + (File.Exists(masksPath) ? File.ReadAllLines(masksPath).Count(l => l.Contains("t1.ini")) : 0) + " t1 entries");
        npp.Cmd(IDM_FILE_RESTORELASTCLOSEDFILE);
        sci = npp.MainSci();
        Log(string.Format("  buffer ids: t1 reopened 2nd={0:X}", npp.BufferId()));
        Thread.Sleep(600);  // 플러그인이 알림을 200 ms 뒤에 쓴다
        string status = npp.StatusText(0);
        ExpectRanges("4d changed text is not re-masked, others are", selInd, h, h + 10, x, x + 8);
        Check("4e status bar tells how many were skipped", status.Contains("가림 1 개 복원 안 됨"), status);

        npp.Exit();
        StartNpp("\"" + t1Path + "\"");
        WaitPluginReady();
        ExpectRanges("4f masks restored after Notepad++ restart", selInd, h, h + 10, x, x + 8);
    }

    // ---- 5. 보이기 / 숨기기, 설정 색 ----
    static void Item5VisibleAndColor()
    {
        Log("== 5. show / hide and mask color");
        long h = At("10.0.12.34");
        // 4 번 결과와 상관없이 host 값을 가린 상태에서 시작한다.
        npp.Cmd(cmdUnmaskAll);
        Sel(h, h + 10); Toggle();
        Caret(0);
        Thread.Sleep(300);
        using (var bmp = Shot("5-visible.png"))
            Observe("5a pixel on masked text while shown", Cell(bmp, h));
        npp.Cmd(cmdVisible);
        Check("5b hide: both indicators alpha 0",
            S(SCI_INDICGETALPHA, selInd) == 0 && S(SCI_INDICGETOUTLINEALPHA, selInd) == 0 && S(SCI_INDICGETALPHA, ruleInd) == 0, "");
        using (var bmp = Shot("5-hidden.png"))
            Observe("5c pixel on masked text while hidden", Cell(bmp, h));
        npp.Cmd(cmdVisible);
        Check("5d show again: alpha 255", S(SCI_INDICGETALPHA, selInd) == 255 && S(SCI_INDICGETALPHA, ruleInd) == 255, "");

        npp.Cmd(cmdOpenSettings);
        sci = npp.MainSci();
        string ini = Npp.Text(sci);
        Check("5e settings file opened", ini.Contains("[Display]") && Win.TitleOf(npp.Main).Contains("TextBlinder.ini"), Win.TitleOf(npp.Main));
        npp.SetDocText(sci, ini.Replace("MaskColor=#808080", "MaskColor=#FF0000"));
        npp.Cmd(IDM_FILE_SAVE);
        Thread.Sleep(300);
        Check("5f saving MaskColor=#FF0000 applies at once (BGR 0000FF)", S(SCI_INDICGETFORE, selInd) == 0x0000FF && S(SCI_INDICGETFORE, ruleInd) == 0x0000FF,
            string.Format("fore={0:X6}", S(SCI_INDICGETFORE, selInd)));
    }

    // ---- 6. 규칙 가림 ----
    static void Item6Rules()
    {
        Log("== 6. rule masks");
        string ini = Npp.Text(sci);
        ini = ini.TrimEnd('\r', '\n') + "\r\nRule1=password\\s*=\\s*\\K.+\r\nRule2=(?i)API_KEY\\s*=\\s*\\K.+\r\nRule3=([bad\r\n";
        npp.SetDocText(sci, ini);
        npp.Cmd(IDM_FILE_SAVE);
        Thread.Sleep(300);
        Thread.Sleep(600);  // 플러그인이 알림을 200 ms 뒤에 쓴다
        string status = npp.StatusText(0);
        Check("6a invalid rule reported on status bar", status.Contains("Rule3"), status);

        ActivateByTitle("t1.ini");
        long p = At("Tr0ub4dor&4"), a = At("sk-live-9f2a71c0e4");
        ExpectRanges("6b rules mask values: \\K keeps key visible, case kept for Rule1, (?i) for Rule2", ruleInd, p, p + 11, a, a + 18);

        npp.Cmd(IDM_FILE_CLOSE);
        npp.Cmd(IDM_FILE_RESTORELASTCLOSEDFILE);
        sci = npp.MainSci();
        ExpectRanges("6c rules applied as soon as the file opens", ruleInd, p, p + 11, a, a + 18);

        S(SCI_DOCUMENTEND);
        Type("password = abc");
        Thread.Sleep(800);
        long q = At("password = abc") + 11;
        ExpectRanges("6d new matching text is masked shortly after typing", ruleInd, p, p + 11, a, a + 18, q, q + 3);
        UndoAll();
        using (var bmp = Shot("6-rules.png"))
            Observe("6e pixel on rule-masked value (mask color now #FF0000)", Cell(bmp, p));
    }

    // ---- 7. 11 절 화면 항목 ----
    static void Item7Screens()
    {
        Log("== 7. section 11 screen items");
        IntPtr mainSci = sci;
        long h = At("10.0.12.34");

        // 찾기 결과 창
        npp.PostCmd(IDM_SEARCH_FIND);
        IntPtr find = Npp.WaitFor(delegate
        {
            IntPtr found = IntPtr.Zero;
            Win.EnumWindows(delegate (IntPtr w, IntPtr l)
            {
                if (Win.PidOf(w) == npp.Proc.Id && Win.IsWindowVisible(w) && Npp.ChildById(w, 1601, null) != IntPtr.Zero) { found = w; return false; }
                return true;
            }, IntPtr.Zero);
            return found;
        }, 5000);
        if (find != IntPtr.Zero)
        {
            IntPtr combo = Npp.ChildById(find, 1601, null);
            var edits = Npp.Children(combo, "Edit");
            if (edits.Count > 0) Npp.SetText(edits[0], "Tr0ub4dor");
            Win.SendMessage(Npp.ChildById(find, 1641, "Button"), Win.BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            Thread.Sleep(1000);
            Win.PostMessage(find, Win.WM_CLOSE, IntPtr.Zero, IntPtr.Zero);
            Thread.Sleep(300);
            IntPtr finder = IntPtr.Zero;
            foreach (var w in Npp.Children(npp.Main, "Scintilla"))
                if (w != mainSci && Win.IsWindowVisible(w) && Npp.Text(w).Contains("Tr0ub4dor")) { finder = w; break; }
            if (finder != IntPtr.Zero)
            {
                Npp.Sci(finder, 2630, 0); Thread.Sleep(300);
                Npp.Capture(finder, Path.Combine(work, "7-find-results.png"), true).Dispose();
                string line = Npp.Text(finder).Split('\n').LastOrDefault(l => l.Contains("Tr0ub4dor")) ?? "";
                Observe("7a find results window shows the text", line.Trim());
                Thread.Sleep(1500);
                Check("7a' plugin leaves the find results window alone (different document)",
                    Npp.Sci(finder, SCI_INDICGETSTYLE, selInd) != INDIC_FULLBOX, "style=" + Npp.Sci(finder, SCI_INDICGETSTYLE, selInd));
            }
            else Observe("7a find results window", "not found");
        }
        else Observe("7a find dialog", "not opened");
        sci = mainSci;

        // Document Map
        var before = new HashSet<IntPtr>(Npp.Children(npp.Main, "Scintilla"));
        npp.Cmd(IDM_VIEW_DOC_MAP);
        Thread.Sleep(800);
        IntPtr map = IntPtr.Zero;
        Win.EnumWindows(delegate (IntPtr w, IntPtr l)
        {
            if (Win.PidOf(w) != npp.Proc.Id) return true;
            foreach (var c in Npp.Children(w, "Scintilla"))
                if (!before.Contains(c) && Win.IsWindowVisible(c)) { map = c; return false; }
            return true;
        }, IntPtr.Zero);
        if (map != IntPtr.Zero)
        {
            // 플러그인은 1 초마다 같은 문서를 보여 주는 창을 찾아 모양을 설정한다.
            Thread.Sleep(1500);
            Npp.Sci(map, 2630, 0); Thread.Sleep(300);
            int red = 0;
            using (var bmp = Npp.Capture(map, Path.Combine(work, "7-document-map.png"), true))
                for (int y = 0; y < bmp.Height; y++)
                    for (int x = 0; x < bmp.Width; x++)
                        if ((bmp.GetPixel(x, y).ToArgb() & 0xFFFFFF) == 0xFF0000) red++;
            Check("7b document map draws mask boxes like the editor",
                Npp.Sci(map, SCI_INDICGETSTYLE, selInd) == INDIC_FULLBOX && Npp.Sci(map, SCI_INDICGETALPHA, selInd) == 255
                && Npp.Sci(map, SCI_INDICGETSTYLE, ruleInd) == INDIC_FULLBOX && red > 0,
                string.Format("style={0} alpha={1} rule style={2}, mask-color pixels in capture={3}", Npp.Sci(map, SCI_INDICGETSTYLE, selInd),
                    Npp.Sci(map, SCI_INDICGETALPHA, selInd), Npp.Sci(map, SCI_INDICGETSTYLE, ruleInd), red));
            npp.Cmd(IDM_VIEW_DOC_MAP);
        }
        else Observe("7b document map", "not found");
        Npp.Capture(npp.Main, Path.Combine(work, "7-main.png"), false).Dispose();

        // 선택 색과 가림
        if (Fmt(Ranges(selInd)).IndexOf("[" + h + "," + (h + 10) + ")") < 0) { Sel(h, h + 10); Toggle(); }
        Caret(0);
        Thread.Sleep(300);
        using (var bmp = Shot("7-before-select.png")) Observe("7c pixel on masked text, not selected", Cell(bmp, h));
        Sel(h, h + 10);
        Thread.Sleep(300);
        using (var bmp = Shot("7-selected.png")) Observe("7d pixel on masked text, selected", Cell(bmp, h));

        // Smart Highlighting 과 겹칠 때
        long u = At("admin", 1), v = At("admin", 2);
        Sel(u, u + 5); Toggle();
        Sel(v, v + 5);
        Thread.Sleep(800);
        long smart = S(SCI_INDICATORVALUEAT, SMART_HIGHLIGHT, u);
        using (var bmp = Shot("7-smart-highlight.png"))
            Observe("7e masked word also smart-highlighted", "smart indicator on masked word=" + smart + ", pixel=" + Cell(bmp, u));
        Caret(0);
        Observe("7f print", "needs a printer and the print dialog; not automated");
    }

    // ---- 8. 11 절 편집과 단축키 ----
    static void Item8EditAndKeys()
    {
        Log("== 8. section 11 editing and shortcuts");
        npp.Cmd(cmdUnmaskAll);
        long p = At("Tr0ub4dor&4");
        Sel(p, p + 11); Toggle();

        S(SCI_GOTOPOS, p + 3); Type("X");
        Observe("8a type inside a mask", "mask before [" + p + "," + (p + 11) + ") -> after " + Fmt(Ranges(selInd)));
        long s0 = Ranges(selInd)[0][0], e0 = Ranges(selInd)[0][1];
        S(SCI_GOTOPOS, s0); Type("Y");
        Observe("8b type at mask start", "before [" + s0 + "," + e0 + ") -> after " + Fmt(Ranges(selInd)));
        s0 = Ranges(selInd)[0][0]; e0 = Ranges(selInd)[0][1];
        S(SCI_GOTOPOS, e0); Type("Z");
        Observe("8c type right after mask end", "before [" + s0 + "," + e0 + ") -> after " + Fmt(Ranges(selInd)));
        s0 = Ranges(selInd)[0][0]; e0 = Ranges(selInd)[0][1];
        S(SCI_DELETERANGE, s0 + 2, 2);
        Observe("8d delete 2 chars inside mask", "before [" + s0 + "," + e0 + ") -> after " + Fmt(Ranges(selInd)));
        UndoAll();

        var menus = npp.AllMenuTexts();
        var c = menus.Where(m => m.EndsWith("\tCtrl+Shift+C")).ToList();
        var v = menus.Where(m => m.EndsWith("\tCtrl+Shift+V")).ToList();
        Check("8e Ctrl+Shift+C used only by TextBlinder in menus", c.Count == 1 && c[0].Contains("TextBlinder"), string.Join(" | ", c));
        Check("8f Ctrl+Shift+V used only by TextBlinder in menus", v.Count == 1 && v[0].Contains("TextBlinder"), string.Join(" | ", v));
        string sc = File.ReadAllText(Path.Combine(portable, "shortcuts.xml"));
        var hits = Regex.Matches(sc, "<[^>]*Ctrl=\"yes\"[^>]*Alt=\"no\"[^>]*Shift=\"yes\"[^>]*Key=\"(67|86)\"[^>]*>");
        Check("8g user's shortcuts.xml has no Ctrl+Shift+C / V", hits.Count == 0, hits.Count + " hits");
        Observe("8h plugins in this copy", string.Join(", ", Directory.GetDirectories(Path.Combine(portable, "plugins")).Select(Path.GetFileName)));
    }
}
