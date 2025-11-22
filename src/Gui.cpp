#include "Gui.h"
#include <windows.h>
#include <string>
#include <sstream>
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")

// Button control IDs
#define ID_BTN_GENERATE_BARCODE  1001
#define ID_BTN_OPEN_FOLDER       1002
#define ID_BTN_MARK_LASERED      1003

// Panel layout constants
static const int PANEL_ORIGIN_X = 20;
static const int PANEL_ORIGIN_Y = 200;
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
        case ID_BTN_GENERATE_BARCODE:
            MessageBoxA(hwnd, "Generate Barcode clicked", "WolfTrack", MB_OK);
            break;
        case ID_BTN_OPEN_FOLDER:
            MessageBoxA(hwnd, "Open Panel Folder clicked", "WolfTrack", MB_OK);
            break;
        case ID_BTN_MARK_LASERED:
            MessageBoxA(hwnd, "Mark As Lasered clicked", "WolfTrack", MB_OK);
            break;
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

    // Create the window
    HWND hwnd = CreateWindowEx(
        0,                                      // Extended styles
        TEXT("WolfTrackPanelViewerClass"),     // Class name
        TEXT("WolfTrack Panel Viewer"),        // Window title
        WS_OVERLAPPEDWINDOW,                   // Window style
        CW_USEDEFAULT, CW_USEDEFAULT,          // X, Y position
        800, 600,                               // Width, Height
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
    HWND btnBarcode = CreateWindowA(
        "BUTTON",
        "Generate Barcode",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_TEXT,
        30, 510, 200, 35,
        hwnd,
        (HMENU)ID_BTN_GENERATE_BARCODE,
        hInstance,
        NULL
    );

    HWND btnFolder = CreateWindowA(
        "BUTTON",
        "Open Panel Folder",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_TEXT,
        250, 510, 200, 35,
        hwnd,
        (HMENU)ID_BTN_OPEN_FOLDER,
        hInstance,
        NULL
    );

    HWND btnLasered = CreateWindowA(
        "BUTTON",
        "Mark As Lasered",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_TEXT,
        470, 510, 200, 35,
        hwnd,
        (HMENU)ID_BTN_MARK_LASERED,
        hInstance,
        NULL
    );
    
    // Set button fonts
    HFONT hButtonFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Segoe UI"));
    SendMessage(btnBarcode, WM_SETFONT, (WPARAM)hButtonFont, TRUE);
    SendMessage(btnFolder, WM_SETFONT, (WPARAM)hButtonFont, TRUE);
    SendMessage(btnLasered, WM_SETFONT, (WPARAM)hButtonFont, TRUE);

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
