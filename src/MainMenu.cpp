#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "MainMenu.h"

#include <array>
#include <string>

static const std::array<std::pair<int,int>, 5> kMenuResolutions = {{
    { 800,  600  },
    { 1280, 720  },
    { 1920, 1080 },
    { 2560, 1440 },
    { 3840, 2160 },
}};

static const std::array<int, 6> kMenuFramerates = { 30, 60, 120, 144, 240, 0 };

static const char* menuFramerateStr(int fps) {
    switch (fps) {
        case 30:  return "30 FPS";
        case 60:  return "60 FPS";
        case 120: return "120 FPS";
        case 144: return "144 FPS";
        case 240: return "240 FPS";
        default:  return "Unlimited";
    }
}

enum {
    ID_MENU_COMBO_RES = 100,
    ID_MENU_COMBO_FPS = 101,
    ID_BTN_RUBIKS     = 200,
    ID_BTN_SHOOTER    = 201,
};

struct MainMenuState {
    MainMenuResult* result;
    HWND            comboRes;
    HWND            comboFps;
};

static LRESULT CALLBACK MainMenuWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MainMenuState* state = reinterpret_cast<MainMenuState*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_CREATE: {
            auto* cs = reinterpret_cast<CREATESTRUCTA*>(lParam);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            return 0;
        }
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if ((id == ID_BTN_RUBIKS || id == ID_BTN_SHOOTER || id == IDCANCEL) && state) {
                int ri = (int)SendMessage(state->comboRes, CB_GETCURSEL, 0, 0);
                int fi = (int)SendMessage(state->comboFps, CB_GETCURSEL, 0, 0);
                if (ri >= 0 && ri < (int)kMenuResolutions.size()) {
                    state->result->width  = kMenuResolutions[ri].first;
                    state->result->height = kMenuResolutions[ri].second;
                }
                if (fi >= 0 && fi < (int)kMenuFramerates.size())
                    state->result->targetFPS = kMenuFramerates[fi];

                if (id == ID_BTN_RUBIKS) {
                    state->result->choice   = MainMenuResult::RUBIKS;
                    state->result->accepted = true;
                } else if (id == ID_BTN_SHOOTER) {
                    state->result->choice   = MainMenuResult::SHOOTER;
                    state->result->accepted = true;
                } else {
                    // IDCANCEL / Quit
                    state->result->choice   = MainMenuResult::QUIT;
                    state->result->accepted = false;
                }
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

MainMenuResult showMainMenu() {
    MainMenuResult result;
    MainMenuState  state{ &result, nullptr, nullptr };

    HINSTANCE hInst = GetModuleHandleA(nullptr);

    WNDCLASSEXA wc   = {};
    wc.cbSize        = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc   = MainMenuWndProc;
    wc.hInstance     = hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = "CrystalByteMainMenu";
    wc.hCursor       = LoadCursorA(nullptr, IDC_ARROW);
    wc.hIcon         = LoadIconA(nullptr, IDI_APPLICATION);
    RegisterClassExA(&wc);

    const int cW = 310, cH = 200;
    RECT rc = { 0, 0, cW, cH };
    AdjustWindowRectEx(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE, WS_EX_DLGMODALFRAME);
    int wW = rc.right  - rc.left;
    int wH = rc.bottom - rc.top;

    int wx = (GetSystemMetrics(SM_CXSCREEN) - wW) / 2;
    int wy = (GetSystemMetrics(SM_CYSCREEN) - wH) / 2;

    HWND hwnd = CreateWindowExA(
        WS_EX_DLGMODALFRAME,
        "CrystalByteMainMenu",
        "CrystalByte \xe2\x80\x94 Main Menu",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        wx, wy, wW, wH,
        nullptr, nullptr, hInst, &state
    );

    if (!hwnd) { UnregisterClassA("CrystalByteMainMenu", hInst); return result; }

    HINSTANCE h = hInst;

    // Resolution row at y=20
    CreateWindowExA(0, "STATIC", "Resolution:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 23, 90, 20, hwnd, nullptr, h, nullptr);
    state.comboRes = CreateWindowExA(0, "COMBOBOX", nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
        120, 20, 170, 150, hwnd, (HMENU)(INT_PTR)ID_MENU_COMBO_RES, h, nullptr);
    for (auto& [w, ht] : kMenuResolutions) {
        std::string lbl = std::to_string(w) + " x " + std::to_string(ht);
        SendMessageA(state.comboRes, CB_ADDSTRING, 0, (LPARAM)lbl.c_str());
    }
    SendMessageA(state.comboRes, CB_SETCURSEL, 1, 0); // default: 1280x720

    // Frame Rate row at y=60
    CreateWindowExA(0, "STATIC", "Frame Rate:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 63, 90, 20, hwnd, nullptr, h, nullptr);
    state.comboFps = CreateWindowExA(0, "COMBOBOX", nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
        120, 60, 170, 170, hwnd, (HMENU)(INT_PTR)ID_MENU_COMBO_FPS, h, nullptr);
    for (int fps : kMenuFramerates)
        SendMessageA(state.comboFps, CB_ADDSTRING, 0, (LPARAM)menuFramerateStr(fps));
    SendMessageA(state.comboFps, CB_SETCURSEL, 1, 0); // default: 60 FPS

    // Three buttons at y=150
    // "Rubik's Cube" x=10 w=90
    CreateWindowExA(0, "BUTTON", "Rubik's Cube",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
        10, 150, 90, 30, hwnd, (HMENU)(INT_PTR)ID_BTN_RUBIKS, h, nullptr);
    // "Space Shooter" x=110 w=100
    CreateWindowExA(0, "BUTTON", "Space Shooter",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        110, 150, 100, 30, hwnd, (HMENU)(INT_PTR)ID_BTN_SHOOTER, h, nullptr);
    // "Quit" x=220 w=70
    CreateWindowExA(0, "BUTTON", "Quit",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        220, 150, 70, 30, hwnd, (HMENU)(INT_PTR)IDCANCEL, h, nullptr);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessage(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    UnregisterClassA("CrystalByteMainMenu", hInst);
    return result;
}
