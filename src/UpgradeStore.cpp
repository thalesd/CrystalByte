#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "UpgradeStore.h"

#include <algorithm>
#include <random>
#include <string>

// ---------------------------------------------------------------------------
// generateUpgradeOptions
// ---------------------------------------------------------------------------

static const char* kColorNames[6] = {
    "White", "Yellow", "Red", "Orange", "Blue", "Green"
};

std::vector<UpgradeOption> generateUpgradeOptions(const UpgradeState& state) {
    std::mt19937 rng(std::random_device{}());

    std::vector<UpgradeOption> pool;

    // Score multiplier (if not maxed at level 3)
    if (state.multiplierLevel < 3) {
        UpgradeOption o;
        o.type = UpgradeType::SCORE_MULTIPLIER;
        o.cost = 15;
        float next = 1.0f + 0.5f * float(state.multiplierLevel + 1);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "Score Multiplier x%.1f (next commit) -- 15 pts", next);
        o.label = buf;
        pool.push_back(o);
    }

    // Undo pack
    {
        UpgradeOption o;
        o.type  = UpgradeType::UNDO_PACK;
        o.cost  = 10;
        o.label = "+5 Undo Moves -- 10 pts";
        pool.push_back(o);
    }

    // Color bonus -- one option per color that doesn't already have the bonus
    for (int i = 0; i < 6; ++i) {
        if (!state.colorBonus[i]) {
            UpgradeOption o;
            o.type     = UpgradeType::COLOR_BONUS;
            o.cost     = 20;
            o.colorIdx = i;
            std::string name = kColorNames[i];
            o.label = name + " stickers score 2x -- 20 pts";
            pool.push_back(o);
        }
    }

    // Fast moves (one-time)
    if (!state.fastMoves) {
        UpgradeOption o;
        o.type  = UpgradeType::FAST_MOVES;
        o.cost  = 8;
        o.label = "Speed Up Moves (2x faster) -- 8 pts";
        pool.push_back(o);
    }

    // Instant points -- always available as a freebie
    {
        UpgradeOption o;
        o.type  = UpgradeType::INSTANT_POINTS;
        o.cost  = 0;
        o.label = "Bonus: +15 pts -- Free";
        pool.push_back(o);
    }

    std::shuffle(pool.begin(), pool.end(), rng);

    std::vector<UpgradeOption> result;
    int take = std::min(3, (int)pool.size());
    for (int i = 0; i < take; ++i)
        result.push_back(pool[i]);
    return result;
}

// ---------------------------------------------------------------------------
// Win32 store dialog
// ---------------------------------------------------------------------------

enum {
    ID_LBL_LAST    = 400,
    ID_LBL_TOTAL   = 401,
    ID_LBL_NOTE    = 402,
    ID_BTN_OPT0    = 500,
    ID_BTN_OPT1    = 501,
    ID_BTN_OPT2    = 502,
};

struct StoreWinState {
    UpgradeState*                     upgrades;
    int                               lastScore;
    StoreAction                       action;
    const std::vector<UpgradeOption>* options;
    HWND lblLastScore;
    HWND lblTotal;
    HWND btnOpts[3];
    bool bought[3];
};

static void refreshStore(HWND hwnd, StoreWinState* s) {
    (void)hwnd;

    std::string ls = "Last commit: " + std::to_string(s->lastScore) + " pts";
    SetWindowTextA(s->lblLastScore, ls.c_str());

    std::string ts = "Total score: " + std::to_string(s->upgrades->totalScore) + " pts";
    SetWindowTextA(s->lblTotal, ts.c_str());

    for (int i = 0; i < (int)s->options->size() && i < 3; ++i) {
        if (s->bought[i]) {
            EnableWindow(s->btnOpts[i], FALSE);
            continue;
        }
        int cost = (*s->options)[i].cost;
        bool canBuy = (s->upgrades->totalScore >= cost);

        // Specific checks to prevent buying maxed upgrades
        const UpgradeOption& opt = (*s->options)[i];
        if (opt.type == UpgradeType::SCORE_MULTIPLIER && s->upgrades->multiplierLevel >= 3)
            canBuy = false;
        if (opt.type == UpgradeType::COLOR_BONUS && s->upgrades->colorBonus[opt.colorIdx])
            canBuy = false;
        if (opt.type == UpgradeType::FAST_MOVES && s->upgrades->fastMoves)
            canBuy = false;

        EnableWindow(s->btnOpts[i], canBuy ? TRUE : FALSE);
    }
}

static void applyUpgrade(UpgradeState& upg, const UpgradeOption& opt) {
    upg.totalScore -= opt.cost;
    if (upg.totalScore < 0) upg.totalScore = 0;

    switch (opt.type) {
        case UpgradeType::SCORE_MULTIPLIER:
            if (upg.multiplierLevel < 3) upg.multiplierLevel++;
            break;
        case UpgradeType::UNDO_PACK:
            upg.undosRemaining += 5;
            break;
        case UpgradeType::COLOR_BONUS:
            upg.colorBonus[opt.colorIdx] = true;
            break;
        case UpgradeType::FAST_MOVES:
            upg.fastMoves = true;
            break;
        case UpgradeType::INSTANT_POINTS:
            upg.totalScore += 15;   // free bonus, no cost
            break;
    }
}

static LRESULT CALLBACK StoreWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* s = reinterpret_cast<StoreWinState*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_CREATE: {
            auto* cs = reinterpret_cast<CREATESTRUCTA*>(lParam);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            return 0;
        }
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (!s) return 0;

            // Upgrade option buttons
            for (int i = 0; i < (int)s->options->size() && i < 3; ++i) {
                if (id == ID_BTN_OPT0 + i && !s->bought[i]) {
                    const UpgradeOption& opt = (*s->options)[i];
                    if (s->upgrades->totalScore >= opt.cost) {
                        applyUpgrade(*s->upgrades, opt);
                        s->bought[i] = true;
                        refreshStore(hwnd, s);
                    }
                    return 0;
                }
            }

            if (id == IDOK) {
                s->action = StoreAction::CONTINUE;
                DestroyWindow(hwnd);
            } else if (id == IDCANCEL) {
                s->action = StoreAction::QUIT;
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

StoreAction showUpgradeStore(UpgradeState& state, int lastCommitScore,
                             const std::vector<UpgradeOption>& options) {
    StoreWinState ws{};
    ws.upgrades   = &state;
    ws.lastScore  = lastCommitScore;
    ws.action     = StoreAction::CONTINUE;
    ws.options    = &options;

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

    const int cW = 380, cH = 300;
    RECT rc = { 0, 0, cW, cH };
    AdjustWindowRectEx(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE, WS_EX_DLGMODALFRAME);
    int wW = rc.right  - rc.left;
    int wH = rc.bottom - rc.top;
    int wx = (GetSystemMetrics(SM_CXSCREEN) - wW) / 2;
    int wy = (GetSystemMetrics(SM_CYSCREEN) - wH) / 2;

    HWND hwnd = CreateWindowExA(
        WS_EX_DLGMODALFRAME, "CrystalByteUpgradeStore",
        "Upgrade Store",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        wx, wy, wW, wH, nullptr, nullptr, hInst, &ws);

    if (!hwnd) { UnregisterClassA("CrystalByteUpgradeStore", hInst); return StoreAction::CONTINUE; }

    HINSTANCE h = hInst;

    ws.lblLastScore = CreateWindowExA(0, "STATIC", "",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 18, 340, 20, hwnd, (HMENU)(INT_PTR)ID_LBL_LAST, h, nullptr);

    ws.lblTotal = CreateWindowExA(0, "STATIC", "",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 42, 340, 20, hwnd, (HMENU)(INT_PTR)ID_LBL_TOTAL, h, nullptr);

    CreateWindowExA(0, "STATIC", "Choose an upgrade (cube rescrambles after continuing):",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 72, 340, 20, hwnd, nullptr, h, nullptr);

    for (int i = 0; i < (int)options.size() && i < 3; ++i) {
        ws.btnOpts[i] = CreateWindowExA(0, "BUTTON", options[i].label.c_str(),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
            20, 100 + i * 46, 340, 32, hwnd,
            (HMENU)(INT_PTR)(ID_BTN_OPT0 + i), h, nullptr);
    }

    CreateWindowExA(0, "BUTTON", "Continue Playing",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
        50, 248, 130, 30, hwnd, (HMENU)IDOK, h, nullptr);
    CreateWindowExA(0, "BUTTON", "Quit",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        200, 248, 130, 30, hwnd, (HMENU)IDCANCEL, h, nullptr);

    refreshStore(hwnd, &ws);

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
    return ws.action;
}
