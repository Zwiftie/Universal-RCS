#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <sstream>
#include <commdlg.h>

#pragma comment(lib, "comctl32.lib")


// Control IDs
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
#define IDC_ACTIVATION_MODE  112    // new combo box

// Custom messages
#define WM_APP_UPDATE_KEY    (WM_APP + 1)
#define WM_APP_UPDATE_STATUS (WM_APP + 2)

// Global variables
HINSTANCE hInst;
HWND hMainWnd = nullptr;
HWND hTrackLeft, hTrackRight, hTrackUp, hTrackDown, hTrackSens;
HWND hStatus, hKeyDisplay, hActivationCombo;
HWND hSetKeyBtn;

bool  active = false;
bool  capturingKey = false;

int   leftVal = 0, rightVal = 0, upVal = 0, downVal = 0;
int   sensVal = 10;
UINT  toggleKey = VK_F2;
int   activationMode = 0;   // 0=Toggle key, 1=Mouse 1, 2=Mouse 1+2

// Sub‑pixel accumulators
float accumX = 0.0f, accumY = 0.0f;

HHOOK hhkKeyboard = nullptr;
HHOOK hhkMouse = nullptr;    // low‑level mouse hook

// Forward declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
void UpdateStatusDisplay();
void UpdateKeyDisplay();
void UpdateSensitivityLabel(HWND hwnd);
void SaveConfig(HWND hwnd);
bool LoadConfig(HWND hwnd);
void ResetConfig();

//-------------------------------------------------------------------
// WinMain
//-------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    hInst = hInstance;

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;   // standard classes for combo box
    InitCommonControlsEx(&icex);

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"RecoilControlClass";

    if (!RegisterClassEx(&wc))
    {
        MessageBox(nullptr, L"Window Registration Failed!", L"Error", MB_ICONERROR);
        return 0;
    }

    // Increased height to fit the new control (430 x 440)
    hMainWnd = CreateWindowEx(0, L"RecoilControlClass", L"Recoil Control",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 430, 440,
        nullptr, nullptr, hInstance, nullptr);
    if (!hMainWnd)
    {
        MessageBox(nullptr, L"Window Creation Failed!", L"Error", MB_ICONERROR);
        return 0;
    }

    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);

    // Install hooks
    hhkKeyboard = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);
    if (!hhkKeyboard)
        MessageBox(hMainWnd, L"Failed to install keyboard hook!", L"Error", MB_ICONERROR);

    hhkMouse = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, hInstance, 0);
    if (!hhkMouse)
        MessageBox(hMainWnd, L"Failed to install mouse hook!", L"Error", MB_ICONERROR);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (hhkKeyboard) UnhookWindowsHookEx(hhkKeyboard);
    if (hhkMouse)    UnhookWindowsHookEx(hhkMouse);
    return static_cast<int>(msg.wParam);
}

//-------------------------------------------------------------------
// Main window procedure
//-------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        LoadConfig(hwnd);

        // ---------- sliders ----------
        // Left
        hTrackLeft = CreateWindowEx(0, TRACKBAR_CLASS, L"Left",
            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ,
            100, 20, 200, 30, hwnd, (HMENU)IDC_LEFT_SLIDER, hInst, nullptr);
        SendMessage(hTrackLeft, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
        SendMessage(hTrackLeft, TBM_SETPOS, TRUE, leftVal);
        CreateWindowEx(0, L"STATIC", L"Left:", WS_CHILD | WS_VISIBLE,
            20, 20, 70, 20, hwnd, nullptr, hInst, nullptr);

        // Right
        hTrackRight = CreateWindowEx(0, TRACKBAR_CLASS, L"Right",
            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ,
            100, 60, 200, 30, hwnd, (HMENU)IDC_RIGHT_SLIDER, hInst, nullptr);
        SendMessage(hTrackRight, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
        SendMessage(hTrackRight, TBM_SETPOS, TRUE, rightVal);
        CreateWindowEx(0, L"STATIC", L"Right:", WS_CHILD | WS_VISIBLE,
            20, 60, 70, 20, hwnd, nullptr, hInst, nullptr);

        // Up
        hTrackUp = CreateWindowEx(0, TRACKBAR_CLASS, L"Up",
            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ,
            100, 100, 200, 30, hwnd, (HMENU)IDC_UP_SLIDER, hInst, nullptr);
        SendMessage(hTrackUp, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
        SendMessage(hTrackUp, TBM_SETPOS, TRUE, upVal);
        CreateWindowEx(0, L"STATIC", L"Up:", WS_CHILD | WS_VISIBLE,
            20, 100, 70, 20, hwnd, nullptr, hInst, nullptr);

        // Down
        hTrackDown = CreateWindowEx(0, TRACKBAR_CLASS, L"Down",
            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ,
            100, 140, 200, 30, hwnd, (HMENU)IDC_DOWN_SLIDER, hInst, nullptr);
        SendMessage(hTrackDown, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
        SendMessage(hTrackDown, TBM_SETPOS, TRUE, downVal);
        CreateWindowEx(0, L"STATIC", L"Down:", WS_CHILD | WS_VISIBLE,
            20, 140, 70, 20, hwnd, nullptr, hInst, nullptr);

        // Sensitivity
        hTrackSens = CreateWindowEx(0, TRACKBAR_CLASS, L"Sens",
            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ,
            100, 180, 200, 30, hwnd, (HMENU)IDC_SENS_SLIDER, hInst, nullptr);
        SendMessage(hTrackSens, TBM_SETRANGE, TRUE, MAKELONG(1, 100));
        SendMessage(hTrackSens, TBM_SETPOS, TRUE, sensVal);
        CreateWindowEx(0, L"STATIC", L"Sensitivity:", WS_CHILD | WS_VISIBLE,
            20, 180, 70, 20, hwnd, nullptr, hInst, nullptr);
        CreateWindowEx(0, L"STATIC", L"1.0", WS_CHILD | WS_VISIBLE,
            310, 180, 50, 20, hwnd, (HMENU)205, hInst, nullptr);

        // ---------- toggle key ----------
        hSetKeyBtn = CreateWindowEx(0, L"BUTTON", L"Set Toggle Key",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            20, 220, 120, 25, hwnd, (HMENU)IDC_SETKEY_BTN, hInst, nullptr);
        hKeyDisplay = CreateWindowEx(0, L"STATIC", L"",
            WS_CHILD | WS_VISIBLE,
            150, 222, 200, 20, hwnd, (HMENU)IDC_KEY_DISPLAY, hInst, nullptr);

        // ---------- activation mode combo ----------
        CreateWindowEx(0, L"STATIC", L"Activation Mode:", WS_CHILD | WS_VISIBLE,
            20, 260, 100, 20, hwnd, nullptr, hInst, nullptr);
        hActivationCombo = CreateWindowEx(0, WC_COMBOBOX, L"",
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS,
            130, 260, 150, 200, hwnd, (HMENU)IDC_ACTIVATION_MODE, hInst, nullptr);
        // Add items
        SendMessage(hActivationCombo, CB_ADDSTRING, 0, (LPARAM)L"Toggle Key");
        SendMessage(hActivationCombo, CB_ADDSTRING, 0, (LPARAM)L"Mouse 1");
        SendMessage(hActivationCombo, CB_ADDSTRING, 0, (LPARAM)L"Mouse 1+2");
        SendMessage(hActivationCombo, CB_SETCURSEL, activationMode, 0);

        // ---------- status ----------
        hStatus = CreateWindowEx(0, L"STATIC", L"",
            WS_CHILD | WS_VISIBLE,
            20, 290, 200, 20, hwnd, (HMENU)IDC_STATUS, hInst, nullptr);

        // ---------- config buttons ----------
        CreateWindowEx(0, L"BUTTON", L"Save Config", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            20, 320, 100, 25, hwnd, (HMENU)IDC_SAVE_BTN, hInst, nullptr);
        CreateWindowEx(0, L"BUTTON", L"Load Config", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            130, 320, 100, 25, hwnd, (HMENU)IDC_LOAD_BTN, hInst, nullptr);
        CreateWindowEx(0, L"BUTTON", L"New Config", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            240, 320, 100, 25, hwnd, (HMENU)IDC_CREATE_BTN, hInst, nullptr);

        UpdateStatusDisplay();
        UpdateKeyDisplay();
        UpdateSensitivityLabel(hwnd);

        SetTimer(hwnd, IDT_MOUSE_MOVE, 10, nullptr);
        break;
    }

    case WM_HSCROLL:
    {
        HWND hTrack = (HWND)lParam;
        int pos = static_cast<int>(SendMessage(hTrack, TBM_GETPOS, 0, 0));
        int id = GetDlgCtrlID(hTrack);

        if (id == IDC_LEFT_SLIDER)  leftVal = pos;
        else if (id == IDC_RIGHT_SLIDER) rightVal = pos;
        else if (id == IDC_UP_SLIDER)    upVal = pos;
        else if (id == IDC_DOWN_SLIDER)  downVal = pos;
        else if (id == IDC_SENS_SLIDER)  sensVal = pos;
        if (id == IDC_SENS_SLIDER)
            UpdateSensitivityLabel(hwnd);
        break;
    }

    case WM_COMMAND:
    {
        WORD id = LOWORD(wParam);
        WORD code = HIWORD(wParam);

        if (id == IDC_SETKEY_BTN)
        {
            if (!capturingKey)
            {
                capturingKey = true;
                SetWindowText(hSetKeyBtn, L"Press a key...");
                EnableWindow(hSetKeyBtn, FALSE);
            }
        }
        else if (id == IDC_SAVE_BTN)
        {
            SaveConfig(hwnd);
            MessageBox(hwnd, L"Config saved.", L"Info", MB_OK);
        }
        else if (id == IDC_LOAD_BTN)
        {
            LoadConfig(hwnd);
            SendMessage(hTrackLeft, TBM_SETPOS, TRUE, leftVal);
            SendMessage(hTrackRight, TBM_SETPOS, TRUE, rightVal);
            SendMessage(hTrackUp, TBM_SETPOS, TRUE, upVal);
            SendMessage(hTrackDown, TBM_SETPOS, TRUE, downVal);
            SendMessage(hTrackSens, TBM_SETPOS, TRUE, sensVal);
            SendMessage(hActivationCombo, CB_SETCURSEL, activationMode, 0);
            UpdateKeyDisplay();
            UpdateSensitivityLabel(hwnd);
            MessageBox(hwnd, L"Config loaded.", L"Info", MB_OK);
        }
        else if (id == IDC_CREATE_BTN)
        {
            ResetConfig();
            SendMessage(hTrackLeft, TBM_SETPOS, TRUE, leftVal);
            SendMessage(hTrackRight, TBM_SETPOS, TRUE, rightVal);
            SendMessage(hTrackUp, TBM_SETPOS, TRUE, upVal);
            SendMessage(hTrackDown, TBM_SETPOS, TRUE, downVal);
            SendMessage(hTrackSens, TBM_SETPOS, TRUE, sensVal);
            SendMessage(hActivationCombo, CB_SETCURSEL, activationMode, 0);
            UpdateKeyDisplay();
            UpdateSensitivityLabel(hwnd);
            MessageBox(hwnd, L"New config created with defaults.", L"Info", MB_OK);
        }
        else if (id == IDC_ACTIVATION_MODE && code == CBN_SELCHANGE)
        {
            activationMode = (int)SendMessage(hActivationCombo, CB_GETCURSEL, 0, 0);
            if (activationMode < 0) activationMode = 0;   // safety
            // Turn off active when switching mode to avoid unexpected behaviour
            active = false;
            UpdateStatusDisplay();
        }
        break;
    }

    case WM_TIMER:
    {
        if (wParam == IDT_MOUSE_MOVE && active)
        {
            int rawDX = rightVal - leftVal;
            int rawDY = downVal - upVal;
            if (rawDX != 0 || rawDY != 0)
            {
                float sens = sensVal / 10.0f;
                float multiplier = 1.0f / sens;

                accumX += rawDX * multiplier;
                accumY += rawDY * multiplier;

                int ix = static_cast<int>(accumX);
                int iy = static_cast<int>(accumY);
                accumX -= ix;
                accumY -= iy;

                if (ix != 0 || iy != 0)
                {
                    INPUT input = {};
                    input.type = INPUT_MOUSE;
                    input.mi.dwFlags = MOUSEEVENTF_MOVE;
                    input.mi.dx = ix;
                    input.mi.dy = iy;
                    SendInput(1, &input, sizeof(INPUT));
                }
            }
        }
        break;
    }

    case WM_APP_UPDATE_KEY:
    {
        capturingKey = false;
        EnableWindow(hSetKeyBtn, TRUE);
        SetWindowText(hSetKeyBtn, L"Set Toggle Key");
        UpdateKeyDisplay();
        break;
    }

    case WM_APP_UPDATE_STATUS:
    {
        UpdateStatusDisplay();
        break;
    }

    case WM_DESTROY:
    {
        KillTimer(hwnd, IDT_MOUSE_MOVE);
        PostQuitMessage(0);
        break;
    }

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

//-------------------------------------------------------------------
// Low‑level keyboard hook
//-------------------------------------------------------------------
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
        bool keyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        bool keyUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);

        if (capturingKey && keyDown)
        {
            toggleKey = p->vkCode;
            capturingKey = false;
            PostMessage(hMainWnd, WM_APP_UPDATE_KEY, 0, 0);
            return 1;
        }

        // Toggle key only works in activation mode 0
        if (activationMode == 0)
        {
            static bool toggleKeyDown = false;
            if (p->vkCode == toggleKey)
            {
                if (keyDown && !toggleKeyDown)
                {
                    toggleKeyDown = true;
                    active = !active;
                    accumX = accumY = 0.0f;
                    PostMessage(hMainWnd, WM_APP_UPDATE_STATUS, 0, 0);
                    return 1;
                }
                if (keyUp)
                {
                    toggleKeyDown = false;
                    return 1;
                }
                return 1;
            }
        }
    }
    return CallNextHookEx(hhkKeyboard, nCode, wParam, lParam);
}

//-------------------------------------------------------------------
// Low‑level mouse hook – handles hold activation modes
//-------------------------------------------------------------------
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && activationMode > 0)
    {
        static bool leftDown = false;
        static bool rightDown = false;

        switch (wParam)
        {
        case WM_LBUTTONDOWN:  leftDown = true;  break;
        case WM_LBUTTONUP:    leftDown = false; break;
        case WM_RBUTTONDOWN:  rightDown = true;  break;
        case WM_RBUTTONUP:    rightDown = false; break;
        }

        bool newActive = false;
        if (activationMode == 1)
            newActive = leftDown;               // hold Mouse 1
        else if (activationMode == 2)
            newActive = leftDown && rightDown;  // hold both Mouse 1 & 2

        if (newActive != active)
        {
            active = newActive;
            if (active)
                accumX = accumY = 0.0f;   // reset when activating via mouse
            PostMessage(hMainWnd, WM_APP_UPDATE_STATUS, 0, 0);
        }
    }
    return CallNextHookEx(hhkMouse, nCode, wParam, lParam);
}

//-------------------------------------------------------------------
// Helper functions
//-------------------------------------------------------------------
void UpdateStatusDisplay()
{
    if (hStatus)
    {
        std::wstring text = L"Status: ";
        text += active ? L"Active" : L"Inactive";
        // Optionally show mode
        if (activationMode == 1)       text += L" (M1)";
        else if (activationMode == 2)  text += L" (M1+2)";
        SetWindowText(hStatus, text.c_str());
    }
}

void UpdateKeyDisplay()
{
    if (!hKeyDisplay) return;
    wchar_t keyName[64] = {};
    UINT scanCode = MapVirtualKey(toggleKey, MAPVK_VK_TO_VSC);
    LONG lParamKey = scanCode << 16;

    switch (toggleKey)
    {
    case VK_INSERT: case VK_DELETE: case VK_HOME: case VK_END:
    case VK_PRIOR:  case VK_NEXT:   case VK_LEFT: case VK_RIGHT:
    case VK_UP:     case VK_DOWN:   case VK_NUMLOCK: case VK_CANCEL:
    case VK_DIVIDE:
        lParamKey |= (1 << 24);
        break;
    }
    if (toggleKey == VK_RSHIFT || toggleKey == VK_RCONTROL || toggleKey == VK_RMENU)
        lParamKey |= (1 << 24);

    GetKeyNameTextW(lParamKey, keyName, 64);
    std::wstring display = L"Key: " + std::wstring(keyName);
    SetWindowText(hKeyDisplay, display.c_str());
}

void UpdateSensitivityLabel(HWND hwnd)
{
    float sens = sensVal / 10.0f;
    wchar_t buf[32];
    swprintf_s(buf, L"%.2f", sens);
    SetDlgItemText(hwnd, 205, buf);
}

std::wstring BrowseConfigFile(HWND hwnd, bool saveDialog)
{
    wchar_t fileName[MAX_PATH] = L"";

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter =
        L"INI Files (*.ini)\0*.ini\0"
        L"All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = L"ini";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_EXPLORER;

    if (saveDialog)
    {
        wcscpy_s(fileName, L"config.ini");
        ofn.Flags |= OFN_OVERWRITEPROMPT;

        if (GetSaveFileNameW(&ofn))
            return fileName;
    }
    else
    {
        ofn.Flags |= OFN_FILEMUSTEXIST;

        if (GetOpenFileNameW(&ofn))
            return fileName;
    }

    return L"";
}

void SaveConfig(HWND hwnd)
{
    std::wstring ini = BrowseConfigFile(hwnd, true);
    if (ini.empty())
        return;

    WritePrivateProfileStringW(L"Recoil", L"Left", std::to_wstring(leftVal).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Recoil", L"Right", std::to_wstring(rightVal).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Recoil", L"Up", std::to_wstring(upVal).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Recoil", L"Down", std::to_wstring(downVal).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Recoil", L"Sensitivity", std::to_wstring(sensVal).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Recoil", L"ToggleKey", std::to_wstring(toggleKey).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Recoil", L"ActivationMode", std::to_wstring(activationMode).c_str(), ini.c_str());
}

bool LoadConfig(HWND hwnd)
{
    std::wstring ini = BrowseConfigFile(hwnd, false);
    if (ini.empty())
        return false;

    leftVal = GetPrivateProfileIntW(L"Recoil", L"Left", 0, ini.c_str());
    rightVal = GetPrivateProfileIntW(L"Recoil", L"Right", 0, ini.c_str());
    upVal = GetPrivateProfileIntW(L"Recoil", L"Up", 0, ini.c_str());
    downVal = GetPrivateProfileIntW(L"Recoil", L"Down", 0, ini.c_str());
    sensVal = GetPrivateProfileIntW(L"Recoil", L"Sensitivity", 10, ini.c_str());
    toggleKey = GetPrivateProfileIntW(L"Recoil", L"ToggleKey", VK_F2, ini.c_str());
    activationMode = GetPrivateProfileIntW(L"Recoil", L"ActivationMode", 0, ini.c_str());

    // Clamp activation mode to valid range
    if (activationMode < 0 || activationMode > 2)
        activationMode = 0;

    return true;
}

void ResetConfig()
{
    leftVal = rightVal = upVal = downVal = 0;
    sensVal = 10;
    toggleKey = VK_F2;
    activationMode = 0;
}