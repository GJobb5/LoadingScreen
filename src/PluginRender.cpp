#include "PluginRender.h"
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>
#include <sampapi/CNetGame.h>
#include <sampapi/CLocalPlayer.h>
#include <windows.h>
#include <string>

// นำเข้า stb_image แทน d3dx9
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using InitGameInstance = HWND(__cdecl*)(HINSTANCE);
kthook::kthook_signal<InitGameInstance> hookGameInstanceInit{ 0x745560 };
HWND gameHwnd = []() {
    // แก้ไขการอ่าน HWND ให้ถูกต้อง (0xC17054 เก็บค่า Handle ตรงๆ ไม่ใช่ Pointer ซ้อน)
    HWND hwnd = *reinterpret_cast<HWND*>(0xC17054);
    if (hwnd != NULL) {
        return hwnd;
    } else {
        hookGameInstanceInit.after += [](const auto& hook, HWND& returnValue, HINSTANCE inst) {
            gameHwnd = returnValue;
        };
        return HWND(0);
    }
}();

// ============================================================
//  ระบบ Lock IP (อนุญาตเฉพาะเซิร์ฟเวอร์ที่กำหนด)
// ============================================================
static constexpr const char* ALLOWED_SERVER_IPS[] = {
    "154.215.14.148",
    "127.0.0.1"
};

static bool CheckServerIP() {
    std::string cmdLine = GetCommandLineA();
    for (const char* ip : ALLOWED_SERVER_IPS) {
        std::string targetArg = "-h ";
        targetArg += ip;
        if (cmdLine.find(targetArg) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// ==========================================
// ฟังก์ชันโหลดรูปด้วย stb_image
// ==========================================
bool LoadTextureFromFile(const char* filename, PDIRECT3DDEVICE9 d3dDevice, PDIRECT3DTEXTURE9* out_texture, float* out_width, float* out_height) {
    if (filename == nullptr || d3dDevice == nullptr || out_texture == nullptr || out_width == nullptr || out_height == nullptr) return false;
    *out_texture = nullptr;
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL) return false;

    D3DLOCKED_RECT locked_rect;
    if (d3dDevice->CreateTexture(image_width, image_height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, out_texture, NULL) != D3D_OK) {
        stbi_image_free(image_data);
        return false;
    }

    if ((*out_texture)->LockRect(0, &locked_rect, NULL, 0) != D3D_OK) {
        (*out_texture)->Release();
        *out_texture = nullptr;
        stbi_image_free(image_data);
        return false;
    }

    // คัดลอกพิกเซลและสลับสีจาก RGBA (stb) เป็น BGRA (DirectX)
    uint8_t* dest = (uint8_t*)locked_rect.pBits;
    uint8_t* src = image_data;
    for (int y = 0; y < image_height; y++) {
        for (int x = 0; x < image_width; x++) {
            dest[x * 4 + 0] = src[x * 4 + 2]; // B
            dest[x * 4 + 1] = src[x * 4 + 1]; // G
            dest[x * 4 + 2] = src[x * 4 + 0]; // R
            dest[x * 4 + 3] = src[x * 4 + 3]; // A
        }
        dest += locked_rect.Pitch;
        src += image_width * 4;
    }
    (*out_texture)->UnlockRect(0);
    stbi_image_free(image_data);

    // ดึงความกว้างและความสูงกลับไปปรับให้ logoSize อัตโนมัติ!
    *out_width = (float)image_width;
    *out_height = (float)image_height;
    return true;
}

PluginRender::PluginRender() {
    using namespace std::placeholders;
    hookPresent.set_dest(getFunctionAddress(17));
    hookReset.set_dest(getFunctionAddress(16));
    hookPresent.before += std::bind(&PluginRender::onPresent, this, _1, _2, _3, _4, _5, _6);
    hookReset.before += std::bind(&PluginRender::onLost, this, _1, _2, _3);
    hookReset.after += std::bind(&PluginRender::onReset, this, _1, _2, _3, _4);
    hookPresent.install();
    hookReset.install();
}

PluginRender::~PluginRender() {
    if (logoTexture != nullptr) {
        logoTexture->Release();
        logoTexture = nullptr;
    }
    if (ImGuiinited && ImGui::GetCurrentContext()) {
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        ImGuiinited = false;
    }
}

std::uintptr_t PluginRender::findDevice(std::uint32_t len) {
    static std::uintptr_t base = [](std::size_t len) {
        std::string pathTo(MAX_PATH, '\0');
        if (auto size = GetSystemDirectoryA(pathTo.data(), MAX_PATH)) {
            pathTo.resize(size);
            pathTo += "\\d3d9.dll";
            std::uintptr_t dwObjBase = reinterpret_cast<std::uintptr_t>(LoadLibraryA(pathTo.c_str()));
            while (dwObjBase++ < dwObjBase + len) {
                if (*reinterpret_cast<std::uint16_t*>(dwObjBase + 0x00) == 0x06C7 &&
                    *reinterpret_cast<std::uint16_t*>(dwObjBase + 0x06) == 0x8689 &&
                    *reinterpret_cast<std::uint16_t*>(dwObjBase + 0x0C) == 0x8689) {
                    dwObjBase += 2;
                    break;
                }
            }
            return dwObjBase;
        }
        return std::uintptr_t(0);
    }(len);
    return base;
}

void* PluginRender::getFunctionAddress(int VTableIndex) {
    return (*reinterpret_cast<void***>(findDevice(0x128000)))[VTableIndex];
}

std::optional<HRESULT> PluginRender::onPresent(const decltype(hookPresent)& hook, IDirect3DDevice9* pDevice, const RECT*, const RECT*, HWND, const RGNDATA*) {
    if (!CheckServerIP()) {
        return std::nullopt;
    }
    
    // สำคัญ: ป้องกันการโหลด ImGui ก่อนที่เกมจะสร้างหน้าต่างเสร็จสมบูรณ์
    if (!gameHwnd) {
        HWND hwnd = *reinterpret_cast<HWND*>(0xC17054);
        if (hwnd) {
            gameHwnd = hwnd;
        } else {
            return std::nullopt; // รอจนกว่าจะได้หน้าต่าง
        }
    }

    if (!ImGuiinited) {
        using namespace std::placeholders;
        auto pLatestWndproc = GetWindowLongPtrA(gameHwnd, GWLP_WNDPROC);
        if (pLatestWndproc) {
            hookWndproc.set_dest(pLatestWndproc);
            hookWndproc.set_cb(std::bind(&PluginRender::onWndproc, this, _1, _2, _3, _4, _5));
            hookWndproc.install();
        }

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.MouseDrawCursor = false;
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

        // ==========================================
        // โหลด Font
        // ==========================================
        ImFontConfig font_config;
        font_config.OversampleH = 2;
        font_config.OversampleV = 2;
        static const ImWchar ranges[] = {
            0x0020, 0x00FF, // ภาษาอังกฤษ
            0x0E00, 0x0E7F, // ภาษาไทย
            0,
        };

        const char* customFontPath = "Spark\\fonts\\Kanit-Bold.ttf";
        const char* fallbackFontPath = "C:\\Windows\\Fonts\\tahoma.ttf";
        
        if (GetFileAttributesA(customFontPath) != INVALID_FILE_ATTRIBUTES) {
            io.Fonts->AddFontFromFileTTF(customFontPath, 20.0f, &font_config, ranges); 
        } else if (GetFileAttributesA(fallbackFontPath) != INVALID_FILE_ATTRIBUTES) {
            io.Fonts->AddFontFromFileTTF(fallbackFontPath, 16.0f, &font_config, ranges);
        } else {
            io.Fonts->AddFontDefault(); // กันแครชกรณีหาฟอนต์ไม่เจอเลย
        }

        ImGui_ImplWin32_Init(gameHwnd);
        ImGui_ImplDX9_Init(pDevice);

        // ==========================================
        // โหลด Logo ด้วย stb_image
        // ==========================================
        const char* logoPath = "Spark\\logo.png";
        if (GetFileAttributesA(logoPath) != INVALID_FILE_ATTRIBUTES) {
            if (LoadTextureFromFile(logoPath, pDevice, &logoTexture, &logoSize.x, &logoSize.y)) {
                // ย่อขนาดรูปภาพไม่ให้เกิน 300 พิกเซล
                float maxDimension = 300.0f;
                float currentMax = (logoSize.x > logoSize.y) ? logoSize.x : logoSize.y;
                
                if (currentMax > maxDimension) {
                    float scale = maxDimension / currentMax;
                    logoSize.x *= scale;
                    logoSize.y *= scale;
                }
            }
        }
        
        ImGuiinited = true;
    }
    
    if (ImGui::GetCurrentContext()) {
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // วาด Load Screen จนกว่าตัวละครจะเกิด (Spawn)
        if (loadScreenAlpha > 0.0f) {
            drawLoadScreen();
        }

        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    }
    return std::nullopt;
}

std::optional<HRESULT> PluginRender::onLost(const decltype(hookReset)& hook, IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* parameters) {
    if (ImGuiinited && ImGui::GetCurrentContext()) {
        ImGui_ImplDX9_InvalidateDeviceObjects();
    }
    return std::nullopt;
}

void PluginRender::onReset(const decltype(hookReset)& hook, HRESULT& returnValue, IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* parameters) {
    if (ImGuiinited && ImGui::GetCurrentContext() && SUCCEEDED(returnValue)) {
        ImGui_ImplDX9_CreateDeviceObjects();
    }
}

HRESULT __stdcall PluginRender::onWndproc(const decltype(hookWndproc)& hook, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (ImGui::GetCurrentContext()) {
        ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam);
    }
    return hook.get_trampoline()(hwnd, uMsg, wParam, lParam);
}

void PluginRender::initParticles() {
    if (!particles.empty()) return;
    
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    if (screenSize.x <= 1.0f || screenSize.y <= 1.0f) return;
    particles.reserve(80);
    for (int i = 0; i < 80; i++) {
        BackgroundParticle p;
        p.x = (float)(rand() % (int)screenSize.x);
        p.y = (float)(rand() % (int)screenSize.y);
        p.radius = (float)(rand() % 40 + 15) / 10.0f;
        p.speedY = (float)(rand() % 30 + 10) / 20.0f;

        int colorType = rand() % 4;
        int alpha = rand() % 150 + 50;
        if (colorType == 0) p.color = IM_COL32(255, 255, 255, alpha);          
        else if (colorType == 1) p.color = IM_COL32(255, 200, 230, alpha);     
        else if (colorType == 2) p.color = IM_COL32(200, 100, 180, alpha);     
        else p.color = IM_COL32(200, 180, 200, alpha);                         
        
        particles.push_back(p);
    }
}

void PluginRender::drawLoadScreen() {
    if (loadScreenAlpha <= 0.0f) return;

    ImGuiIO& io = ImGui::GetIO();
    float deltaTime = io.DeltaTime;
    ImVec2 screenSize = io.DisplaySize;
    if (!(deltaTime > 0.0f) || deltaTime > 0.10f) deltaTime = 1.0f / 60.0f;

    namespace samp = sampapi::v037r1;
    HMODULE sampModule = GetModuleHandleA("samp.dll");
    bool networkReady = false;
    if (sampModule != nullptr) {
        auto pNetGame = samp::RefNetGame();
        if (pNetGame != nullptr) {
            networkReady = true;
            auto pool = pNetGame->GetPlayerPool();
            if (pool && pool->GetLocalPlayer() != nullptr) {
                isGameLoaded = true;
            }
        }
    }

    if (!isGameLoaded) {
        float targetProgress = 0.25f;
        if (sampModule != nullptr) targetProgress = networkReady ? 0.90f : 0.60f;
        float blend = deltaTime * 4.0f;
        if (blend > 1.0f) blend = 1.0f;
        loadProgress += (targetProgress - loadProgress) * blend;
        if (loadProgress < 0.0f) loadProgress = 0.0f;
        if (loadProgress > targetProgress) loadProgress = targetProgress;
    } else {
        loadProgress = 1.0f;
        loadScreenAlpha -= deltaTime * 1.5f;
        if (loadScreenAlpha < 0.0f) loadScreenAlpha = 0.0f;
    }    ImVec2 center = ImVec2(screenSize.x / 2.0f, screenSize.y / 2.0f);

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(screenSize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.03f, 0.04f, loadScreenAlpha));
    ImGui::Begin("##CustomLoadScreen", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    float time = (float)ImGui::GetTime();
    initParticles();
    for (auto& p : particles) {
        p.y -= p.speedY * deltaTime * 60.0f;
        if (p.y < -20.0f) {
            p.y = screenSize.y + 20.0f;
            p.x = (float)(rand() % (int)screenSize.x);
        }
        ImU32 pCol = (p.color & 0x00FFFFFF) | ((int)(((p.color >> 24) & 0xFF) * loadScreenAlpha) << 24);
        dl->AddCircleFilled(ImVec2(p.x, p.y), p.radius, pCol);
    }

    ImU32 textCol = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, loadScreenAlpha));
    ImU32 pinkAccent = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.7686f, 0.9137f, loadScreenAlpha));
    dl->AddRectFilled(ImVec2(30, 30), ImVec2(34, 52), pinkAccent);
    dl->AddText(ImGui::GetFont(), 18.0f, ImVec2(45, 30), textCol, "Spark Global");
    const char* trText = "Powered By Spark Team";
    ImVec2 trSize = ImGui::CalcTextSize(trText);
    dl->AddText(ImVec2(screenSize.x - trSize.x - 30, 30), ImGui::ColorConvertFloat4ToU32(ImVec4(0.7f, 0.7f, 0.7f, loadScreenAlpha)), trText);

    float bounceScale = 1.0f + (sinf(time * 3.5f) * 0.05f);
    float bounceY = sinf(time * 2.5f) * 8.0f;
    ImVec2 currentLogoSize = ImVec2(logoSize.x * bounceScale, logoSize.y * bounceScale);
    if (logoTexture != nullptr) {
        dl->AddImage((void*)logoTexture,
                     ImVec2(center.x - currentLogoSize.x/2, center.y - currentLogoSize.y/2 + bounceY),
                     ImVec2(center.x + currentLogoSize.x/2, center.y + currentLogoSize.y/2 + bounceY),
                     ImVec2(0,0), ImVec2(1,1), ImColor(255, 255, 255, (int)(255 * loadScreenAlpha)));
    } else {
        const char* logoText = "[ LOGO ]";
        ImGui::SetWindowFontScale(2.5f * bounceScale);
        ImVec2 ltSize = ImGui::CalcTextSize(logoText);
        dl->AddText(ImVec2(center.x - ltSize.x/2, center.y - ltSize.y/2 + bounceY), textCol, logoText);
        ImGui::SetWindowFontScale(1.0f);
    }

    float barH = 6.0f;
    float barY = screenSize.y - barH;
    dl->AddRectFilled(ImVec2(0, barY), ImVec2(screenSize.x, screenSize.y), ImGui::ColorConvertFloat4ToU32(ImVec4(0.1f, 0.1f, 0.1f, loadScreenAlpha)));
    dl->AddRectFilled(ImVec2(0, barY), ImVec2(screenSize.x * loadProgress, screenSize.y), pinkAccent);
    char loadText[64];
    sprintf_s(loadText, "Loading game %d%%", (int)(loadProgress * 100.0f));
    ImVec2 brSize = ImGui::CalcTextSize(loadText);
    float brX = screenSize.x - brSize.x - 55;
    float brY = barY - 25;
    dl->AddText(ImVec2(brX, brY), textCol, loadText);

    float sRadius = 5.0f;
    ImVec2 sCenter = ImVec2(screenSize.x - 30, brY + 7);
    dl->AddCircle(sCenter, sRadius, ImGui::ColorConvertFloat4ToU32(ImVec4(0.2f, 0.2f, 0.2f, loadScreenAlpha)), 16, 2.0f);
    dl->PathArcTo(sCenter, sRadius, time * 8.0f, time * 8.0f + 3.14f, 16);
    dl->PathStroke(pinkAccent, false, 2.0f);

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}