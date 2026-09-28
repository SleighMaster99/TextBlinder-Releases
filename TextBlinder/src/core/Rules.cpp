// SPDX-License-Identifier: GPL-3.0-only
#include "Rules.h"

#include "Text.h"

namespace tb
{
std::vector<Rule> ReadRules(const Ini& ini)
{
    std::vector<Rule> rules;
    for (const IniEntry& e : ini.Section("Rules"))
    {
        if (StartsWithNoCase(e.key, "Rule") && !e.value.empty())
            rules.push_back({e.key, e.value});
    }
    return rules;
}
}
