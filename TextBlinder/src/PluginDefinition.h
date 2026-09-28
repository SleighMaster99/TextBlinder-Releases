// SPDX-License-Identifier: GPL-3.0-only
// This file is part of TextBlinder, based on PluginDefinition.h of the Notepad++ plugin template.
// Copyright (C)2022 Don HO <don.h@free.fr>
// Copyright (C)2026 SleighMaster99
#pragma once

#include "PluginInterface.h"

// plugins\TextBlinder\TextBlinder.dll 의 폴더 이름과 같아야 한다.
const wchar_t NPP_PLUGIN_NAME[] = L"TextBlinder";

// 메뉴 명령 순서. funcItem 배열의 번호와 같다.
enum CommandIndex
{
    kCmdToggleMask = 0,
    kCmdToggleVisible,
    kCmdUnmaskAll,
    kCmdOpenSettings,
    kCmdShortcutMapper,
    kCommandCount
};

constexpr int nbFunc = kCommandCount;

void pluginInit(HANDLE hModule);
void pluginCleanUp();
void commandMenuInit();
void onNotify(SCNotification* notifyCode);
