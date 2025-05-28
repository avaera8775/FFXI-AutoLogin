#include "theme.h"
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

namespace ThemeHelper {
    
    HBRUSH CreateSolidBrush(COLORREF color) {
        return ::CreateSolidBrush(color);
    }
    
    HPEN CreatePen(COLORREF color, int width) {
        return ::CreatePen(PS_SOLID, width, color);
    }
    
    HFONT CreateFont(int size, bool bold) {
        return ::CreateFont(
            -MulDiv(size, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72),
            0, 0, 0,
            bold ? FW_BOLD : FW_NORMAL,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET,
            OUT_TT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            L"Segoe UI"
        );
    }
    
    void DrawRoundedRect(HDC hdc, RECT rect, COLORREF fillColor, COLORREF borderColor, int radius) {
        Graphics graphics(hdc);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        
        // Create brushes and pens
        SolidBrush fillBrush(Color(GetRValue(fillColor), GetGValue(fillColor), GetBValue(fillColor)));
        Pen borderPen(Color(GetRValue(borderColor), GetGValue(borderColor), GetBValue(borderColor)), 1.0f);
        
        // Create rounded rectangle path
        GraphicsPath path;
        path.AddArc(rect.left, rect.top, radius * 2, radius * 2, 180, 90);
        path.AddArc(rect.right - radius * 2, rect.top, radius * 2, radius * 2, 270, 90);
        path.AddArc(rect.right - radius * 2, rect.bottom - radius * 2, radius * 2, radius * 2, 0, 90);
        path.AddArc(rect.left, rect.bottom - radius * 2, radius * 2, radius * 2, 90, 90);
        path.CloseFigure();
        
        // Fill and draw border
        graphics.FillPath(&fillBrush, &path);
        graphics.DrawPath(&borderPen, &path);
    }
    
    void DrawButton(HDC hdc, RECT rect, const wchar_t* text, bool hovered, bool pressed, bool enabled) {
        COLORREF bgColor = Theme::BG_CONTROL;
        COLORREF borderColor = Theme::BORDER_NORMAL;
        COLORREF textColor = Theme::TEXT_PRIMARY;
        
        if (!enabled) {
            bgColor = Theme::BG_CONTROL;
            borderColor = Theme::BORDER_NORMAL;
            textColor = Theme::TEXT_DISABLED;
        } else if (pressed) {
            bgColor = Theme::ACCENT_BLUE;
            borderColor = Theme::ACCENT_BLUE;
            textColor = RGB(255, 255, 255);
        } else if (hovered) {
            bgColor = Theme::BG_HOVER;
            borderColor = Theme::BORDER_HOVER;
        }
        
        // Draw button background
        DrawRoundedRect(hdc, rect, bgColor, borderColor);
        
        // Draw button text
        if (text && wcslen(text) > 0) {
            HFONT font = CreateFont(Theme::FONT_SIZE_NORMAL, false);
            HFONT oldFont = (HFONT)SelectObject(hdc, font);
            
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, textColor);
            
            DrawText(hdc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            
            SelectObject(hdc, oldFont);
            DeleteObject(font);
        }
    }
    
    void DrawTextBox(HDC hdc, RECT rect, bool focused, bool enabled) {
        COLORREF bgColor = enabled ? Theme::BG_CONTROL : Theme::BG_DARK;
        COLORREF borderColor = focused ? Theme::BORDER_FOCUS : Theme::BORDER_NORMAL;
        
        DrawRoundedRect(hdc, rect, bgColor, borderColor);
    }
    
    void DrawPanel(HDC hdc, RECT rect, const wchar_t* title) {
        // Draw panel background
        HBRUSH bgBrush = CreateSolidBrush(Theme::BG_PANEL);
        HPEN borderPen = CreatePen(Theme::BORDER_NORMAL);
        
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, bgBrush);
        HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
        
        Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
        
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(bgBrush);
        DeleteObject(borderPen);
        
        // Draw title if provided
        if (title && wcslen(title) > 0) {
            HFONT titleFont = CreateFont(Theme::FONT_SIZE_LARGE, true);
            HFONT oldFont = (HFONT)SelectObject(hdc, titleFont);
            
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, Theme::TEXT_PRIMARY);
            
            RECT titleRect = { rect.left + Theme::PADDING_NORMAL, rect.top + Theme::PADDING_SMALL, 
                              rect.right - Theme::PADDING_NORMAL, rect.top + Theme::PADDING_SMALL + 20 };
            DrawText(hdc, title, -1, &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            
            SelectObject(hdc, oldFont);
            DeleteObject(titleFont);
        }
    }
    
    void DrawCharacterCard(HDC hdc, RECT rect, const wchar_t* name, const wchar_t* slot, bool hovered) {
        COLORREF bgColor = hovered ? Theme::BG_HOVER : Theme::BG_CONTROL;
        COLORREF borderColor = hovered ? Theme::BORDER_HOVER : Theme::BORDER_NORMAL;
        
        DrawRoundedRect(hdc, rect, bgColor, borderColor);
        
        // Draw character name
        HFONT nameFont = CreateFont(Theme::FONT_SIZE_NORMAL, true);
        HFONT oldFont = (HFONT)SelectObject(hdc, nameFont);
        
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, Theme::TEXT_PRIMARY);
        
        RECT nameRect = { rect.left + Theme::PADDING_NORMAL, rect.top + Theme::PADDING_SMALL,
                         rect.right - 100, rect.top + Theme::PADDING_SMALL + 16 };
        DrawText(hdc, name, -1, &nameRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        
        // Draw slot info
        HFONT slotFont = CreateFont(Theme::FONT_SIZE_NORMAL, false);
        SelectObject(hdc, slotFont);
        SetTextColor(hdc, Theme::TEXT_SECONDARY);
        
        RECT slotRect = { rect.left + Theme::PADDING_NORMAL, rect.top + Theme::PADDING_SMALL + 18,
                         rect.right - 100, rect.bottom - Theme::PADDING_SMALL };
        DrawText(hdc, slot, -1, &slotRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        
        SelectObject(hdc, oldFont);
        DeleteObject(nameFont);
        DeleteObject(slotFont);
        
        // Draw launch button area
        RECT buttonRect = { rect.right - 80, rect.top + Theme::PADDING_SMALL,
                           rect.right - Theme::PADDING_SMALL, rect.bottom - Theme::PADDING_SMALL };
        DrawButton(hdc, buttonRect, L"Launch", hovered, false, true);
    }
}
