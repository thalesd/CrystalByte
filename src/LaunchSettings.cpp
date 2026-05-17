#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "LaunchSettings.h"

#include <array>
#include <string>

static const std::array<std::pair<int, int>, 5> kResolutions = {{
    { 800,  600  },
    { 1280, 720  },
    { 1920, 1080 },
    { 2560, 1440 },
    { 3840, 2160 },
}};

static const std::array<int, 6> kFramerates = { 30, 60, 120, 144, 240, 0 };

static const char* framerateStr(int fps) {
    switch (fps) {
        case 30:  return "30 FPS";
        case 60:  return "60 FPS";
        case 120: return "120 FPS";
        case 144: return "144 FPS";
        case 240: return "240 FPS";
        default:  return "Unlimited";
    }
}

enum { ID_COMBO_RES = 100, ID_COMBO_FPS = 101, ID_CHECK_BVH = 102 };

struct DialogState {
    LaunchConfig* config;
    HWND          comboRes;
    HWND          comboFps;
    HWND          checkBvh;
};

static LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    DialogState* state = reinterpret_cast<DialogState*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_CREATE: {
            auto* cs = reinterpret_cast<CREATESTRUCTA*>(lParam);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            return 0;
        }
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == IDOK && state) {
                int ri = (int)SendMessage(state->comboRes, CB_GETCURSEL, 0, 0);
                int fi = (int)SendMessage(state->comboFps, CB_GETCURSEL, 0, 0);
                if (ri >= 0 && ri < (int)kResolutions.size()) {
                    state->config->width  = kResolutions[ri].first;
                    state->config->height = kResolutions[ri].second;
                }
                if (fi >= 0 && fi < (int)kFramerates.size())
                    state->config->targetFPS = kFramerates[fi];
                state->config->showBvh  = (SendMessage(state->checkBvh, BM_GETCHECK, 0, 0) == BST_CHECKED);
                state->config->accepted = true;
                DestroyWindow(hwnd);
            } else if (id == IDCANCEL) {
                DestroyWindow(hwnd);
            }
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
}

LaunchConfig showLaunchSettings() {
    LaunchConfig config;
    DialogState  state{ &config, nullptr, nullptr };

    HINSTANCE hInst = GetModuleHandleA(nullptr);

    WNDCLASSEXA wc    = {};
    wc.cbSize         = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc    = SettingsWndProc;
    wc.hInstance      = hInst;
    wc.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName  = "CrystalByteSettings";
    wc.hCursor        = LoadCursorA(nullptr, IDC_ARROW);
    wc.hIcon          = LoadIconA(nullptr, IDI_APPLICATION);
    RegisterClassExA(&wc);

    // Calculate window size to get a 310x185 client area
    const int cW = 310, cH = 185;
    RECT rc = { 0, 0, cW, cH };
    AdjustWindowRectEx(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE, WS_EX_DLGMODALFRAME);
    int wW = rc.right  - rc.left;
    int wH = rc.bottom - rc.top;

    // Center on primary monitor
    int wx = (GetSystemMetrics(SM_CXSCREEN) - wW) / 2;
    int wy = (GetSystemMetrics(SM_CYSCREEN) - wH) / 2;

    HWND hwnd = CreateWindowExA(
        WS_EX_DLGMODALFRAME,
        "CrystalByteSettings",
        "CrystalByte \xe2\x80\x94 Launch Settings",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        wx, wy, wW, wH,
        nullptr, nullptr, hInst, &state
    );

    if (!hwnd) { UnregisterClassA("CrystalByteSettings", hInst); return config; }

    HINSTANCE h = hInst;

    // Resolution row
    CreateWindowExA(0, "STATIC", "Resolution:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 23, 90, 20, hwnd, nullptr, h, nullptr);
    state.comboRes = CreateWindowExA(0, "COMBOBOX", nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
        120, 20, 170, 150, hwnd, (HMENU)(INT_PTR)ID_COMBO_RES, h, nullptr);
    for (auto& [w, ht] : kResolutions) {
        std::string lbl = std::to_string(w) + " x " + std::to_string(ht);
        SendMessageA(state.comboRes, CB_ADDSTRING, 0, (LPARAM)lbl.c_str());
    }
    SendMessageA(state.comboRes, CB_SETCURSEL, 1, 0);  // default: 1280 x 720

    // Frame rate row
    CreateWindowExA(0, "STATIC", "Frame Rate:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 63, 90, 20, hwnd, nullptr, h, nullptr);
    state.comboFps = CreateWindowExA(0, "COMBOBOX", nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
        120, 60, 170, 170, hwnd, (HMENU)(INT_PTR)ID_COMBO_FPS, h, nullptr);
    for (int fps : kFramerates)
        SendMessageA(state.comboFps, CB_ADDSTRING, 0, (LPARAM)framerateStr(fps));
    SendMessageA(state.comboFps, CB_SETCURSEL, 1, 0);  // default: 60 FPS

    // BVH checkbox
    state.checkBvh = CreateWindowExA(0, "BUTTON", "Show BVH wireframes",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
        20, 100, 200, 20, hwnd, (HMENU)(INT_PTR)ID_CHECK_BVH, h, nullptr);

    // Buttons
    CreateWindowExA(0, "BUTTON", "Launch",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
        55, 140, 90, 30, hwnd, (HMENU)IDOK, h, nullptr);
    CreateWindowExA(0, "BUTTON", "Cancel",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        165, 140, 90, 30, hwnd, (HMENU)IDCANCEL, h, nullptr);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // Message loop — IsDialogMessage handles Tab/Enter keyboard navigation
    MSG msg;
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessage(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    UnregisterClassA("CrystalByteSettings", hInst);
    return config;
}
