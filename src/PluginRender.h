#pragma once

#include <kthook/kthook.hpp>
#include <d3d9.h>
#include <optional>
#include <vector>
#include <imgui.h> // <--- เพิ่มบรรทัดนี้เข้ามาครับ!

// โครงสร้างเก็บข้อมูลพาร์ติเคิลที่ลอยในพื้นหลัง
struct BackgroundParticle {
    float x, y;
    float radius;
    float speedY;
    ImU32 color; // ตอนนี้คอมไพเลอร์จะรู้จัก ImU32 แล้ว
};

using PresentSignature = HRESULT(__stdcall*)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
using ResetSignature = HRESULT(__stdcall*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);

class PluginRender {
private:
    bool ImGuiinited = false;
    bool isFinished = false; // โหลดเสร็จสมบูรณ์ -> ปิดการทำงานเพื่อคืนทรัพยากร 100%
    
    // สถานะของ Load Screen
    float loadScreenAlpha = 1.0f;
    bool isGameLoaded = false;
    float loadProgress = 0.0f;

    // --- ตัวแปรสำหรับ UI ---
    std::vector<BackgroundParticle> particles;
    void initParticles();
    
    IDirect3DTexture9* logoTexture = nullptr; 
    IDirect3DStateBlock9* pStateBlock = nullptr;
    ImVec2 logoSize = ImVec2(250.0f, 100.0f);
    ImFont* m_font = nullptr;
    // -----------------------------

    void drawLoadScreen();
    void cleanup();

    std::uintptr_t findDevice(std::uint32_t Len);
    void* getFunctionAddress(int VTableIndex);

    kthook::kthook_signal<PresentSignature> hookPresent{};
    kthook::kthook_signal<ResetSignature> hookReset{};
    kthook::kthook_simple<WNDPROC> hookWndproc{};

    std::optional<HRESULT> onPresent(const decltype(hookPresent)& hook, IDirect3DDevice9* pDevice, const RECT*, const RECT*, HWND, const RGNDATA*);
    std::optional<HRESULT> onLost(const decltype(hookReset)& hook, IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* parameters);
    void onReset(const decltype(hookReset)& hook, HRESULT& returnValue, IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* parameters);
    HRESULT __stdcall onWndproc(const decltype(hookWndproc)& hook, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
    PluginRender();
    ~PluginRender();
};