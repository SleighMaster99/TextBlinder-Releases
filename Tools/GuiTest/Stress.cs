// SPDX-License-Identifier: GPL-3.0-only
// 닫기와 다시 열기를 반복해서, 버퍼 ID 가 재사용될 때 가림 복원이 빠지는지 본다.
using System;
using System.Collections.Generic;
using System.IO;

static partial class Program
{
    static void StressReopen(string workDir)
    {
        work = workDir;
        t1Path = Path.Combine(work, "t1.ini");
        string cfg = Path.Combine(portable, "plugins", "Config");
        masksPath = Path.Combine(cfg, "TextBlinder.masks");
        if (File.Exists(masksPath)) File.Delete(masksPath);
        File.WriteAllText(t1Path, TestText, new System.Text.UTF8Encoding(false));
        StartNpp("\"" + t1Path + "\"");
        try
        {
            WaitPluginReady();
            for (int i = 0; i < 36; i++) if (S(SCI_INDICGETSTYLE, i) == INDIC_FULLBOX && selInd < 0) { selInd = i; ruleInd = i + 1; }
            long h = At("10.0.12.34");
            Sel(h, h + 10); Toggle();
            Caret(0);
            var seen = new Dictionary<long, string>();
            int bad = 0, emptyReuse = 0;
            for (int round = 1; round <= 60; round++)
            {
                long t1 = npp.BufferId();
                seen[t1] = "t1#" + round;
                npp.Cmd(IDM_FILE_CLOSE);
                long empty = npp.BufferId();
                string emptyNote = seen.ContainsKey(empty) ? " (reuses " + seen[empty] + ")" : "";
                seen[empty] = "empty#" + round;
                npp.Cmd(IDM_FILE_RESTORELASTCLOSEDFILE);
                sci = npp.MainSci();
                long re = npp.BufferId();
                string reNote = seen.ContainsKey(re) ? " (reuses " + seen[re] + ")" : "";
                var r = Ranges(selInd);
                bool ok = Fmt(r) == "[" + h + "," + (h + 10) + ")";
                if (!ok) bad++;
                if (reNote.Contains("empty#")) emptyReuse++;
                Log(string.Format("round {0}: t1={1:X} empty={2:X}{3} reopened={4:X}{5} masks={6} {7}",
                    round, t1, empty, emptyNote, re, reNote, Fmt(r), ok ? "ok" : "MISSING"));
            }
            Check("stress: masks restored in every reopen", bad == 0, bad + " bad rounds, reopened file got a replaced empty doc's id in " + emptyReuse + " rounds");
        }
        finally { npp.Dispose(); }
    }
}
