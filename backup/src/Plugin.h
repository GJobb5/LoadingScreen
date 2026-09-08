#pragma once

#include "PluginRender.h"
#include <windows.h>

class Plugin {
public:
    Plugin(HMODULE hModule);
    HMODULE hModule;
private:
    PluginRender render; // คลาสสำหรับวาด LoadScreen และจัดการ ImGui
};