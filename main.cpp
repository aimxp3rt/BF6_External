#include "driver.h"
#include "game_data.h"
#include "esp.h"
#include "aimbot.h"
#include "radar.h"

#include "../Project1/imgui/imgui.h"
#include "../Project1/imgui/imgui_impl_win32.h"
#include "../Project1/imgui/imgui_impl_dx11.h"
#include <d3d11.h>
#include <iostream>
#include <memory>

// DirectX 11 globals
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Window globals
static HWND g_hWnd = nullptr;
static WNDCLASSEX g_wc = {};

// Forward declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Application state
struct AppState {
    bool show_menu;
    bool show_esp;
    bool show_radar;
    bool show_aimbot;
    
    AppState() : show_menu(true), show_esp(true), show_radar(true), show_aimbot(false) {}
};

void RenderMenu(AppState& state, BF6::ESPRenderer& esp, BF6::Aimbot& aimbot, BF6::RadarRenderer& radar) {
    if (!state.show_menu) return;

    ImGui::Begin("BF6 KILLSTARS", &state.show_menu, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Separator();
    
    // ESP Settings
    if (ImGui::CollapsingHeader("ESP Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enable ESP", &state.show_esp);
        
        auto& esp_config = esp.GetConfig();
        ImGui::Checkbox("Box ESP", &esp_config.box_enabled);
        ImGui::Checkbox("Skeleton ESP", &esp_config.skeleton_enabled);
        ImGui::Checkbox("Health Bar", &esp_config.health_bar_enabled);
        ImGui::Checkbox("Name ESP", &esp_config.name_enabled);
        ImGui::Checkbox("Distance", &esp_config.distance_enabled);
        ImGui::Checkbox("Snaplines", &esp_config.snapline_enabled);
        
        ImGui::SliderFloat("Max Distance", &esp_config.max_distance, 50.0f, 1000.0f);
        ImGui::Checkbox("Show Team", &esp_config.show_team);
        ImGui::Checkbox("Visible Only", &esp_config.visible_only);
    }
    
    // Aimbot Settings
    if (ImGui::CollapsingHeader("Aimbot Settings")) {
        ImGui::Checkbox("Enable Aimbot", &state.show_aimbot);
        
        auto& aim_config = aimbot.GetConfig();
        aim_config.enabled = state.show_aimbot;
        
        ImGui::Checkbox("FOV Circle", &aim_config.draw_fov);
        ImGui::SliderFloat("FOV Size", &aim_config.fov_size, 10.0f, 500.0f);
        
        ImGui::Checkbox("Smooth Aim", &aim_config.smooth_enabled);
        ImGui::SliderFloat("Smooth Value", &aim_config.smooth_value, 1.0f, 20.0f);
        
        const char* target_modes[] = { "Closest to Crosshair", "Closest Distance", "Lowest Health" };
        ImGui::Combo("Target Mode", (int*)&aim_config.target_mode, target_modes, 3);
        
        const char* target_bones[] = { "Head", "Neck", "Chest", "Body Center" };
        ImGui::Combo("Target Bone", (int*)&aim_config.target_bone, target_bones, 4);
        
        ImGui::Checkbox("Visible Only", &aim_config.visible_only);
        ImGui::SliderFloat("Max Distance##Aim", &aim_config.max_distance, 50.0f, 500.0f);
    }
    
    // Radar Settings
    if (ImGui::CollapsingHeader("Radar Settings")) {
        ImGui::Checkbox("Enable Radar", &state.show_radar);
        
        auto& radar_config = radar.GetConfig();
        radar_config.enabled = state.show_radar;
        
        ImGui::SliderFloat("Radar Size", &radar_config.size, 100.0f, 500.0f);
        ImGui::SliderFloat("Zoom", &radar_config.zoom, 0.5f, 3.0f);
        ImGui::SliderFloat("Max Range", &radar_config.max_range, 50.0f, 500.0f);
        
        ImGui::Checkbox("Show Grid", &radar_config.show_grid);
        ImGui::Checkbox("Show Names##Radar", &radar_config.show_names);
        ImGui::Checkbox("Show Distance##Radar", &radar_config.show_distance);
        ImGui::Checkbox("Show Team##Radar", &radar_config.show_team);
    }
    
    ImGui::Separator();
    ImGui::Text("Press INSERT to toggle menu");
    ImGui::Text("Press END to exit");
    
    ImGui::End();
}

int main(int argc, char** argv) {

    SetConsoleTitleA("");

    std::cout << "==================================" << std::endl;
    std::cout << "  BF6 KILLSTARS v1.0" << std::endl;
    std::cout << "  READ-ONLY | UNDETECTED" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << std::endl;

    if (!mem::Connected_Driver()) {
        std::cerr << "[!] Failed to initialize Driver" << std::endl;
        system("pause");
        return 1;
    }

    // Attach to game
    std::cout << "[*] Attaching to bf6.exe..." << std::endl;
    if (!mem::find_process(L"bf6.exe")) {
        std::cerr << "[!] Failed to attach to game process" << std::endl;
        std::cerr << "[!] Make sure Battlefield 6 is running" << std::endl;
        system("pause");
        return 1;
    }

    std::cout << "[+] Successfully attached to game!" << std::endl;

    // Initialize game data manager
    auto game_data = std::make_shared<BF6::GameData>();
    if (!game_data->IsValid()) {
        std::cerr << "[!] Failed to initialize game data manager" << std::endl;
        system("pause");
        return 1;
    }

    // Initialize features
    BF6::ESPRenderer esp(game_data);
    BF6::Aimbot aimbot(game_data);
    BF6::RadarRenderer radar(game_data);

    std::cout << "[+] All features initialized!" << std::endl;
    std::cout << "[*] Creating overlay window..." << std::endl;

    // Create overlay window
    g_wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"BF6_DMA", NULL };
    RegisterClassEx(&g_wc);
    
    // Create transparent overlay window
    g_hWnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
        g_wc.lpszClassName,
        L"BF6 DMA Overlay",
        WS_POPUP,
        0, 0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, g_wc.hInstance, NULL
    );

    // Set window transparency
    SetLayeredWindowAttributes(g_hWnd, RGB(0, 0, 0), 255, LWA_ALPHA);

    // Initialize Direct3D
    if (!CreateDeviceD3D(g_hWnd)) {
        CleanupDeviceD3D();
        UnregisterClass(g_wc.lpszClassName, g_wc.hInstance);
        return 1;
    }

    ShowWindow(g_hWnd, SW_SHOWDEFAULT);
    UpdateWindow(g_hWnd);

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(g_hWnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    std::cout << "[+] Overlay created successfully!" << std::endl;
    std::cout << "[*] Press INSERT to toggle menu" << std::endl;
    std::cout << "[*] Press END to exit" << std::endl;
    std::cout << std::endl;

    // Application state
    AppState state;
    bool running = true;

    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    
    while (running && msg.message != WM_QUIT) {
        // Handle messages
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        // Toggle menu with INSERT key
        if (GetAsyncKeyState(VK_INSERT) & 1) {
            state.show_menu = !state.show_menu;
        }

        // Exit with END key
        if (GetAsyncKeyState(VK_END) & 1) {
            running = false;
            break;
        }

        // Update game data
        game_data->Update();

        // Update aimbot
        if (state.show_aimbot) {
            aimbot.Update();
        }

        // Start ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Render features
        if (state.show_esp) {
            esp.Render();
        }

        if (state.show_radar) {
            radar.Render();
        }

        if (state.show_aimbot) {
            aimbot.RenderFOV();
        }

        // Render menu
        RenderMenu(state, esp, aimbot, radar);

        // Render ImGui
        ImGui::Render();
        
        const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);
    }

    // Cleanup
    std::cout << "[*] Shutting down..." << std::endl;
    
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(g_hWnd);
    UnregisterClass(g_wc.lpszClassName, g_wc.hInstance);

    std::cout << "[+] Goodbye!" << std::endl;
    return 0;
}

// DirectX helper functions
bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, 
                                      featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, 
                                      &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

