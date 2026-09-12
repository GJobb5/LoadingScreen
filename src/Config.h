#pragma once
#include <windows.h>
#include "resource.h"

// ============================================================================
//                       LOADING SCREEN CONFIGURATION
//                 (ศูนย์รวมการตั้งค่าทั้งหมดของปลั๊กอินหน้าจอโหลด)
// ============================================================================

namespace Config {

    // ─── 1. ข้อมูลปลั๊กอิน & การแสดงผล (Plugin Metadata) ─────────────────────────
    constexpr const char* PLUGIN_NAME        = "Custom Loading Screen";
    constexpr const char* PLUGIN_VERSION     = "1.2.0";
    constexpr const char* PLUGIN_AUTHOR      = "Spark Team";

    // ─── 2. ข้อมูลแบรนด์เซิร์ฟเวอร์ (Server Branding) ────────────────────────────
    constexpr const char* SERVER_NAME        = "Spark Global";
    constexpr const char* FOOTER_CREDIT      = "Powered By Spark Team";

    // ─── 3. ระบบป้องกันการใช้งานข้ามเซิร์ฟเวอร์ (Server Protection / IP Lock) ────
    // หากเปิดใช้งาน (true) ปลั๊กอินจะทำงานเฉพาะบน Server IP ที่ระบุไว้เท่านั้น
    constexpr bool ENABLE_IP_PROTECTION      = false;

    constexpr const char* ALLOWED_SERVER_IPS[] = {
        "154.215.14.148",
        "127.0.0.1"
    };

    // ─── 4. ข้อมูล Resource (Embedded Resources ในตัวไฟล์ .asi เหมือน sampvoice) ───
    constexpr int  LOGO_RESOURCE_ID          = IDB_PNG_LOGO;
    constexpr const char* LOGO_RESOURCE_TYPE = "PNG";

    constexpr int  FONT_RESOURCE_ID          = IDR_FONT1;
    constexpr const char* FONT_RESOURCE_TYPE = "RCDATA";

    // ฟอนต์สำรองกรณีฉุกเฉิน (หากไม่สามารถโหลด Resource ได้)
    constexpr const char* PATH_FONT_FALLBACK = "C:\\Windows\\Fonts\\tahoma.ttf";

    // ─── 5. ธีมและโทนสี (Theme & Color Scheme) ──────────────────────────────────
    namespace Theme {
        // สีไฮไลท์หลัก (Accent Color) เช่น แถบโหลด, แสงหมุน, แถบหัวข้อ
        // ค่าสีในรูปแบบ R, G, B (0.0f - 1.0f) ค่าเริ่มต้นคือสีชมพูพาสเทล
        constexpr float ACCENT_R             = 1.0000f; // 255
        constexpr float ACCENT_G             = 0.7686f; // 196
        constexpr float ACCENT_B             = 0.9137f; // 233

        // สีพื้นหลังจอโหลด (Background RGB)
        constexpr float BG_R                 = 0.02f;
        constexpr float BG_G                 = 0.03f;
        constexpr float BG_B                 = 0.04f;
    }

    // ─── 6. ปรับแต่งเอฟเฟกต์ & แอนิเมชัน (Visual Effects) ────────────────────────
    namespace Visuals {
        constexpr float MAX_LOGO_DIMENSION   = 300.0f; // ขนาดโลโก้สูงสุด (กว้าง/สูง)
        constexpr float FADE_OUT_SPEED       = 1.5f;   // ความเร็วการจางหายของหน้าจอโหลด
        constexpr int   PARTICLE_COUNT       = 80;     // จำนวนอนุภาคลอยในพื้นหลัง
    }
}
