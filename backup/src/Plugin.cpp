#include "Plugin.h"

Plugin::Plugin(HMODULE hndl) : hModule(hndl) {
    // การสร้าง PluginRender (ตัวแปร render ใน Plugin.h) 
    // จะไปเรียก Constructor ของมัน ซึ่งจะทำการ Hook DirectX และจัดการ Load Screen ให้อัตโนมัติทันที
}