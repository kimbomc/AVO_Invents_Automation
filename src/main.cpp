#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <filesystem>
#include <windows.h>
#include <commctrl.h>
#include "Panel.h"
#include "MasterData.h"
#include "Gui.h"
#include "SessionState.h"
#include "Config.h"

namespace fs = std::filesystem;

// Force Windows subsystem to prevent console window
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

// Global variables for the operator name dialog
static char g_operatorNameBuffer[256] = "";
static HWND g_hEditOperatorName = NULL;
static HWND g_hDialogWindow = NULL;
static bool g_dialogClosed = false;

// STAGE 1 UPGRADE: Helper functions for operator persistence
static std::string getExeDirectory() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string exePath(buffer);
    size_t pos = exePath.find_last_of("\\/");
    return (pos != std::string::npos) ? exePath.substr(0, pos) : ".";
}

static std::string loadOperatorFromSettings() {
    std::string settingsPath = getExeDirectory() + "\\settings.ini";
    std::ifstream file(settingsPath);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("operator=") == 0) {
                return line.substr(9); // Skip "operator="
            }
        }
    }
    return "";
}

static void saveOperatorToSettings(const std::string& operatorName) {
    std::string settingsPath = getExeDirectory() + "\\settings.ini";
    std::ofstream file(settingsPath);
    if (file.is_open()) {
        file << "operator=" << operatorName << "\n";
        file.close();
    }
}

// Window procedure for the operator name dialog
LRESULT CALLBACK OperatorDialogWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        // STAGE 1 UPGRADE: Load saved operator name if available
        std::string savedOperator = loadOperatorFromSettings();
        if (!savedOperator.empty()) {
            strncpy_s(g_operatorNameBuffer, savedOperator.c_str(), 255);
        }
        
        // Create static text label
        HWND hLabel = CreateWindowExA(
            0,
            "STATIC",
            "Please enter your name:",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 20, 360, 20,
            hwnd,
            NULL,
            GetModuleHandle(NULL),
            NULL
        );
        
        // Create the edit control
        g_hEditOperatorName = CreateWindowExA(
            WS_EX_CLIENTEDGE,
            "EDIT",
            g_operatorNameBuffer, // Pre-fill with saved name
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            20, 50, 360, 30,
            hwnd,
            (HMENU)101,
            GetModuleHandle(NULL),
            NULL
        );
        
        // Create OK button
        HWND hButton = CreateWindowExA(
            0,
            "BUTTON",
            "OK",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            160, 95, 100, 35,
            hwnd,
            (HMENU)IDOK,
            GetModuleHandle(NULL),
            NULL
        );
        
        // Set font for all controls
        HFONT hFont = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Segoe UI"));
        
        SendMessage(hLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(g_hEditOperatorName, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        SetFocus(g_hEditOperatorName);
        return 0;
    }
        
    case WM_COMMAND: {
        if (LOWORD(wParam) == IDOK) {
            // Get the text from the edit control
            GetWindowTextA(g_hEditOperatorName, g_operatorNameBuffer, 256);
            
            // Validate that name is not empty
            if (strlen(g_operatorNameBuffer) == 0) {
                MessageBoxA(hwnd, "Please enter your name.", "Error", MB_OK | MB_ICONERROR);
                SetFocus(g_hEditOperatorName);
                return 0;
            }
            
            g_dialogClosed = true;
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    }
        
    case WM_CLOSE: {
        // Don't allow closing without entering a name
        MessageBoxA(hwnd, "You must enter an operator name to continue.", "Required", MB_OK | MB_ICONWARNING);
        return 0;
    }
    
    case WM_DESTROY: {
        g_hDialogWindow = NULL;
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void showOperatorNameDialog() {
    g_dialogClosed = false;
    
    // Register window class for the dialog
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = OperatorDialogWindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = TEXT("OperatorNameDialog");
    RegisterClassEx(&wc);
    
    // Create the dialog window
    g_hDialogWindow = CreateWindowExA(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        "OperatorNameDialog",
        "AVO Invents - Enter Operator Name",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        0, 0, 420, 180,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    if (g_hDialogWindow) {
        // Center and show the dialog
        RECT rc;
        GetWindowRect(g_hDialogWindow, &rc);
        int xPos = (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2;
        int yPos = (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2;
        SetWindowPos(g_hDialogWindow, HWND_TOPMOST, xPos, yPos, 0, 0, SWP_NOSIZE);
        
        ShowWindow(g_hDialogWindow, SW_SHOW);
        UpdateWindow(g_hDialogWindow);
        
        // Message loop for the dialog
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    g_currentOperator = std::string(g_operatorNameBuffer);
    
    // STAGE 1 UPGRADE: Save operator name to settings.ini
    saveOperatorToSettings(g_currentOperator);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Explicitly free any console that might have been allocated
    FreeConsole();
    
    // STAGE 1 UPGRADE: Ensure folder structure exists at startup
    std::string exeDir = getExeDirectory();
    fs::create_directories(fs::path(exeDir) / WolfTrackConfig::INPUT_PANELS_ROOT);
    fs::create_directories(fs::path(exeDir) / WolfTrackConfig::INPUT_PANELS_ARCHIVE);
    fs::create_directories(fs::path(exeDir) / WolfTrackConfig::PENDING_ART_ROOT);
    fs::create_directories(fs::path(exeDir) / WolfTrackConfig::COMPLETED_ART_ROOT);
    fs::create_directories(fs::path(exeDir) / "MasterData");
    
    // Show GUI dialog for operator name
    showOperatorNameDialog();

    // Initialize empty panel
    Panel p;
    p.panelID = "";
    p.panelNumber = "";
    p.status = PanelStatus::Detected;
    p.createdAt = "";
    p.laseredAt = "";
    p.sourceFile = "";

    // Show GUI with no panel loaded
    runPanelViewerGui(p);

    return 0;
}
