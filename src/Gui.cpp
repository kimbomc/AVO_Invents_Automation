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

// Panel layout constants
static const int PANEL_ORIGIN_X = 20;
static const int PANEL_ORIGIN_Y = 250;
static const int PANEL_COLS     = 6;
static const int PANEL_ROWS     = 4;
static const int SLOT_WIDTH     = 120;
static const int SLOT_HEIGHT    = 60;
static const int SLOT_H_GAP     = 10;
static const int SLOT_V_GAP     = 10;

// Global panel data to display
static Panel g_panel;

// Window procedure callback
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
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
                    InvalidateRect(hwnd, NULL, TRUE);
                    MessageBoxA(hwnd, "Panel CSV loaded successfully!", "WolfTrack", MB_OK | MB_ICONINFORMATION);
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
            std::string panelFolder = getPanelPendingFolder(g_panel);
            if (fs::exists(panelFolder)) {
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
                std::string msg = "Generated:\\n\\n" + artPath + "\\n\\n" + codePath;
                MessageBoxA(hwnd, msg.c_str(), "WolfTrack - SVG Files Created", MB_OK | MB_ICONINFORMATION);
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
        
        // Set up text formatting with nicer font
        SetBkMode(hdc, TRANSPARENT);
        HFONT hFont = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Segoe UI"));
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        
        // Set text color to dark gray
        SetTextColor(hdc, RGB(40, 40, 40));
        
        // Check if no panel is loaded
        if (g_panel.panelID.empty()) {
            HFONT hBigFont = CreateFont(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Segoe UI"));
            SelectObject(hdc, hBigFont);
            SetTextColor(hdc, RGB(100, 100, 100));
            
            std::string text = "No panel loaded";
            TextOutA(hdc, 20, 100, text.c_str(), (int)text.length());
            
            SelectObject(hdc, hFont);
            SetTextColor(hdc, RGB(120, 120, 120));
            text = "Click 'Load Panel CSV' to begin";
            TextOutA(hdc, 20, 140, text.c_str(), (int)text.length());
            
            // Draw footer with master statistics even when no panel loaded
            MasterStats stats = computeMasterStats();
            HFONT hFooterFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Segoe UI"));
            SelectObject(hdc, hFooterFont);
            SetTextColor(hdc, RGB(80, 80, 80));
            
            std::ostringstream footerText;
            footerText << "Total Panels: " << stats.totalPanels 
                       << "   Total PCBs: " << stats.totalPcbs;
            if (!stats.lastPanelID.empty()) {
                footerText << "   Last Panel: " << stats.lastPanelID;
            }
            
            std::string footer = footerText.str();
            TextOutA(hdc, 20, 540, footer.c_str(), (int)footer.length());
            
            DeleteObject(hFooterFont);
            DeleteObject(hBigFont);
            SelectObject(hdc, hOldFont);
            DeleteObject(hFont);
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        int y = 20;
        int lineHeight = 28;
        
        // Panel ID
        std::string text = "Panel ID: " + g_panel.panelID;
        TextOutA(hdc, 20, y, text.c_str(), (int)text.length());
        y += lineHeight;
        
        // Panel Number
        text = "Panel Number: " + g_panel.panelNumber;
        TextOutA(hdc, 20, y, text.c_str(), (int)text.length());
        y += lineHeight;
        
        // Operator
        text = "Operator: " + g_currentOperator;
        TextOutA(hdc, 20, y, text.c_str(), (int)text.length());
        y += lineHeight;
        
        // PCB1
        text = "PCB1: " + g_panel.pcbSerials[0];
        TextOutA(hdc, 20, y, text.c_str(), (int)text.length());
        y += lineHeight;
        
        // PCB24
        text = "PCB24: " + g_panel.pcbSerials[23];
        TextOutA(hdc, 20, y, text.c_str(), (int)text.length());
        y += lineHeight;
        
        // Status
        text = "Status: " + panelStatusToString(g_panel.status);
        TextOutA(hdc, 20, y, text.c_str(), (int)text.length());
        y += lineHeight;
        
        // Created At
        text = "Created At: " + g_panel.createdAt;
        TextOutA(hdc, 20, y, text.c_str(), (int)text.length());
        
        // Draw 24 PCB slots in a 6x4 grid
        HPEN hPen = CreatePen(PS_SOLID, 2, RGB(100, 100, 100));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        
        for (int i = 0; i < 24; ++i) {
            int row = i / PANEL_COLS;
            int col = i % PANEL_COLS;
            
            int left   = PANEL_ORIGIN_X + col * (SLOT_WIDTH + SLOT_H_GAP);
            int top    = PANEL_ORIGIN_Y + row * (SLOT_HEIGHT + SLOT_V_GAP);
            int right  = left + SLOT_WIDTH;
            int bottom = top + SLOT_HEIGHT;
            
            // Draw rectangle for PCB slot
            Rectangle(hdc, left, top, right, bottom);
            
            // Draw PCB label and short serial
            std::string pcbLabel = "PCB" + std::to_string(i + 1);
            std::string shortSerial = g_panel.pcbSerials[i].substr(0, 8);
            std::string slotText = pcbLabel + ": " + shortSerial;
            
            TextOutA(hdc, left + 4, top + 4, slotText.c_str(), (int)slotText.length());
        }
        
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
        
        // Draw footer with master statistics
        MasterStats stats = computeMasterStats();
        HFONT hFooterFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Segoe UI"));
        SelectObject(hdc, hFooterFont);
        SetTextColor(hdc, RGB(80, 80, 80));
        
        std::ostringstream footerText;
        footerText << "Total Panels: " << stats.totalPanels 
                   << "   Total PCBs: " << stats.totalPcbs;
        if (!stats.lastPanelID.empty()) {
            footerText << "   Last Panel: " << stats.lastPanelID;
        }
        
        std::string footer = footerText.str();
        TextOutA(hdc, 20, 540, footer.c_str(), (int)footer.length());
        
        DeleteObject(hFooterFont);
        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
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
    // Grid: 6 cols * (120 width + 10 gap) = 780, plus 20 left margin + 20 right margin = 820
    // Grid: 4 rows * (60 height + 10 gap) = 280, plus 250 top + 60 bottom margin + 50 button area = 640
    int clientWidth = 820;
    int clientHeight = 650;
    
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

    // Create button controls with modern styling
    HWND btnLoadCsv = CreateWindowA(
        "BUTTON",
        "Load Panel CSV",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_TEXT,
        30, 570, 150, 35,
        hwnd,
        (HMENU)ID_BTN_LOAD_CSV,
        hInstance,
        NULL
    );
    
    HWND btnBarcode = CreateWindowA(
        "BUTTON",
        "Generate Laser Files",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_TEXT,
        190, 570, 160, 35,
        hwnd,
        (HMENU)ID_BTN_GENERATE_BARCODE,
        hInstance,
        NULL
    );

    HWND btnFolder = CreateWindowA(
        "BUTTON",
        "Open Panel Folder",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_TEXT,
        360, 570, 150, 35,
        hwnd,
        (HMENU)ID_BTN_OPEN_FOLDER,
        hInstance,
        NULL
    );
    
    HWND btnHistory = CreateWindowA(
        "BUTTON",
        "View History",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_TEXT,
        520, 570, 130, 35,
        hwnd,
        (HMENU)ID_BTN_VIEW_HISTORY,
        hInstance,
        NULL
    );
    
    // Set button fonts
    HFONT hButtonFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Segoe UI"));
    SendMessage(btnLoadCsv, WM_SETFONT, (WPARAM)hButtonFont, TRUE);
    SendMessage(btnBarcode, WM_SETFONT, (WPARAM)hButtonFont, TRUE);
    SendMessage(btnFolder, WM_SETFONT, (WPARAM)hButtonFont, TRUE);
    SendMessage(btnHistory, WM_SETFONT, (WPARAM)hButtonFont, TRUE);

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
