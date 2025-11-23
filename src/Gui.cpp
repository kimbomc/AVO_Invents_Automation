#include "Gui.h"
#include "MasterData.h"
#include "Config.h"
#include "SessionState.h"
#include <windows.h>
#include <commdlg.h>
#include <string>
#include <sstream>
#include <commctrl.h>
#include <filesystem>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

namespace fs = std::filesystem;

// Get the directory where the executable is located
static std::string getExeDirectory() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string exePath(buffer);
    size_t pos = exePath.find_last_of("\\/");
    return (pos != std::string::npos) ? exePath.substr(0, pos) : ".";
}

// Get absolute path for a relative path from exe directory
static std::string getAbsolutePath(const std::string& relativePath) {
    static std::string exeDir = getExeDirectory();
    fs::path absPath = fs::path(exeDir) / relativePath;
    return absPath.string();
}

// Button control IDs
#define ID_BTN_LOAD_CSV          1001
#define ID_BTN_GENERATE_BARCODE  1002
#define ID_BTN_OPEN_FOLDER       1003
#define ID_BTN_VIEW_HISTORY      1004

// Status indicator control IDs (not used anymore, kept for compatibility)
#define ID_STATUS_STEP1          1005
#define ID_STATUS_STEP2          1006
#define ID_STATUS_STEP3          1007

// Helper function to draw rounded rectangle
void DrawRoundedRect(HDC hdc, RECT rect, int radius, HPEN pen, HBRUSH brush) {
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
}

// Helper function to draw simple upload icon (box with arrow)
void DrawUploadIcon(HDC hdc, int x, int y, int size, COLORREF color) {
    HPEN iconPen = CreatePen(PS_SOLID, 2, color);
    HPEN oldPen = (HPEN)SelectObject(hdc, iconPen);
    
    // Box
    Rectangle(hdc, x, y + size/3, x + size, y + size);
    
    // Arrow up
    MoveToEx(hdc, x + size/2, y + size/3, NULL);
    LineTo(hdc, x + size/2, y);
    
    // Arrow head
    MoveToEx(hdc, x + size/2 - 4, y + 4, NULL);
    LineTo(hdc, x + size/2, y);
    LineTo(hdc, x + size/2 + 4, y + 4);
    
    SelectObject(hdc, oldPen);
    DeleteObject(iconPen);
}

// Helper function to draw lightning/zap icon
void DrawLightningIcon(HDC hdc, int x, int y, int size, COLORREF color) {
    HPEN iconPen = CreatePen(PS_SOLID, 2, color);
    HPEN oldPen = (HPEN)SelectObject(hdc, iconPen);
    
    // Lightning bolt zigzag
    MoveToEx(hdc, x + size/2 + 2, y, NULL);
    LineTo(hdc, x + size/2 - 4, y + size/2);
    LineTo(hdc, x + size/2, y + size/2);
    LineTo(hdc, x + size/2 - 2, y + size);
    
    SelectObject(hdc, oldPen);
    DeleteObject(iconPen);
}

// Helper function to draw folder icon
void DrawFolderIcon(HDC hdc, int x, int y, int size, COLORREF color) {
    HPEN iconPen = CreatePen(PS_SOLID, 2, color);
    HPEN oldPen = (HPEN)SelectObject(hdc, iconPen);
    HBRUSH nullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, nullBrush);
    
    // Folder body
    Rectangle(hdc, x, y + size/3, x + size, y + size);
    
    // Folder tab
    MoveToEx(hdc, x, y + size/3, NULL);
    LineTo(hdc, x, y + size/4);
    LineTo(hdc, x + size/2, y + size/4);
    LineTo(hdc, x + size/2, y + size/3);
    
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(iconPen);
}

// Helper function to draw history/clock icon
void DrawHistoryIcon(HDC hdc, int x, int y, int size, COLORREF color) {
    HPEN iconPen = CreatePen(PS_SOLID, 2, color);
    HPEN oldPen = (HPEN)SelectObject(hdc, iconPen);
    HBRUSH nullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, nullBrush);
    
    // Circle
    Ellipse(hdc, x, y, x + size, y + size);
    
    // Clock hands
    int centerX = x + size/2;
    int centerY = y + size/2;
    MoveToEx(hdc, centerX, centerY, NULL);
    LineTo(hdc, centerX, centerY - size/3);  // Hour hand up
    MoveToEx(hdc, centerX, centerY, NULL);
    LineTo(hdc, centerX + size/3, centerY);  // Minute hand right
    
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(iconPen);
}

// Design System - Professional Windows Desktop Colors
static const COLORREF COLOR_SLATE_50     = RGB(248, 250, 252);  // App canvas
static const COLORREF COLOR_SLATE_100    = RGB(241, 245, 249);  
static const COLORREF COLOR_SLATE_200    = RGB(226, 232, 240);  // Empty slot, borders
static const COLORREF COLOR_SLATE_300    = RGB(203, 213, 225);  // Filled slot border
static const COLORREF COLOR_SLATE_400    = RGB(148, 163, 184);  
static const COLORREF COLOR_SLATE_600    = RGB(71, 85, 105);    // Secondary text
static const COLORREF COLOR_SLATE_700    = RGB(51, 65, 85);     // Dark text
static const COLORREF COLOR_SLATE_900    = RGB(15, 23, 42);     // Header background
static const COLORREF COLOR_WHITE        = RGB(255, 255, 255);  // Sidebar, cards
static const COLORREF COLOR_SKY_600      = RGB(2, 132, 199);    // Avo Blue primary
static const COLORREF COLOR_SKY_500      = RGB(14, 165, 233);   // Sky hover
static const COLORREF COLOR_SKY_100      = RGB(224, 242, 254);  // Sky tint background
static const COLORREF COLOR_EMERALD_500  = RGB(16, 185, 129);   // Success green
static const COLORREF COLOR_EMERALD_400  = RGB(52, 211, 153);   // LED green glow
static const COLORREF COLOR_AMBER_500    = RGB(245, 158, 11);   // Processing/warning

// Layout Dimensions (Enterprise Desktop UI)
static const int HEADER_HEIGHT      = 64;   // Top bar with meta info
static const int STATUS_HEIGHT      = 80;   // Bottom progress pipeline
static const int CONTROL_PANEL_WIDTH = 320; // Right sidebar with command buttons

// Panel grid layout - 4 columns x 6 rows (24 slots)
static const int PANEL_COLS     = 4;
static const int PANEL_ROWS     = 6;
static const int SLOT_WIDTH     = 160;  // Larger tiles for desktop UI
static const int SLOT_HEIGHT    = 70;
static const int SLOT_H_GAP     = 12;
static const int SLOT_V_GAP     = 12;

// Global panel data to display
static Panel g_panel;

// Global status indicator handles
static HWND g_hStatusStep1 = NULL;
static HWND g_hStatusStep2 = NULL;
static HWND g_hStatusStep3 = NULL;

// Status state flags
static bool g_step1Complete = false;
static bool g_step2Complete = false;
static bool g_step3Complete = false;

// Window procedure callback
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;
        
        if (pDIS->CtlType == ODT_BUTTON) {
            HDC hdc = pDIS->hDC;
            RECT rect = pDIS->rcItem;
            int btnID = pDIS->CtlID;
            
            // Set high-quality text rendering mode
            SetBkMode(hdc, TRANSPARENT);
            
            // Determine button state and styling
            bool isEnabled = true;
            bool isPrimary = (btnID == ID_BTN_LOAD_CSV && g_panel.panelID.empty());
            bool isHover = (pDIS->itemState & ODS_SELECTED);
            
            // Check if button should be disabled
            if (btnID == ID_BTN_GENERATE_BARCODE || btnID == ID_BTN_OPEN_FOLDER) {
                isEnabled = !g_panel.panelID.empty();
            }
            
            COLORREF bgColor, borderColor, textColor, iconColor;
            
            if (!isEnabled) {
                // Disabled state - light gray
                bgColor = RGB(243, 244, 246);  // Gray-100
                borderColor = RGB(229, 231, 235);  // Gray-200
                textColor = RGB(209, 213, 219);  // Gray-300
                iconColor = RGB(209, 213, 219);
            } else if (isPrimary) {
                // Primary button - Blue (like "Load CSV" in your image)
                bgColor = RGB(59, 130, 246);  // Blue-500
                borderColor = RGB(59, 130, 246);
                textColor = COLOR_WHITE;
                iconColor = COLOR_WHITE;
            } else {
                // Secondary/default - White with border
                bgColor = COLOR_WHITE;
                borderColor = RGB(229, 231, 235);  // Gray-200
                textColor = RGB(55, 65, 81);  // Gray-700
                iconColor = RGB(107, 114, 128);  // Gray-500
            }
            
            // Draw rounded rectangle background
            HBRUSH bgBrush = CreateSolidBrush(bgColor);
            HPEN borderPen = CreatePen(PS_SOLID, 1, borderColor);
            DrawRoundedRect(hdc, rect, 8, borderPen, bgBrush);
            DeleteObject(borderPen);
            DeleteObject(bgBrush);
            
            // Icon container (40x40px rounded square on left side)
            int iconContainerSize = 40;
            int iconContainerX = rect.left + 20;
            int iconContainerY = rect.top + (rect.bottom - rect.top - iconContainerSize) / 2;
            
            // Draw icon container background
            COLORREF containerBg;
            if (isPrimary) {
                // Semi-transparent white for primary button (20% opacity simulation)
                containerBg = RGB(255, 255, 255);  // We'll use alpha blending effect
                HBRUSH containerBrush = CreateSolidBrush(RGB(96, 165, 250)); // Lighter blue
                HPEN containerPen = CreatePen(PS_NULL, 0, 0);
                HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, containerBrush);
                HPEN oldPen = (HPEN)SelectObject(hdc, containerPen);
                RoundRect(hdc, iconContainerX, iconContainerY, 
                         iconContainerX + iconContainerSize, 
                         iconContainerY + iconContainerSize, 8, 8);
                SelectObject(hdc, oldBrush);
                SelectObject(hdc, oldPen);
                DeleteObject(containerBrush);
                DeleteObject(containerPen);
            } else if (!isEnabled) {
                // Light gray container for disabled
                containerBg = RGB(243, 244, 246);
            } else {
                // Light gray container for secondary buttons
                HBRUSH containerBrush = CreateSolidBrush(RGB(226, 232, 240)); // Slate-200
                HPEN containerPen = CreatePen(PS_NULL, 0, 0);
                HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, containerBrush);
                HPEN oldPen = (HPEN)SelectObject(hdc, containerPen);
                RoundRect(hdc, iconContainerX, iconContainerY, 
                         iconContainerX + iconContainerSize, 
                         iconContainerY + iconContainerSize, 8, 8);
                SelectObject(hdc, oldBrush);
                SelectObject(hdc, oldPen);
                DeleteObject(containerBrush);
                DeleteObject(containerPen);
            }
            
            // Icon centered in container (24px icon in 40px container = 8px padding)
            int iconX = iconContainerX + 8;
            int iconY = iconContainerY + 8;
            
            // Draw icon based on button ID
            if (btnID == ID_BTN_LOAD_CSV) {
                DrawUploadIcon(hdc, iconX, iconY, 24, iconColor);
            } else if (btnID == ID_BTN_GENERATE_BARCODE) {
                DrawLightningIcon(hdc, iconX, iconY, 24, iconColor);
            } else if (btnID == ID_BTN_OPEN_FOLDER) {
                DrawFolderIcon(hdc, iconX, iconY, 24, iconColor);
            } else if (btnID == ID_BTN_VIEW_HISTORY) {
                DrawHistoryIcon(hdc, iconX, iconY, 24, iconColor);
            }
            
            // Text area (right of icon container with proper spacing)
            int textX = iconContainerX + iconContainerSize + 16;
            int textY = rect.top + 18;
            
            SetBkMode(hdc, TRANSPARENT);
            
            // Title (14pt Semi-Bold for hierarchy)
            HFONT hTitleFont = CreateFont(18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Segoe UI"));
            HFONT hOldFont = (HFONT)SelectObject(hdc, hTitleFont);
            
            SetTextColor(hdc, textColor);
            
            std::string title, subtitle;
            if (btnID == ID_BTN_LOAD_CSV) {
                title = "Load CSV";
                subtitle = "Step 1";
            } else if (btnID == ID_BTN_GENERATE_BARCODE) {
                title = "Generate Files";
                subtitle = "Step 2";
            } else if (btnID == ID_BTN_OPEN_FOLDER) {
                title = "Open Folder";
                subtitle = "";
            } else if (btnID == ID_BTN_VIEW_HISTORY) {
                title = "View History";
                subtitle = "";
            }
            
            TextOutA(hdc, textX, textY, title.c_str(), (int)title.length());
            
            // Subtitle (10pt Regular, muted color for hierarchy)
            if (!subtitle.empty()) {
                HFONT hSubFont = CreateFont(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Segoe UI"));
                SelectObject(hdc, hSubFont);
                
                COLORREF subtitleColor;
                if (!isEnabled) {
                    subtitleColor = RGB(209, 213, 219); // Gray-300
                } else if (isPrimary) {
                    subtitleColor = RGB(224, 242, 254); // Blue-100 (muted white on blue)
                } else {
                    subtitleColor = RGB(156, 163, 175); // Gray-400 (muted on white)
                }
                SetTextColor(hdc, subtitleColor);
                
                TextOutA(hdc, textX, textY + 24, subtitle.c_str(), (int)subtitle.length());
                DeleteObject(hSubFont);
            }
            
            SelectObject(hdc, hOldFont);
            DeleteObject(hTitleFont);
            
            return TRUE;
        }
        break;
    }
    case WM_COMMAND: {
        int controlId = LOWORD(wParam);
        switch (controlId) {
        case ID_BTN_LOAD_CSV: {
            // Show file open dialog
            OPENFILENAMEA ofn = {};
            char szFile[260] = {0};
            
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = "CSV Files\\0*.csv\\0All Files\\0*.*\\0";
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            std::string inputPanelsDir = getAbsolutePath(WolfTrackConfig::INPUT_PANELS_ROOT);
            ofn.lpstrInitialDir = inputPanelsDir.c_str();
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
            
            if (GetOpenFileNameA(&ofn)) {
                // Load the panel from CSV
                if (loadPanelFromCsvFile(szFile, g_panel)) {
                    // Update status flags
                    g_step1Complete = true;
                    g_step2Complete = true; // Laser files generated during load
                    g_step3Complete = true; // Panel archived and logged during load
                    
                    // Repaint entire window to show updated pipeline and panel data
                    InvalidateRect(hwnd, NULL, TRUE);
                } else {
                    MessageBoxA(hwnd, "Failed to load panel CSV. Please check the file format.", "Error", MB_OK | MB_ICONERROR);
                }
            }
            break;
        }
        case ID_BTN_GENERATE_BARCODE: {
            // Check if a panel is loaded
            if (g_panel.panelID.empty()) {
                MessageBoxA(hwnd, "No panel loaded. Please load a CSV first.", "WolfTrack", MB_OK | MB_ICONWARNING);
                break;
            }
            
            // STAGE 1 UPGRADE: Validate operator name
            if (g_currentOperator.empty()) {
                MessageBoxA(hwnd, "Operator name is missing. Please restart the application.", "Validation Error", MB_OK | MB_ICONERROR);
                break;
            }
            
            // STAGE 1 UPGRADE: Validate all 24 PCB serials are non-empty
            bool allPcbsValid = true;
            std::string invalidPcbs = "";
            for (size_t i = 0; i < g_panel.pcbSerials.size(); ++i) {
                if (g_panel.pcbSerials[i].empty()) {
                    allPcbsValid = false;
                    if (!invalidPcbs.empty()) invalidPcbs += ", ";
                    invalidPcbs += "PCB" + std::to_string(i + 1);
                }
            }
            
            if (!allPcbsValid) {
                std::string msg = "Cannot generate laser files. The following PCB fields are empty:\n\n" + invalidPcbs + "\n\nPlease load a complete CSV file.";
                MessageBoxA(hwnd, msg.c_str(), "Validation Error", MB_OK | MB_ICONERROR);
                break;
            }
            
            // STAGE 1 UPGRADE: Check if panel folder already exists
            std::string pendingRoot = getAbsolutePath(WolfTrackConfig::PENDING_ART_ROOT);
            fs::path panelFolder = fs::path(pendingRoot) / g_panel.panelID;
            
            if (fs::exists(panelFolder) && fs::is_directory(panelFolder)) {
                int result = MessageBoxA(hwnd, 
                    "Panel folder already exists. This will overwrite existing files.\n\nDo you want to continue?",
                    "Duplicate Panel Warning", 
                    MB_YESNO | MB_ICONWARNING);
                
                if (result == IDNO) {
                    break; // User cancelled
                }
            }
            
            std::string artPath = createPanelArtSvg(g_panel);
            std::string codePath = createPanelBarcodeSvgPlaceholder(g_panel);
            
            if (!artPath.empty() && !codePath.empty()) {
                // No popup - status indicators already updated during CSV load
            } else {
                std::string msg = "Failed to generate SVG files.\\n";
                if (artPath.empty()) msg += "Panel artwork failed.\\n";
                if (codePath.empty()) msg += "DataMatrix label failed.";
                MessageBoxA(hwnd, msg.c_str(), "Error", MB_OK | MB_ICONERROR);
            }
            break;
        }
        case ID_BTN_OPEN_FOLDER: {
            // Check if a panel is loaded
            if (g_panel.panelID.empty()) {
                MessageBoxA(hwnd, "No panel loaded. Please load a CSV first.", "WolfTrack", MB_OK | MB_ICONWARNING);
                break;
            }
            
            std::string folder = getPanelPendingFolder(g_panel);
            HINSTANCE result = ShellExecuteA(hwnd, "open", folder.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            if ((INT_PTR)result <= 32) {
                MessageBoxA(hwnd, "Failed to open folder", "Error", MB_OK | MB_ICONERROR);
            }
            break;
        }
        case ID_BTN_VIEW_HISTORY: {
            // Ensure the master CSV file exists before trying to open it
            ensureMasterCsvExists();
            
            // Open the master CSV history file using absolute path
            std::string histPath = getAbsolutePath("MasterData\\wolftrack_panels_master.csv");
            HINSTANCE result = ShellExecuteA(hwnd, "open", histPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            if ((INT_PTR)result <= 32) {
                MessageBoxA(hwnd, "Failed to open history file. Make sure it exists.", "Error", MB_OK | MB_ICONERROR);
            }
            break;
        }
        }
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        
        // Get client area dimensions
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        int clientWidth = clientRect.right - clientRect.left;
        int clientHeight = clientRect.bottom - clientRect.top;
        
        // === FILL APP CANVAS (Slate-50) ===
        HBRUSH hCanvasBrush = CreateSolidBrush(COLOR_SLATE_50);
        FillRect(hdc, &clientRect, hCanvasBrush);
        DeleteObject(hCanvasBrush);
        
        // === HEADER BAR (64px, Slate-900) ===
        RECT headerRect = {0, 0, clientWidth, HEADER_HEIGHT};
        HBRUSH hHeaderBrush = CreateSolidBrush(COLOR_SLATE_900);
        FillRect(hdc, &headerRect, hHeaderBrush);
        DeleteObject(hHeaderBrush);
        
        SetBkMode(hdc, TRANSPARENT);
        
        // App Title (Left side)
        HFONT hTitleFont = CreateFont(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Segoe UI"));
        HFONT hOldFont = (HFONT)SelectObject(hdc, hTitleFont);
        SetTextColor(hdc, COLOR_WHITE);
        TextOutA(hdc, 24, 20, "WolfTrack Panel Station", 23);
        
        // Meta info on right (Panel ID, Operator, Time) - Monospace for technical look
        HFONT hMonoFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, TEXT("Consolas"));
        SelectObject(hdc, hMonoFont);
        SetTextColor(hdc, RGB(148, 163, 184)); // Slate-400
        
        int metaX = clientWidth - 580;
        
        // Panel ID
        if (!g_panel.panelID.empty()) {
            std::string panelLabel = "Panel: ";
            TextOutA(hdc, metaX, 16, panelLabel.c_str(), (int)panelLabel.length());
            SetTextColor(hdc, COLOR_WHITE);
            TextOutA(hdc, metaX, 32, g_panel.panelID.c_str(), (int)g_panel.panelID.length());
            
            // Vertical divider
            HPEN hDividerPen = CreatePen(PS_SOLID, 1, RGB(71, 85, 105)); // Slate-600
            HPEN hOldPen = (HPEN)SelectObject(hdc, hDividerPen);
            MoveToEx(hdc, metaX + 150, 16, NULL);
            LineTo(hdc, metaX + 150, 48);
            SelectObject(hdc, hOldPen);
            DeleteObject(hDividerPen);
        }
        
        // Operator
        if (!g_currentOperator.empty()) {
            SetTextColor(hdc, RGB(148, 163, 184));
            std::string opLabel = "Operator: ";
            TextOutA(hdc, metaX + 170, 16, opLabel.c_str(), (int)opLabel.length());
            SetTextColor(hdc, COLOR_WHITE);
            TextOutA(hdc, metaX + 170, 32, g_currentOperator.c_str(), (int)g_currentOperator.length());
            
            // Vertical divider
            HPEN hDividerPen = CreatePen(PS_SOLID, 1, RGB(71, 85, 105));
            HPEN hOldPen = (HPEN)SelectObject(hdc, hDividerPen);
            MoveToEx(hdc, metaX + 320, 16, NULL);
            LineTo(hdc, metaX + 320, 48);
            SelectObject(hdc, hOldPen);
            DeleteObject(hDividerPen);
        }
        
        // Time (placeholder for now)
        SetTextColor(hdc, RGB(148, 163, 184));
        std::string timeLabel = "Time: ";
        TextOutA(hdc, metaX + 340, 16, timeLabel.c_str(), (int)timeLabel.length());
        SetTextColor(hdc, COLOR_WHITE);
        
        // Get current time
        SYSTEMTIME st;
        GetLocalTime(&st);
        char timeStr[32];
        sprintf_s(timeStr, "%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
        TextOutA(hdc, metaX + 340, 32, timeStr, (int)strlen(timeStr));
        
        DeleteObject(hMonoFont);
        DeleteObject(hTitleFont);
        
        // === RIGHT CONTROL PANEL (320px, White sidebar) ===
        int sidebarX = clientWidth - CONTROL_PANEL_WIDTH;
        RECT sidebarRect = {sidebarX, HEADER_HEIGHT, clientWidth, clientHeight - STATUS_HEIGHT};
        HBRUSH hSidebarBrush = CreateSolidBrush(COLOR_WHITE);
        FillRect(hdc, &sidebarRect, hSidebarBrush);
        DeleteObject(hSidebarBrush);
        
        // Left border for sidebar (Slate-200)
        HPEN hBorderPen = CreatePen(PS_SOLID, 1, COLOR_SLATE_200);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hBorderPen);
        MoveToEx(hdc, sidebarX, HEADER_HEIGHT, NULL);
        LineTo(hdc, sidebarX, clientHeight - STATUS_HEIGHT);
        SelectObject(hdc, hOldPen);
        DeleteObject(hBorderPen);
        
        // Sidebar title
        HFONT hSidebarTitleFont = CreateFont(14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Segoe UI"));
        SelectObject(hdc, hSidebarTitleFont);
        SetTextColor(hdc, COLOR_SLATE_700);
        TextOutA(hdc, sidebarX + 20, HEADER_HEIGHT + 20, "Actions", 7);
        DeleteObject(hSidebarTitleFont);
        
        // === MAIN CONTENT AREA (PCB Grid) ===
        int contentWidth = sidebarX - 40;
        int gridWidth = PANEL_COLS * (SLOT_WIDTH + SLOT_H_GAP) - SLOT_H_GAP;
        int gridHeight = PANEL_ROWS * (SLOT_HEIGHT + SLOT_V_GAP) - SLOT_V_GAP;
        int gridX = 20 + (contentWidth - gridWidth) / 2;  // Center grid
        int gridY = HEADER_HEIGHT + 40;
        
        if (g_panel.panelID.empty()) {
            // === EMPTY STATE ===
            HFONT hEmptyFont = CreateFont(24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Segoe UI"));
            SelectObject(hdc, hEmptyFont);
            SetTextColor(hdc, COLOR_SLATE_600);
            
            std::string text = "No panel loaded";
            SIZE textSize;
            GetTextExtentPoint32A(hdc, text.c_str(), (int)text.length(), &textSize);
            TextOutA(hdc, (contentWidth - textSize.cx) / 2, gridY + 100, text.c_str(), (int)text.length());
            
            HFONT hHintFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Segoe UI"));
            SelectObject(hdc, hHintFont);
            SetTextColor(hdc, COLOR_SLATE_400);
            text = "Click 'Load Panel CSV' to begin";
            GetTextExtentPoint32A(hdc, text.c_str(), (int)text.length(), &textSize);
            TextOutA(hdc, (contentWidth - textSize.cx) / 2, gridY + 135, text.c_str(), (int)text.length());
            
            DeleteObject(hHintFont);
            DeleteObject(hEmptyFont);
        } else {
            // === PCB GRID (4x6 = 24 slots) ===
            HFONT hPosFont = CreateFont(10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Segoe UI"));
            HFONT hSerialFont = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, TEXT("Consolas"));
            
            for (int i = 0; i < 24; ++i) {
                int row = i / PANEL_COLS;
                int col = i % PANEL_COLS;
                
                int left   = gridX + col * (SLOT_WIDTH + SLOT_H_GAP);
                int top    = gridY + row * (SLOT_HEIGHT + SLOT_V_GAP);
                int right  = left + SLOT_WIDTH;
                int bottom = top + SLOT_HEIGHT;
                
                RECT slotRect = {left, top, right, bottom};
                
                bool isEmpty = g_panel.pcbSerials[i].empty();
                
                if (isEmpty) {
                    // EMPTY SLOT: Recessed, disabled look with inner shadow effect
                    HBRUSH hEmptyBrush = CreateSolidBrush(COLOR_SLATE_200);
                    FillRect(hdc, &slotRect, hEmptyBrush);
                    DeleteObject(hEmptyBrush);
                    
                    // Subtle inner shadow (dark top-left edge)
                    HPEN hShadowPen = CreatePen(PS_SOLID, 1, RGB(203, 213, 225)); // Darker shade
                    SelectObject(hdc, hShadowPen);
                    MoveToEx(hdc, left, bottom - 1, NULL);
                    LineTo(hdc, left, top);
                    LineTo(hdc, right - 1, top);
                    DeleteObject(hShadowPen);
                    
                    // Position label (faded)
                    SelectObject(hdc, hPosFont);
                    SetTextColor(hdc, COLOR_SLATE_400);
                    std::string posText = "POS " + std::to_string(i + 1);
                    TextOutA(hdc, left + 6, top + 4, posText.c_str(), (int)posText.length());
                    
                    // Empty text
                    HFONT hEmptyTextFont = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Segoe UI"));
                    SelectObject(hdc, hEmptyTextFont);
                    SetTextColor(hdc, COLOR_SLATE_400);
                    std::string emptyText = "Empty";
                    SIZE emptySize;
                    GetTextExtentPoint32A(hdc, emptyText.c_str(), (int)emptyText.length(), &emptySize);
                    TextOutA(hdc, left + (SLOT_WIDTH - emptySize.cx) / 2, top + (SLOT_HEIGHT - emptySize.cy) / 2, 
                             emptyText.c_str(), (int)emptyText.length());
                    DeleteObject(hEmptyTextFont);
                } else {
                    // FILLED SLOT: Raised, active look with drop shadow
                    // Drop shadow (offset by 2px)
                    RECT shadowRect = {left + 2, top + 2, right + 2, bottom + 2};
                    HBRUSH hShadowBrush = CreateSolidBrush(RGB(203, 213, 225)); // Light shadow
                    FillRect(hdc, &shadowRect, hShadowBrush);
                    DeleteObject(hShadowBrush);
                    
                    // White card
                    HBRUSH hCardBrush = CreateSolidBrush(COLOR_WHITE);
                    FillRect(hdc, &slotRect, hCardBrush);
                    DeleteObject(hCardBrush);
                    
                    // Border (Slate-300)
                    HPEN hCardPen = CreatePen(PS_SOLID, 1, COLOR_SLATE_300);
                    HPEN hOldPen = (HPEN)SelectObject(hdc, hCardPen);
                    HBRUSH hNullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
                    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hNullBrush);
                    Rectangle(hdc, left, top, right, bottom);
                    SelectObject(hdc, hOldBrush);
                    SelectObject(hdc, hOldPen);
                    DeleteObject(hCardPen);
                    
                    // Position label (tiny, uppercase)
                    SelectObject(hdc, hPosFont);
                    SetTextColor(hdc, COLOR_SLATE_400);
                    std::string posText = "POS " + std::to_string(i + 1);
                    TextOutA(hdc, left + 6, top + 4, posText.c_str(), (int)posText.length());
                    
                    // Serial number (large, bold, monospace, centered)
                    SelectObject(hdc, hSerialFont);
                    SetTextColor(hdc, COLOR_SLATE_900);
                    std::string serial = g_panel.pcbSerials[i].length() > 12 
                        ? g_panel.pcbSerials[i].substr(0, 12)
                        : g_panel.pcbSerials[i];
                    SIZE serialSize;
                    GetTextExtentPoint32A(hdc, serial.c_str(), (int)serial.length(), &serialSize);
                    TextOutA(hdc, left + (SLOT_WIDTH - serialSize.cx) / 2, 
                             top + (SLOT_HEIGHT - serialSize.cy) / 2 + 4,
                             serial.c_str(), (int)serial.length());
                    
                    // Green LED indicator (top-right corner)
                    HBRUSH hLEDBrush = CreateSolidBrush(COLOR_EMERALD_400);
                    HPEN hLEDPen = CreatePen(PS_SOLID, 1, COLOR_EMERALD_500);
                    SelectObject(hdc, hLEDBrush);
                    SelectObject(hdc, hLEDPen);
                    Ellipse(hdc, right - 14, top + 4, right - 6, top + 12);
                    DeleteObject(hLEDPen);
                    DeleteObject(hLEDBrush);
                }
            }
            
            DeleteObject(hSerialFont);
            DeleteObject(hPosFont);
        }
        

        SelectObject(hdc, hOldFont);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void runPanelViewerGui(const Panel& panel) {
    // Store panel data globally for the window procedure
    g_panel = panel;
    // Get the instance handle for the current process
    HINSTANCE hInstance = GetModuleHandle(NULL);

    // Define and register window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = TEXT("WolfTrackPanelViewerClass");

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, TEXT("Window class registration failed"), TEXT("Error"), MB_ICONERROR);
        return;
    }

    // Calculate required client area size
    // Professional desktop layout: content area + right control panel (320px)
    int clientWidth = 1100;   // Wide enough for 4-column grid + sidebar
    int clientHeight = 750;   // Taller for 6-row grid
    
    // Calculate window size including borders and title bar
    RECT windowRect = {0, 0, clientWidth, clientHeight};
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
    
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;
    
    // Create the window
    HWND hwnd = CreateWindowEx(
        0,                                      // Extended styles
        TEXT("WolfTrackPanelViewerClass"),     // Class name
        TEXT("AVO Invents Ltd – WolfTrack Panel Viewer"),  // Window title
        WS_OVERLAPPEDWINDOW,                   // Window style
        CW_USEDEFAULT, CW_USEDEFAULT,          // X, Y position
        windowWidth, windowHeight,              // Width, Height (adjusted for borders)
        NULL,                                   // Parent window
        NULL,                                   // Menu
        hInstance,                              // Instance handle
        NULL                                    // Additional application data
    );

    if (hwnd == NULL) {
        MessageBox(NULL, TEXT("Window creation failed"), TEXT("Error"), MB_ICONERROR);
        return;
    }

    // Create large Command Buttons in right control panel (76px height each) - Owner-drawn for custom styling
    int sidebarX = clientWidth - CONTROL_PANEL_WIDTH;
    int btnY = HEADER_HEIGHT + 60;  // Start below "Actions" title
    int btnHeight = 76;
    int btnSpacing = 16;
    
    HWND btnLoadCsv = CreateWindowA(
        "BUTTON",
        "",  // Text drawn in WM_DRAWITEM
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        sidebarX + 16, btnY, CONTROL_PANEL_WIDTH - 32, btnHeight,
        hwnd,
        (HMENU)ID_BTN_LOAD_CSV,
        hInstance,
        NULL
    );
    btnY += btnHeight + btnSpacing;
    
    // "Simulate: Load Demo Data" link text below Load CSV button
    HWND lblSimulate = CreateWindowA(
        "STATIC",
        "(Simulate: Load Demo Data)",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        sidebarX + 16, btnY - 12, CONTROL_PANEL_WIDTH - 32, 16,
        hwnd,
        NULL,
        hInstance,
        NULL
    );
    HFONT hLinkFont = CreateFont(11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Segoe UI"));
    SendMessage(lblSimulate, WM_SETFONT, (WPARAM)hLinkFont, TRUE);
    btnY += 8;
    
    HWND btnBarcode = CreateWindowA(
        "BUTTON",
        "",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        sidebarX + 16, btnY, CONTROL_PANEL_WIDTH - 32, btnHeight,
        hwnd,
        (HMENU)ID_BTN_GENERATE_BARCODE,
        hInstance,
        NULL
    );
    btnY += btnHeight + btnSpacing;

    HWND btnFolder = CreateWindowA(
        "BUTTON",
        "",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        sidebarX + 16, btnY, CONTROL_PANEL_WIDTH - 32, btnHeight,
        hwnd,
        (HMENU)ID_BTN_OPEN_FOLDER,
        hInstance,
        NULL
    );
    btnY += btnHeight + btnSpacing;
    
    HWND btnHistory = CreateWindowA(
        "BUTTON",
        "",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        sidebarX + 16, btnY, CONTROL_PANEL_WIDTH - 32, btnHeight,
        hwnd,
        (HMENU)ID_BTN_VIEW_HISTORY,
        hInstance,
        NULL
    );

    // Show and update the window
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // Run the message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
