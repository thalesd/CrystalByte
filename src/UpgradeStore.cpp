#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "UpgradeStore.h"

#include <string>

enum {
    ID_BTN_BUY_MULT   = 301,
    ID_BTN_BUY_UNDO   = 302,
    ID_BTN_NEWGAME    = 300,
    ID_LBL_LAST_SCORE = 400,
    ID_LBL_TOTAL      = 401,
};

struct StoreState {
    UpgradeState* upgrades;
    int           lastCommitScore;
    StoreAction   action;
    HWND          btnBuyMult;
    HWND          btnBuyUndo;
    HWND          lblLastScore;
    HWND          lblTotal;
};

static void refreshStoreLabels(HWND hwnd, StoreState* state) {
    std::string ls = "Last Commit Score: " + std::to_string(state->lastCommitScore);
    SetWindowTextA(state->lblLastScore, ls.c_str());

    std::string ts = "Total Score: " + std::to_string(state->upgrades->totalScore);
    SetWindowTextA(state->lblTotal, ts.c_str());

    // Enable/disable buy buttons
    bool canBuyMult = (state->upgrades->multiplierLevel < 3) &&
                      (state->upgrades->totalScore >= 15);
    EnableWindow(state->btnBuyMult, canBuyMult ? TRUE : FALSE);

    bool canBuyUndo = (state->upgrades->totalScore >= 10);
    EnableWindow(state->btnBuyUndo, canBuyUndo ? TRUE : FALSE);

    // Update multiplier button text
    int lv = state->upgrades->multiplierLevel;
    std::string multLabel;
    if (lv >= 3)
        multLabel = "Score Multiplier (Lv 3) — Maxed";
    else
        multLabel = "Score Multiplier (Lv " + std::to_string(lv) + ") — Buy for 15 pts";
    SetWindowTextA(state->btnBuyMult, multLabel.c_str());
}

static LRESULT CALLBACK StoreWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    StoreState* state = reinterpret_cast<StoreState*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_CREATE: {
            auto* cs = reinterpret_cast<CREATESTRUCTA*>(lParam);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            return 0;
        }
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (!state) return 0;

            if (id == ID_BTN_BUY_MULT) {
                if (state->upgrades->multiplierLevel < 3 &&
                    state->upgrades->totalScore >= 15) {
                    state->upgrades->totalScore -= 15;
                    state->upgrades->multiplierLevel++;
                    refreshStoreLabels(hwnd, state);
                }
            } else if (id == ID_BTN_BUY_UNDO) {
                if (state->upgrades->totalScore >= 10) {
                    state->upgrades->totalScore -= 10;
                    state->upgrades->undosRemaining += 5;
                    refreshStoreLabels(hwnd, state);
                }
            } else if (id == IDOK) {
                state->action = StoreAction::CONTINUE;
                DestroyWindow(hwnd);
            } else if (id == ID_BTN_NEWGAME) {
                state->action = StoreAction::NEW_GAME;
                DestroyWindow(hwnd);
            } else if (id == IDCANCEL) {
                state->action = StoreAction::QUIT;
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

StoreAction showUpgradeStore(UpgradeState& upgrades, int lastCommitScore) {
    StoreState state{};
    state.upgrades        = &upgrades;
    state.lastCommitScore = lastCommitScore;
    state.action          = StoreAction::CONTINUE;

    HINSTANCE hInst = GetModuleHandleA(nullptr);

    WNDCLASSEXA wc   = {};
    wc.cbSize        = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc   = StoreWndProc;
    wc.hInstance     = hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = "CrystalByteUpgradeStore";
    wc.hCursor       = LoadCursorA(nullptr, IDC_ARROW);
    wc.hIcon         = LoadIconA(nullptr, IDI_APPLICATION);
    RegisterClassExA(&wc);

    const int cW = 340, cH = 260;
    RECT rc = { 0, 0, cW, cH };
    AdjustWindowRectEx(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE, WS_EX_DLGMODALFRAME);
    int wW = rc.right  - rc.left;
    int wH = rc.bottom - rc.top;

    int wx = (GetSystemMetrics(SM_CXSCREEN) - wW) / 2;
    int wy = (GetSystemMetrics(SM_CYSCREEN) - wH) / 2;

    HWND hwnd = CreateWindowExA(
        WS_EX_DLGMODALFRAME,
        "CrystalByteUpgradeStore",
        "Upgrade Store",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        wx, wy, wW, wH,
        nullptr, nullptr, hInst, &state
    );

    if (!hwnd) {
        UnregisterClassA("CrystalByteUpgradeStore", hInst);
        return StoreAction::CONTINUE;
    }

    HINSTANCE h = hInst;

    // Labels
    state.lblLastScore = CreateWindowExA(0, "STATIC", "",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 20, 300, 20, hwnd, (HMENU)(INT_PTR)ID_LBL_LAST_SCORE, h, nullptr);

    state.lblTotal = CreateWindowExA(0, "STATIC", "",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 50, 300, 20, hwnd, (HMENU)(INT_PTR)ID_LBL_TOTAL, h, nullptr);

    // Buy multiplier button
    state.btnBuyMult = CreateWindowExA(0, "BUTTON", "",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        20, 90, 300, 28, hwnd, (HMENU)(INT_PTR)ID_BTN_BUY_MULT, h, nullptr);

    // Buy undo pack button
    state.btnBuyUndo = CreateWindowExA(0, "BUTTON", "Undo Pack (5 undos) \xe2\x80\x94 Buy for 10 pts",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        20, 130, 300, 28, hwnd, (HMENU)(INT_PTR)ID_BTN_BUY_UNDO, h, nullptr);

    // Action buttons
    CreateWindowExA(0, "BUTTON", "Continue Playing",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
        20, 210, 90, 30, hwnd, (HMENU)IDOK, h, nullptr);

    CreateWindowExA(0, "BUTTON", "New Game",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        120, 210, 90, 30, hwnd, (HMENU)(INT_PTR)ID_BTN_NEWGAME, h, nullptr);

    CreateWindowExA(0, "BUTTON", "Quit",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        220, 210, 90, 30, hwnd, (HMENU)(INT_PTR)IDCANCEL, h, nullptr);

    // Initialize label text and button states
    refreshStoreLabels(hwnd, &state);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessage(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    UnregisterClassA("CrystalByteUpgradeStore", hInst);
    return state.action;
}
