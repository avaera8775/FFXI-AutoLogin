#pragma once
#include <windows.h>

// Modern dark theme color scheme
namespace Theme {
    // Background colors
    const COLORREF BG_DARK = RGB(45, 45, 48);          // #2D2D30
    const COLORREF BG_PANEL = RGB(62, 62, 66);         // #3E3E42
    const COLORREF BG_CONTROL = RGB(51, 51, 55);       // #333337
    const COLORREF BG_HOVER = RGB(70, 70, 74);         // #46464A
    
    // Text colors
    const COLORREF TEXT_PRIMARY = RGB(204, 204, 204);   // #CCCCCC
    const COLORREF TEXT_SECONDARY = RGB(153, 153, 153); // #999999
    const COLORREF TEXT_DISABLED = RGB(102, 102, 102);  // #666666
    
    // Accent colors
    const COLORREF ACCENT_BLUE = RGB(0, 122, 204);      // #007ACC
    const COLORREF ACCENT_GREEN = RGB(78, 201, 176);    // #4EC9B0
    const COLORREF ACCENT_RED = RGB(244, 71, 71);       // #F44747
    
    // Border colors
    const COLORREF BORDER_NORMAL = RGB(64, 64, 64);     // #404040
    const COLORREF BORDER_FOCUS = RGB(0, 122, 204);     // #007ACC
    const COLORREF BORDER_HOVER = RGB(90, 90, 90);      // #5A5A5A
    
    // Fonts
    const int FONT_SIZE_NORMAL = 9;
    const int FONT_SIZE_LARGE = 11;
    const int FONT_SIZE_TITLE = 14;
    
    // Spacing
    const int PADDING_SMALL = 8;
    const int PADDING_NORMAL = 12;
    const int PADDING_LARGE = 16;
    const int BORDER_RADIUS = 4;
    
    // Control dimensions
    const int BUTTON_HEIGHT = 32;
    const int TEXTBOX_HEIGHT = 28;
    const int DROPDOWN_HEIGHT = 28;
}

// Helper functions for drawing
namespace ThemeHelper {
    HBRUSH CreateSolidBrush(COLORREF color);
    HPEN CreatePen(COLORREF color, int width = 1);
    HFONT CreateFont(int size, bool bold = false);
    void DrawRoundedRect(HDC hdc, RECT rect, COLORREF fillColor, COLORREF borderColor, int radius = Theme::BORDER_RADIUS);
    void DrawButton(HDC hdc, RECT rect, const wchar_t* text, bool hovered, bool pressed, bool enabled = true);
    void DrawTextBox(HDC hdc, RECT rect, bool focused, bool enabled = true);
    void DrawPanel(HDC hdc, RECT rect, const wchar_t* title);
    void DrawCharacterCard(HDC hdc, RECT rect, const wchar_t* name, const wchar_t* slot, bool hovered);
}
