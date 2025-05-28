#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include "theme.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// Window class names
const wchar_t* MAIN_WINDOW_CLASS = L"FFXIAutoLoginMainWindow";
const wchar_t* SETUP_WINDOW_CLASS = L"FFXIAutoLoginSetupWindow";

// Control IDs
enum ControlIDs {
    ID_BTN_SETUP = 1001,
    ID_BTN_SETTINGS = 1002,
    ID_BTN_ABOUT = 1003,
    ID_BTN_ADD_CHARACTER = 1004,
    ID_EDIT_CHAR_NAME = 1005,
    ID_EDIT_PASSWORD = 1006,
    ID_EDIT_TOTP = 1007,
    ID_COMBO_SLOT = 1008,
    ID_EDIT_ARGS = 1009,
    ID_LIST_CHARACTERS = 1010
};

// Global variables
HINSTANCE g_hInstance;
HWND g_hMainWindow;
HWND g_hSetupWindow;
GdiplusStartupInput g_gdiplusStartupInput;
ULONG_PTR g_gdiplusToken;

// Character data structure
struct Character {
    std::wstring name;
    std::wstring password;
    std::wstring totpSecret;
    int slot;
    std::wstring args;
};

std::vector<Character> g_characters;

// Forward declarations
LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SetupWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void CreateMainWindow();
void CreateSetupWindow();
void DrawMainWindow(HDC hdc, RECT clientRect);
void DrawSetupWindow(HDC hdc, RECT clientRect);
void LoadCharacters();
void SaveCharacters();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    g_hInstance = hInstance;
    
    // Initialize GDI+
    GdiplusStartup(&g_gdiplusToken, &g_gdiplusStartupInput, NULL);
    
    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);
    
    // Register window classes
    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = MainWindowProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = CreateSolidBrush(Theme::BG_DARK);
    wcex.lpszClassName = MAIN_WINDOW_CLASS;
    RegisterClassEx(&wcex);
    
    wcex.lpfnWndProc = SetupWindowProc;
    wcex.lpszClassName = SETUP_WINDOW_CLASS;
    RegisterClassEx(&wcex);
    
    // Load character data
    LoadCharacters();
    
    // Create main window
    CreateMainWindow();
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Cleanup
    GdiplusShutdown(g_gdiplusToken);
    return (int)msg.wParam;
}

void CreateMainWindow() {
    g_hMainWindow = CreateWindowEx(
        WS_EX_LAYERED,
        MAIN_WINDOW_CLASS,
        L"FFXI AutoLogin",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        600, 500,
        NULL, NULL, g_hInstance, NULL
    );
    
    if (g_hMainWindow) {
        // Set window transparency for modern look
        SetLayeredWindowAttributes(g_hMainWindow, 0, 250, LWA_ALPHA);
        
        ShowWindow(g_hMainWindow, SW_SHOW);
        UpdateWindow(g_hMainWindow);
    }
}

void CreateSetupWindow() {
    if (g_hSetupWindow) {
        SetForegroundWindow(g_hSetupWindow);
        return;
    }
    
    g_hSetupWindow = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_DLGMODALFRAME,
        SETUP_WINDOW_CLASS,
        L"Setup Characters",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT,
        450, 400,
        g_hMainWindow, NULL, g_hInstance, NULL
    );
    
    if (g_hSetupWindow) {
        SetLayeredWindowAttributes(g_hSetupWindow, 0, 250, LWA_ALPHA);
        
        // Center on parent
        RECT parentRect, windowRect;
        GetWindowRect(g_hMainWindow, &parentRect);
        GetWindowRect(g_hSetupWindow, &windowRect);
        
        int x = parentRect.left + (parentRect.right - parentRect.left - (windowRect.right - windowRect.left)) / 2;
        int y = parentRect.top + (parentRect.bottom - parentRect.top - (windowRect.bottom - windowRect.top)) / 2;
        
        SetWindowPos(g_hSetupWindow, HWND_TOP, x, y, 0, 0, SWP_NOSIZE);
        
        ShowWindow(g_hSetupWindow, SW_SHOW);
        UpdateWindow(g_hSetupWindow);
    }
}

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // Create setup button
            CreateWindow(L"BUTTON", L"Setup Characters",
                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                20, 20, 150, Theme::BUTTON_HEIGHT,
                hwnd, (HMENU)ID_BTN_SETUP, g_hInstance, NULL);
            
            // Create settings button
            CreateWindow(L"BUTTON", L"Settings",
                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                20, 420, 100, Theme::BUTTON_HEIGHT,
                hwnd, (HMENU)ID_BTN_SETTINGS, g_hInstance, NULL);
            
            // Create about button
            CreateWindow(L"BUTTON", L"About",
                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                480, 420, 100, Theme::BUTTON_HEIGHT,
                hwnd, (HMENU)ID_BTN_ABOUT, g_hInstance, NULL);
            
            return 0;
        }
        
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case ID_BTN_SETUP:
                    CreateSetupWindow();
                    break;
                    
                case ID_BTN_SETTINGS:
                    MessageBox(hwnd, L"Settings dialog would open here", L"Settings", MB_OK);
                    break;
                    
                case ID_BTN_ABOUT:
                    MessageBox(hwnd, L"FFXI AutoLogin\nCreated by: jaku\nhttps://twitter.com/jaku \n\nGUI created by: avaera\n https://www.github.com/avaera8775/", L"About", MB_OK);
                    break;
            }
            return 0;
        }
        
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;
            if (dis->CtlType == ODT_BUTTON) {
                wchar_t buttonText[256];
                GetWindowText(dis->hwndItem, buttonText, 256);
                
                bool hovered = (dis->itemState & ODS_HOTLIGHT) != 0;
                bool pressed = (dis->itemState & ODS_SELECTED) != 0;
                
                ThemeHelper::DrawButton(dis->hDC, dis->rcItem, buttonText, hovered, pressed);
            }
            return TRUE;
        }
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            DrawMainWindow(hdc, clientRect);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_ERASEBKGND:
            return TRUE; // Prevent flicker
        
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK SetupWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // Character name
            CreateWindow(L"STATIC", L"Character Name:",
                WS_CHILD | WS_VISIBLE,
                20, 60, 120, 20,
                hwnd, NULL, g_hInstance, NULL);
            
            CreateWindow(L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                150, 58, 200, Theme::TEXTBOX_HEIGHT,
                hwnd, (HMENU)ID_EDIT_CHAR_NAME, g_hInstance, NULL);
            
            // Password
            CreateWindow(L"STATIC", L"Password:",
                WS_CHILD | WS_VISIBLE,
                20, 100, 120, 20,
                hwnd, NULL, g_hInstance, NULL);
            
            CreateWindow(L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_PASSWORD,
                150, 98, 200, Theme::TEXTBOX_HEIGHT,
                hwnd, (HMENU)ID_EDIT_PASSWORD, g_hInstance, NULL);
            
            // TOTP Secret
            CreateWindow(L"STATIC", L"TOTP Secret:",
                WS_CHILD | WS_VISIBLE,
                20, 140, 120, 20,
                hwnd, NULL, g_hInstance, NULL);
            
            CreateWindow(L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                150, 138, 200, Theme::TEXTBOX_HEIGHT,
                hwnd, (HMENU)ID_EDIT_TOTP, g_hInstance, NULL);
            
            // Slot
            CreateWindow(L"STATIC", L"Character Slot:",
                WS_CHILD | WS_VISIBLE,
                20, 180, 120, 20,
                hwnd, NULL, g_hInstance, NULL);
            
            HWND hCombo = CreateWindow(L"COMBOBOX", L"",
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                150, 178, 100, 100,
                hwnd, (HMENU)ID_COMBO_SLOT, g_hInstance, NULL);
            
            // Populate slot combo
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Slot 1");
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Slot 2");
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Slot 3");
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Slot 4");
            SendMessage(hCombo, CB_SETCURSEL, 0, 0);
            
            // Windower Args
            CreateWindow(L"STATIC", L"Windower Args:",
                WS_CHILD | WS_VISIBLE,
                20, 220, 120, 20,
                hwnd, NULL, g_hInstance, NULL);
            
            CreateWindow(L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                150, 218, 200, Theme::TEXTBOX_HEIGHT,
                hwnd, (HMENU)ID_EDIT_ARGS, g_hInstance, NULL);
            
            // Add Character button
            CreateWindow(L"BUTTON", L"Add Character",
                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                150, 270, 120, Theme::BUTTON_HEIGHT,
                hwnd, (HMENU)ID_BTN_ADD_CHARACTER, g_hInstance, NULL);
            
            return 0;
        }
        
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case ID_BTN_ADD_CHARACTER: {
                    // Get form data and add character
                    wchar_t name[256], password[256], totp[256], args[256];
                    GetDlgItemText(hwnd, ID_EDIT_CHAR_NAME, name, 256);
                    GetDlgItemText(hwnd, ID_EDIT_PASSWORD, password, 256);
                    GetDlgItemText(hwnd, ID_EDIT_TOTP, totp, 256);
                    GetDlgItemText(hwnd, ID_EDIT_ARGS, args, 256);
                    
                    int slot = (int)SendDlgItemMessage(hwnd, ID_COMBO_SLOT, CB_GETCURSEL, 0, 0) + 1;
                    
                    if (wcslen(name) > 0 && wcslen(password) > 0) {
                        Character newChar;
                        newChar.name = name;
                        newChar.password = password;
                        newChar.totpSecret = totp;
                        newChar.slot = slot;
                        newChar.args = args;
                        
                        g_characters.push_back(newChar);
                        SaveCharacters();
                        
                        // Clear form
                        SetDlgItemText(hwnd, ID_EDIT_CHAR_NAME, L"");
                        SetDlgItemText(hwnd, ID_EDIT_PASSWORD, L"");
                        SetDlgItemText(hwnd, ID_EDIT_TOTP, L"");
                        SetDlgItemText(hwnd, ID_EDIT_ARGS, L"");
                        
                        // Refresh main window
                        InvalidateRect(g_hMainWindow, NULL, TRUE);
                        
                        MessageBox(hwnd, L"Character added successfully!", L"Success", MB_OK);
                    } else {
                        MessageBox(hwnd, L"Please enter at least character name and password.", L"Error", MB_OK);
                    }
                    break;
                }
            }
            return 0;
        }
        
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;
            if (dis->CtlType == ODT_BUTTON) {
                wchar_t buttonText[256];
                GetWindowText(dis->hwndItem, buttonText, 256);
                
                bool hovered = (dis->itemState & ODS_HOTLIGHT) != 0;
                bool pressed = (dis->itemState & ODS_SELECTED) != 0;
                
                ThemeHelper::DrawButton(dis->hDC, dis->rcItem, buttonText, hovered, pressed);
            }
            return TRUE;
        }
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            DrawSetupWindow(hdc, clientRect);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_ERASEBKGND:
            return TRUE;
        
        case WM_CLOSE:
            DestroyWindow(hwnd);
            g_hSetupWindow = NULL;
            return 0;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void DrawMainWindow(HDC hdc, RECT clientRect) {
    // Fill background
    HBRUSH bgBrush = CreateSolidBrush(Theme::BG_DARK);
    FillRect(hdc, &clientRect, bgBrush);
    DeleteObject(bgBrush);
    
    // Draw title
    HFONT titleFont = ThemeHelper::CreateFont(Theme::FONT_SIZE_TITLE, true);
    HFONT oldFont = (HFONT)SelectObject(hdc, titleFont);
    
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, Theme::TEXT_PRIMARY);
    
    RECT titleRect = { 20, 70, clientRect.right - 20, 100 };
    DrawText(hdc, L"Saved Characters", -1, &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    
    SelectObject(hdc, oldFont);
    DeleteObject(titleFont);
    
    // Draw character cards
    int y = 110;
    for (size_t i = 0; i < g_characters.size(); i++) {
        RECT cardRect = { 20, y, clientRect.right - 20, y + 60 };
        
        wchar_t slotText[50];
        swprintf_s(slotText, L"Slot %d", g_characters[i].slot);
        
        ThemeHelper::DrawCharacterCard(hdc, cardRect, g_characters[i].name.c_str(), slotText, false);
        
        y += 70;
    }
    
    // Draw empty state if no characters
    if (g_characters.empty()) {
        HFONT emptyFont = ThemeHelper::CreateFont(Theme::FONT_SIZE_NORMAL, false);
        oldFont = (HFONT)SelectObject(hdc, emptyFont);
        
        SetTextColor(hdc, Theme::TEXT_SECONDARY);
        
        RECT emptyRect = { 20, 150, clientRect.right - 20, 200 };
        DrawText(hdc, L"No characters configured. Click 'Setup Characters' to add some.", -1, &emptyRect, DT_CENTER | DT_VCENTER | DT_WORDBREAK);
        
        SelectObject(hdc, oldFont);
        DeleteObject(emptyFont);
    }
}

void DrawSetupWindow(HDC hdc, RECT clientRect) {
    // Fill background
    HBRUSH bgBrush = CreateSolidBrush(Theme::BG_DARK);
    FillRect(hdc, &clientRect, bgBrush);
    DeleteObject(bgBrush);
    
    // Draw title
    HFONT titleFont = ThemeHelper::CreateFont(Theme::FONT_SIZE_TITLE, true);
    HFONT oldFont = (HFONT)SelectObject(hdc, titleFont);
    
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, Theme::TEXT_PRIMARY);
    
    RECT titleRect = { 20, 20, clientRect.right - 20, 50 };
    DrawText(hdc, L"Add New Character", -1, &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    
    SelectObject(hdc, oldFont);
    DeleteObject(titleFont);
}

void LoadCharacters() {
    // TODO: Load from encrypted config file
    // For now, just initialize empty
    g_characters.clear();
}

void SaveCharacters() {
    // TODO: Save to encrypted config file
    // For now, just a placeholder
}
