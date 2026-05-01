#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <string>
#include <sstream>
#include <commdlg.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "winmm.lib")

// ---------------------------------------------------------------
// Control IDs
// ---------------------------------------------------------------
#define IDT_MOUSE_MOVE        1

#define IDC_LEFT_SLIDER      101
#define IDC_RIGHT_SLIDER     102
#define IDC_UP_SLIDER        103
#define IDC_DOWN_SLIDER      104
#define IDC_SENS_SLIDER      105

#define IDC_SETKEY_BTN       106
#define IDC_KEY_DISPLAY      107
#define IDC_STATUS           108
#define IDC_SAVE_BTN         109
#define IDC_LOAD_BTN         110
#define IDC_CREATE_BTN       111
#define IDC_ACTIVATION_MODE  112

#define IDC_DELAY_ENABLE     120
#define IDC_DELAY_SLIDER     121
#define IDC_DELAY_LABEL      122   // "Delay: Xms" text label

#define IDC_LEFT2_SLIDER     131
#define IDC_RIGHT2_SLIDER    132
#define IDC_UP2_SLIDER       133
#define IDC_DOWN2_SLIDER     134

// Live value labels for each slider (shown to the right)
#define IDC_VAL_LEFT         140
#define IDC_VAL_RIGHT        141
#define IDC_VAL_UP           142
#define IDC_VAL_DOWN         143
#define IDC_SENS_LABEL       144
#define IDC_VAL_LEFT2        145
#define IDC_VAL_RIGHT2       146
#define IDC_VAL_UP2          147
#define IDC_VAL_DOWN2        148
#define IDC_VAL_DELAY        149   // delay ms value next to the delay slider

// Custom messages
#define WM_APP_UPDATE_KEY    (WM_APP + 1)
#define WM_APP_UPDATE_STATUS (WM_APP + 2)

// ---------------------------------------------------------------
// Globals
// ---------------------------------------------------------------
HINSTANCE hInst;
HWND hMainWnd = nullptr;

HWND hTrackLeft, hTrackRight, hTrackUp, hTrackDown, hTrackSens;
HWND hTrackLeft2, hTrackRight2, hTrackUp2, hTrackDown2;
HWND hDelayCheck, hTrackDelay, hDelayLabel;
HWND hStatus, hKeyDisplay, hActivationCombo, hSetKeyBtn;

bool  active = false;
bool  capturingKey = false;

int   leftVal = 0, rightVal = 0, upVal = 0, downVal = 0;
int   leftVal2 = 0, rightVal2 = 0, upVal2 = 0, downVal2 = 0;
int   sensVal = 10;
UINT  toggleKey = VK_F2;
int   activationMode = 0;

bool  delayEnabled = false;
int   delayMs = 500;
bool  usingSecondary = false;
DWORD activationTime = 0;

float accumX = 0.0f, accumY = 0.0f;

HHOOK hhkKeyboard = nullptr;
HHOOK hhkMouse = nullptr;

// ---------------------------------------------------------------
// Fine‑tuning constant: 1 slider unit = STEP_SCALE pixels per tick
// ---------------------------------------------------------------
const float STEP_SCALE = 0.5f;

// ---------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK LowLevelKeyboardProc(int, WPARAM, LPARAM);
LRESULT CALLBACK LowLevelMouseProc(int, WPARAM, LPARAM);
void UpdateStatusDisplay();
void UpdateKeyDisplay();
void UpdateAllValueLabels(HWND hwnd);
void UpdateSliderLabel(HWND hwnd, int staticId, int value, bool isDelay = false);
void UpdateDelayLabel();
void UpdateDelayControlsEnabled();
void SaveConfig(HWND);
bool LoadConfig(HWND);
void ResetConfig();
void ApplyLoadedConfig();

static int  SliderVal(HWND h) { return (int)SendMessage(h, TBM_GETPOS, 0, 0); }
static void SetSlider(HWND h, int v) { SendMessage(h, TBM_SETPOS, TRUE, v); }

// ---------------------------------------------------------------
// WinMain
// ---------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    hInst = hInstance;
    timeBeginPeriod(1);

    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_BAR_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icex);

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"RecoilControlClass";
    if (!RegisterClassEx(&wc)) {
        MessageBox(nullptr, L"Window Registration Failed!", L"Error", MB_ICONERROR);
        return 0;
    }

    // Extra width for value labels on the right
    hMainWnd = CreateWindowEx(0, L"RecoilControlClass", L"Recoil Control",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 540, 660,
        nullptr, nullptr, hInstance, nullptr);
    if (!hMainWnd) {
        MessageBox(nullptr, L"Window Creation Failed!", L"Error", MB_ICONERROR);
        return 0;
    }

    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);

    hhkKeyboard = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);
    if (!hhkKeyboard)
        MessageBox(hMainWnd, L"Failed to install keyboard hook!", L"Error", MB_ICONERROR);

    hhkMouse = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, hInstance, 0);
    if (!hhkMouse)
        MessageBox(hMainWnd, L"Failed to install mouse hook!", L"Error", MB_ICONERROR);

    SetTimer(hMainWnd, IDT_MOUSE_MOVE, 1, nullptr);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    timeEndPeriod(1);
    if (hhkKeyboard) UnhookWindowsHookEx(hhkKeyboard);
    if (hhkMouse)    UnhookWindowsHookEx(hhkMouse);
    return (int)msg.wParam;
}

// ---------------------------------------------------------------
// Helpers to create labelled sliders
// col1=label x, col2=slider x, sw=slider width
// Value label is placed at col2+sw+6, width 46, with thin edge and centred text
// ---------------------------------------------------------------
static HWND MakeSliderRow(HWND parent,
    const wchar_t* labelText,
    int col1, int col2, int sw, int row,
    int sliderId, int valLabelId,
    int lo, int hi, int initVal)
{
    // Left text label
    CreateWindowEx(0, L"STATIC", labelText, WS_CHILD | WS_VISIBLE,
        col1, row + 5, col2 - col1 - 4, 18, parent, nullptr, hInst, nullptr);

    // Trackbar
    HWND h = CreateWindowEx(0, TRACKBAR_CLASS, L"",
        WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ,
        col2, row, sw, 28, parent, (HMENU)(INT_PTR)sliderId, hInst, nullptr);
    SendMessage(h, TBM_SETRANGE, TRUE, MAKELONG(lo, hi));
    SendMessage(h, TBM_SETPOS, TRUE, initVal);

    // Live value label (right of slider) – thin border, centred text
    wchar_t buf[16]; swprintf_s(buf, L"%d", initVal);
    CreateWindowEx(WS_EX_STATICEDGE, L"STATIC", buf,
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
        col2 + sw + 6, row + 4, 46, 20,
        parent, (HMENU)(INT_PTR)valLabelId, hInst, nullptr);

    return h;
}

// ---------------------------------------------------------------
// Update one value label from a raw int
// isDelay=true  → shows the raw ms value
// isDelay=false → shows the raw int (slider units)
// ---------------------------------------------------------------
void UpdateSliderLabel(HWND hwnd, int staticId, int value, bool /*isDelay*/)
{
    wchar_t buf[16];
    swprintf_s(buf, L"%d", value);
    SetDlgItemText(hwnd, staticId, buf);
}

void UpdateAllValueLabels(HWND hwnd)
{
    UpdateSliderLabel(hwnd, IDC_VAL_LEFT, leftVal, false);
    UpdateSliderLabel(hwnd, IDC_VAL_RIGHT, rightVal, false);
    UpdateSliderLabel(hwnd, IDC_VAL_UP, upVal, false);
    UpdateSliderLabel(hwnd, IDC_VAL_DOWN, downVal, false);
    // Sens label shows the float representation
    {
        wchar_t buf[16]; swprintf_s(buf, L"%.2f", sensVal / 10.0f);
        SetDlgItemText(hwnd, IDC_SENS_LABEL, buf);
    }
    UpdateSliderLabel(hwnd, IDC_VAL_LEFT2, leftVal2, false);
    UpdateSliderLabel(hwnd, IDC_VAL_RIGHT2, rightVal2, false);
    UpdateSliderLabel(hwnd, IDC_VAL_UP2, upVal2, false);
    UpdateSliderLabel(hwnd, IDC_VAL_DOWN2, downVal2, false);
    UpdateSliderLabel(hwnd, IDC_VAL_DELAY, delayMs, true);
}

// ---------------------------------------------------------------
// WndProc
// ---------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        // ============================================================
    case WM_CREATE:
    {
        LoadConfig(hwnd);   // silent cancel OK

        int col1 = 16;
        int col2 = 126;
        int sw = 230;
        int row = 16;

        // ---- Primary Movement ----
        CreateWindowEx(0, L"STATIC", L"── Primary Movement ──",
            WS_CHILD | WS_VISIBLE, col1, row, 240, 18, hwnd, nullptr, hInst, nullptr);
        row += 24;

        hTrackLeft = MakeSliderRow(hwnd, L"Left:", col1, col2, sw, row, IDC_LEFT_SLIDER, IDC_VAL_LEFT, 0, 100, leftVal);  row += 34;
        hTrackRight = MakeSliderRow(hwnd, L"Right:", col1, col2, sw, row, IDC_RIGHT_SLIDER, IDC_VAL_RIGHT, 0, 100, rightVal); row += 34;
        hTrackUp = MakeSliderRow(hwnd, L"Up:", col1, col2, sw, row, IDC_UP_SLIDER, IDC_VAL_UP, 0, 100, upVal);    row += 34;
        hTrackDown = MakeSliderRow(hwnd, L"Down:", col1, col2, sw, row, IDC_DOWN_SLIDER, IDC_VAL_DOWN, 0, 100, downVal);  row += 34;

        // Sensitivity (label shows float)
        CreateWindowEx(0, L"STATIC", L"In-game Sens:",
            WS_CHILD | WS_VISIBLE, col1, row + 5, col2 - col1 - 4, 18, hwnd, nullptr, hInst, nullptr);
        hTrackSens = CreateWindowEx(0, TRACKBAR_CLASS, L"",
            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ,
            col2, row, sw, 28, hwnd, (HMENU)IDC_SENS_SLIDER, hInst, nullptr);
        SendMessage(hTrackSens, TBM_SETRANGE, TRUE, MAKELONG(1, 100));
        SendMessage(hTrackSens, TBM_SETPOS, TRUE, sensVal);
        {
            wchar_t buf[16]; swprintf_s(buf, L"%.2f", sensVal / 10.0f);
            CreateWindowEx(WS_EX_STATICEDGE, L"STATIC", buf,
                WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
                col2 + sw + 6, row + 4, 46, 20,
                hwnd, (HMENU)IDC_SENS_LABEL, hInst, nullptr);
        }
        row += 38;

        // ---- Activation mode ----
        CreateWindowEx(0, L"STATIC", L"Activation:",
            WS_CHILD | WS_VISIBLE, col1, row + 3, 100, 18, hwnd, nullptr, hInst, nullptr);
        hActivationCombo = CreateWindowEx(0, WC_COMBOBOX, L"",
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS,
            col2, row, 160, 200, hwnd, (HMENU)IDC_ACTIVATION_MODE, hInst, nullptr);
        SendMessage(hActivationCombo, CB_ADDSTRING, 0, (LPARAM)L"Toggle Key");
        SendMessage(hActivationCombo, CB_ADDSTRING, 0, (LPARAM)L"Hold Mouse 1");
        SendMessage(hActivationCombo, CB_ADDSTRING, 0, (LPARAM)L"Hold Mouse 1+2");
        SendMessage(hActivationCombo, CB_SETCURSEL, activationMode, 0);
        row += 32;

        // ---- Toggle key ----
        hSetKeyBtn = CreateWindowEx(0, L"BUTTON", L"Set Toggle Key",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            col1, row, 120, 26, hwnd, (HMENU)IDC_SETKEY_BTN, hInst, nullptr);
        hKeyDisplay = CreateWindowEx(0, L"STATIC", L"",
            WS_CHILD | WS_VISIBLE,
            col2, row + 4, 240, 18, hwnd, (HMENU)IDC_KEY_DISPLAY, hInst, nullptr);
        row += 34;

        // ---- Status ----
        hStatus = CreateWindowEx(0, L"STATIC", L"",
            WS_CHILD | WS_VISIBLE,
            col1, row, 360, 18, hwnd, (HMENU)IDC_STATUS, hInst, nullptr);
        row += 28;

        // ---- Separator ----
        CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
            col1, row, 470, 2, hwnd, nullptr, hInst, nullptr);
        row += 12;

        // ---- Delayed secondary settings ----
        hDelayCheck = CreateWindowEx(0, L"BUTTON",
            L"Enable Delayed Secondary Settings (optional)",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            col1, row, 340, 20, hwnd, (HMENU)IDC_DELAY_ENABLE, hInst, nullptr);
        SendMessage(hDelayCheck, BM_SETCHECK, delayEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
        row += 28;

        // Delay slider — label on left shows "Delay:", value label on right shows ms
        CreateWindowEx(0, L"STATIC", L"Delay:",
            WS_CHILD | WS_VISIBLE, col1, row + 5, col2 - col1 - 4, 18, hwnd, nullptr, hInst, nullptr);
        hTrackDelay = CreateWindowEx(0, TRACKBAR_CLASS, L"",
            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ,
            col2, row, sw, 28, hwnd, (HMENU)IDC_DELAY_SLIDER, hInst, nullptr);
        SendMessage(hTrackDelay, TBM_SETRANGE, TRUE, MAKELONG(100, 5000));
        SendMessage(hTrackDelay, TBM_SETPOS, TRUE, delayMs);
        {
            wchar_t buf[16]; swprintf_s(buf, L"%d", delayMs);
            CreateWindowEx(WS_EX_STATICEDGE, L"STATIC", buf,
                WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
                col2 + sw + 6, row + 4, 46, 20,
                hwnd, (HMENU)IDC_VAL_DELAY, hInst, nullptr);
        }
        // Small "ms" suffix after the value box
        CreateWindowEx(0, L"STATIC", L"ms",
            WS_CHILD | WS_VISIBLE,
            col2 + sw + 56, row + 5, 24, 18, hwnd, nullptr, hInst, nullptr);
        row += 34;

        // Explanatory note
        CreateWindowEx(0, L"STATIC",
            L"After the delay elapses, the secondary values below replace the primary ones.",
            WS_CHILD | WS_VISIBLE | SS_WORDELLIPSIS,
            col1, row, 460, 16, hwnd, nullptr, hInst, nullptr);
        row += 22;

        CreateWindowEx(0, L"STATIC", L"── Secondary Movement (post-delay) ──",
            WS_CHILD | WS_VISIBLE, col1, row, 300, 18, hwnd, nullptr, hInst, nullptr);
        row += 22;

        hTrackLeft2 = MakeSliderRow(hwnd, L"Left (2nd):", col1, col2, sw, row, IDC_LEFT2_SLIDER, IDC_VAL_LEFT2, 0, 100, leftVal2);  row += 34;
        hTrackRight2 = MakeSliderRow(hwnd, L"Right (2nd):", col1, col2, sw, row, IDC_RIGHT2_SLIDER, IDC_VAL_RIGHT2, 0, 100, rightVal2); row += 34;
        hTrackUp2 = MakeSliderRow(hwnd, L"Up (2nd):", col1, col2, sw, row, IDC_UP2_SLIDER, IDC_VAL_UP2, 0, 100, upVal2);    row += 34;
        hTrackDown2 = MakeSliderRow(hwnd, L"Down (2nd):", col1, col2, sw, row, IDC_DOWN2_SLIDER, IDC_VAL_DOWN2, 0, 100, downVal2);  row += 42;

        // ---- Config buttons ----
        CreateWindowEx(0, L"BUTTON", L"Save Config", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            col1, row, 110, 26, hwnd, (HMENU)IDC_SAVE_BTN, hInst, nullptr);
        CreateWindowEx(0, L"BUTTON", L"Load Config", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            col1 + 118, row, 110, 26, hwnd, (HMENU)IDC_LOAD_BTN, hInst, nullptr);
        CreateWindowEx(0, L"BUTTON", L"New Config", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            col1 + 236, row, 110, 26, hwnd, (HMENU)IDC_CREATE_BTN, hInst, nullptr);

        UpdateStatusDisplay();
        UpdateKeyDisplay();
        UpdateAllValueLabels(hwnd);
        UpdateDelayControlsEnabled();
        break;
    }

    // ============================================================
    case WM_HSCROLL:
    {
        HWND hTrack = (HWND)lParam;
        int  pos = SliderVal(hTrack);
        int  id = GetDlgCtrlID(hTrack);

        switch (id) {
        case IDC_LEFT_SLIDER:   leftVal = pos; UpdateSliderLabel(hwnd, IDC_VAL_LEFT, pos); break;
        case IDC_RIGHT_SLIDER:  rightVal = pos; UpdateSliderLabel(hwnd, IDC_VAL_RIGHT, pos); break;
        case IDC_UP_SLIDER:     upVal = pos; UpdateSliderLabel(hwnd, IDC_VAL_UP, pos); break;
        case IDC_DOWN_SLIDER:   downVal = pos; UpdateSliderLabel(hwnd, IDC_VAL_DOWN, pos); break;
        case IDC_LEFT2_SLIDER:  leftVal2 = pos; UpdateSliderLabel(hwnd, IDC_VAL_LEFT2, pos); break;
        case IDC_RIGHT2_SLIDER: rightVal2 = pos; UpdateSliderLabel(hwnd, IDC_VAL_RIGHT2, pos); break;
        case IDC_UP2_SLIDER:    upVal2 = pos; UpdateSliderLabel(hwnd, IDC_VAL_UP2, pos); break;
        case IDC_DOWN2_SLIDER:  downVal2 = pos; UpdateSliderLabel(hwnd, IDC_VAL_DOWN2, pos); break;
        case IDC_SENS_SLIDER:
            sensVal = pos;
            { wchar_t buf[16]; swprintf_s(buf, L"%.2f", sensVal / 10.0f); SetDlgItemText(hwnd, IDC_SENS_LABEL, buf); }
            break;
        case IDC_DELAY_SLIDER:
            delayMs = pos;
            UpdateSliderLabel(hwnd, IDC_VAL_DELAY, pos, true);
            break;
        }
        break;
    }

    // ============================================================
    case WM_COMMAND:
    {
        WORD id = LOWORD(wParam);
        WORD code = HIWORD(wParam);

        if (id == IDC_SETKEY_BTN && !capturingKey) {
            capturingKey = true;
            SetWindowText(hSetKeyBtn, L"Press a key...");
            EnableWindow(hSetKeyBtn, FALSE);
        }
        else if (id == IDC_DELAY_ENABLE) {
            delayEnabled = (SendMessage(hDelayCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);
            UpdateDelayControlsEnabled();
        }
        else if (id == IDC_ACTIVATION_MODE && code == CBN_SELCHANGE) {
            activationMode = (int)SendMessage(hActivationCombo, CB_GETCURSEL, 0, 0);
            if (activationMode < 0) activationMode = 0;
            active = false;
            usingSecondary = false;
            UpdateStatusDisplay();
        }
        else if (id == IDC_SAVE_BTN) {
            SaveConfig(hwnd);
            MessageBox(hwnd, L"Config saved.", L"Info", MB_OK);
        }
        else if (id == IDC_LOAD_BTN) {
            if (LoadConfig(hwnd)) {
                ApplyLoadedConfig();
                MessageBox(hwnd, L"Config loaded.", L"Info", MB_OK);
            }
        }
        else if (id == IDC_CREATE_BTN) {
            ResetConfig();
            ApplyLoadedConfig();
            MessageBox(hwnd, L"New config created with defaults.", L"Info", MB_OK);
        }
        break;
    }

    // ============================================================
    case WM_TIMER:
    {
        if (wParam != IDT_MOUSE_MOVE || !active) break;

        int lv, rv, uv, dv;

        if (delayEnabled) {
            DWORD now = timeGetTime();
            if (!usingSecondary && (now - activationTime) >= (DWORD)delayMs) {
                usingSecondary = true;
                accumX = accumY = 0.0f;   // clear sub-pixel debt on transition
                UpdateStatusDisplay();
            }
        }

        if (delayEnabled && usingSecondary) {
            lv = leftVal2; rv = rightVal2; uv = upVal2; dv = downVal2;
        }
        else {
            lv = leftVal;  rv = rightVal;  uv = upVal;  dv = downVal;
        }

        int rawDX = rv - lv;
        int rawDY = dv - uv;

        if (rawDX != 0 || rawDY != 0) {
            float invSens = 1.0f / (sensVal / 10.0f);
            // Scale movement by STEP_SCALE (0.1) for finer control
            accumX += rawDX * invSens * STEP_SCALE;
            accumY += rawDY * invSens * STEP_SCALE;

            int ix = (int)accumX;
            int iy = (int)accumY;
            accumX -= ix;
            accumY -= iy;

            if (ix != 0 || iy != 0) {
                INPUT input = {};
                input.type = INPUT_MOUSE;
                input.mi.dwFlags = MOUSEEVENTF_MOVE;
                input.mi.dx = ix;
                input.mi.dy = iy;
                SendInput(1, &input, sizeof(INPUT));
            }
        }
        break;
    }

    case WM_APP_UPDATE_KEY:
        capturingKey = false;
        EnableWindow(hSetKeyBtn, TRUE);
        SetWindowText(hSetKeyBtn, L"Set Toggle Key");
        UpdateKeyDisplay();
        break;

    case WM_APP_UPDATE_STATUS:
        UpdateStatusDisplay();
        break;

    case WM_DESTROY:
        KillTimer(hwnd, IDT_MOUSE_MOVE);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ---------------------------------------------------------------
// Low-level keyboard hook
// ---------------------------------------------------------------
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION) {
        PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
        bool keyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        bool keyUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);

        if (capturingKey && keyDown) {
            toggleKey = p->vkCode;
            capturingKey = false;
            PostMessage(hMainWnd, WM_APP_UPDATE_KEY, 0, 0);
            return 1;
        }

        if (activationMode == 0) {
            static bool toggleKeyDown = false;
            if (p->vkCode == toggleKey) {
                if (keyDown && !toggleKeyDown) {
                    toggleKeyDown = true;
                    active = !active;
                    accumX = accumY = 0.0f;
                    usingSecondary = false;
                    if (active) activationTime = timeGetTime();
                    PostMessage(hMainWnd, WM_APP_UPDATE_STATUS, 0, 0);
                    return 1;
                }
                if (keyUp) { toggleKeyDown = false; return 1; }
                return 1;
            }
        }
    }
    return CallNextHookEx(hhkKeyboard, nCode, wParam, lParam);
}

// ---------------------------------------------------------------
// Low-level mouse hook
// ---------------------------------------------------------------
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && activationMode > 0) {
        static bool leftDown = false, rightDown = false;
        switch (wParam) {
        case WM_LBUTTONDOWN: leftDown = true;  break;
        case WM_LBUTTONUP:   leftDown = false; break;
        case WM_RBUTTONDOWN: rightDown = true;  break;
        case WM_RBUTTONUP:   rightDown = false; break;
        }
        bool newActive = (activationMode == 1) ? leftDown
            : (activationMode == 2) ? (leftDown && rightDown)
            : false;
        if (newActive != active) {
            active = newActive;
            accumX = accumY = 0.0f;
            usingSecondary = false;
            if (active) activationTime = timeGetTime();
            PostMessage(hMainWnd, WM_APP_UPDATE_STATUS, 0, 0);
        }
    }
    return CallNextHookEx(hhkMouse, nCode, wParam, lParam);
}

// ---------------------------------------------------------------
// UI helpers
// ---------------------------------------------------------------
void UpdateStatusDisplay()
{
    if (!hStatus) return;
    std::wstring text = L"Status: ";
    if (active) {
        if (delayEnabled && usingSecondary) text += L"Active — Secondary settings";
        else                                text += L"Active — Primary settings";
    }
    else {
        text += L"Inactive";
    }
    if (activationMode == 1)      text += L"  [Hold M1]";
    else if (activationMode == 2) text += L"  [Hold M1+M2]";
    SetWindowText(hStatus, text.c_str());
}

void UpdateKeyDisplay()
{
    if (!hKeyDisplay) return;
    wchar_t keyName[64] = {};
    UINT scanCode = MapVirtualKey(toggleKey, MAPVK_VK_TO_VSC);
    LONG lParamKey = scanCode << 16;
    switch (toggleKey) {
    case VK_INSERT: case VK_DELETE: case VK_HOME: case VK_END:
    case VK_PRIOR:  case VK_NEXT:   case VK_LEFT: case VK_RIGHT:
    case VK_UP:     case VK_DOWN:   case VK_NUMLOCK: case VK_CANCEL:
    case VK_DIVIDE: case VK_RSHIFT: case VK_RCONTROL: case VK_RMENU:
        lParamKey |= (1 << 24);
        break;
    }
    GetKeyNameTextW(lParamKey, keyName, 64);
    SetWindowText(hKeyDisplay, (std::wstring(L"Key: ") + keyName).c_str());
}

void UpdateDelayLabel()   // kept for compat but value label does the work now
{
    if (!hDelayCheck) return;   // nothing extra needed
}

void UpdateDelayControlsEnabled()
{
    BOOL en = delayEnabled ? TRUE : FALSE;
    if (hTrackDelay)  EnableWindow(hTrackDelay, en);
    if (hTrackLeft2)  EnableWindow(hTrackLeft2, en);
    if (hTrackRight2) EnableWindow(hTrackRight2, en);
    if (hTrackUp2)    EnableWindow(hTrackUp2, en);
    if (hTrackDown2)  EnableWindow(hTrackDown2, en);
    // Grey out the value labels too
    HWND ids[] = {
        GetDlgItem(hMainWnd, IDC_VAL_DELAY),
        GetDlgItem(hMainWnd, IDC_VAL_LEFT2),
        GetDlgItem(hMainWnd, IDC_VAL_RIGHT2),
        GetDlgItem(hMainWnd, IDC_VAL_UP2),
        GetDlgItem(hMainWnd, IDC_VAL_DOWN2),
    };
    for (HWND h : ids) if (h) EnableWindow(h, en);
}

void ApplyLoadedConfig()
{
    SetSlider(hTrackLeft, leftVal);
    SetSlider(hTrackRight, rightVal);
    SetSlider(hTrackUp, upVal);
    SetSlider(hTrackDown, downVal);
    SetSlider(hTrackSens, sensVal);
    SetSlider(hTrackLeft2, leftVal2);
    SetSlider(hTrackRight2, rightVal2);
    SetSlider(hTrackUp2, upVal2);
    SetSlider(hTrackDown2, downVal2);
    SetSlider(hTrackDelay, delayMs);
    SendMessage(hActivationCombo, CB_SETCURSEL, activationMode, 0);
    SendMessage(hDelayCheck, BM_SETCHECK, delayEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
    UpdateKeyDisplay();
    UpdateAllValueLabels(hMainWnd);
    UpdateDelayControlsEnabled();
    UpdateStatusDisplay();
}

// ---------------------------------------------------------------
// File dialog
// ---------------------------------------------------------------
std::wstring BrowseConfigFile(HWND hwnd, bool saveDialog)
{
    wchar_t fileName[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"INI Files (*.ini)\0*.ini\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = L"ini";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_EXPLORER;
    if (saveDialog) {
        wcscpy_s(fileName, L"config.ini");
        ofn.Flags |= OFN_OVERWRITEPROMPT;
        return GetSaveFileNameW(&ofn) ? fileName : L"";
    }
    else {
        ofn.Flags |= OFN_FILEMUSTEXIST;
        return GetOpenFileNameW(&ofn) ? fileName : L"";
    }
}

// ---------------------------------------------------------------
// Save / Load / Reset
// ---------------------------------------------------------------
void SaveConfig(HWND hwnd)
{
    std::wstring ini = BrowseConfigFile(hwnd, true);
    if (ini.empty()) return;
    auto W = [](int v) { return std::to_wstring(v); };
    const wchar_t* f = ini.c_str();

    WritePrivateProfileStringW(L"Recoil", L"Left", W(leftVal).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"Right", W(rightVal).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"Up", W(upVal).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"Down", W(downVal).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"Sensitivity", W(sensVal).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"ToggleKey", W(toggleKey).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"ActivationMode", W(activationMode).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"DelayEnabled", W(delayEnabled ? 1 : 0).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"DelayMs", W(delayMs).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"Left2", W(leftVal2).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"Right2", W(rightVal2).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"Up2", W(upVal2).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"Down2", W(downVal2).c_str(), f);
}

bool LoadConfig(HWND hwnd)
{
    std::wstring ini = BrowseConfigFile(hwnd, false);
    if (ini.empty()) return false;
    const wchar_t* f = ini.c_str();

    leftVal = GetPrivateProfileIntW(L"Recoil", L"Left", 0, f);
    rightVal = GetPrivateProfileIntW(L"Recoil", L"Right", 0, f);
    upVal = GetPrivateProfileIntW(L"Recoil", L"Up", 0, f);
    downVal = GetPrivateProfileIntW(L"Recoil", L"Down", 0, f);
    sensVal = GetPrivateProfileIntW(L"Recoil", L"Sensitivity", 10, f);
    toggleKey = GetPrivateProfileIntW(L"Recoil", L"ToggleKey", VK_F2, f);
    activationMode = GetPrivateProfileIntW(L"Recoil", L"ActivationMode", 0, f);
    delayEnabled = GetPrivateProfileIntW(L"Recoil", L"DelayEnabled", 0, f) != 0;
    delayMs = GetPrivateProfileIntW(L"Recoil", L"DelayMs", 500, f);
    leftVal2 = GetPrivateProfileIntW(L"Recoil", L"Left2", 0, f);
    rightVal2 = GetPrivateProfileIntW(L"Recoil", L"Right2", 0, f);
    upVal2 = GetPrivateProfileIntW(L"Recoil", L"Up2", 0, f);
    downVal2 = GetPrivateProfileIntW(L"Recoil", L"Down2", 0, f);

    // Clamp
    if (activationMode < 0 || activationMode > 2) activationMode = 0;
    if (sensVal < 1)    sensVal = 1;
    if (sensVal > 100)  sensVal = 100;
    if (delayMs < 100)  delayMs = 100;
    if (delayMs > 5000) delayMs = 5000;

    return true;
}

void ResetConfig()
{
    leftVal = rightVal = upVal = downVal = 0;
    leftVal2 = rightVal2 = upVal2 = downVal2 = 0;
    sensVal = 10;
    toggleKey = VK_F2;
    activationMode = 0;
    delayEnabled = false;
    delayMs = 500;
}
