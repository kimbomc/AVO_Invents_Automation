#include "MasterData.h"
#include "Config.h"
#include "SessionState.h"
#include <fstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <windows.h>

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

static std::string currentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    tm = *std::localtime(&t);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void ensureMasterCsvExists() {
    // Get absolute paths
    std::string masterDir = getAbsolutePath("MasterData");
    std::string masterCsvPath = getAbsolutePath(MASTER_CSV_PATH);
    
    // Create MasterData directory if it doesn't exist
    fs::create_directories(masterDir);
    
    if (!fs::exists(masterCsvPath)) {
        std::ofstream out(masterCsvPath, std::ios::out | std::ios::binary);
        // Write UTF-8 BOM to help Excel recognize encoding
        out << "\xEF\xBB\xBF";
        out << "PanelID,PanelNumber";
        for (int i = 1; i <= 24; ++i) {
            out << ",PCB" << i;
        }
        out << ",Operator,CreatedAt,Status,SourceFile\n";
        out.close();
    }
}

std::string generateNextPanelID() {
    ensureMasterCsvExists();

    std::string masterCsvPath = getAbsolutePath(MASTER_CSV_PATH);
    std::ifstream in(masterCsvPath);
    if (!in.is_open()) {
        return "WT-P-00001";
    }

    std::string line;
    int maxNum = 0;
    bool foundHeader = false;
    
    while (std::getline(in, line)) {
        if (!foundHeader) {
            foundHeader = true;
            continue; // Skip header row
        }
        
        if (line.empty()) {
            continue;
        }
        
        // Extract first field (PanelID)
        size_t commaPos = line.find(',');
        if (commaPos == std::string::npos) {
            continue;
        }
        
        std::string panelID = line.substr(0, commaPos);
        
        // Expected format: "WT-P-XXXXX"
        if (panelID.size() >= 6 && panelID.substr(0, 5) == "WT-P-") {
            std::string numStr = panelID.substr(5);
            try {
                int num = std::stoi(numStr);
                if (num > maxNum) {
                    maxNum = num;
                }
            } catch (...) {
                // Skip invalid IDs
            }
        }
    }
    in.close();

    // Increment and format
    maxNum++;
    std::ostringstream oss;
    oss << "WT-P-" << std::setfill('0') << std::setw(5) << maxNum;
    return oss.str();
}

MasterStats computeMasterStats() {
    MasterStats stats;
    stats.totalPanels = 0;
    stats.totalPcbs = 0;
    stats.lastPanelID = "";
    
    ensureMasterCsvExists();
    
    std::string masterCsvPath = getAbsolutePath(MASTER_CSV_PATH);
    std::ifstream in(masterCsvPath);
    if (!in.is_open()) {
        return stats;
    }
    
    std::string line;
    bool foundHeader = false;
    int maxNum = 0;
    
    while (std::getline(in, line)) {
        if (!foundHeader) {
            foundHeader = true;
            continue; // Skip header row
        }
        
        if (line.empty()) {
            continue;
        }
        
        stats.totalPanels++;
        
        // Extract PanelID (first field)
        size_t commaPos = line.find(',');
        if (commaPos != std::string::npos) {
            std::string panelID = line.substr(0, commaPos);
            
            // Track the highest numbered panel ID
            if (panelID.size() >= 6 && panelID.substr(0, 5) == "WT-P-") {
                std::string numStr = panelID.substr(5);
                try {
                    int num = std::stoi(numStr);
                    if (num > maxNum) {
                        maxNum = num;
                        stats.lastPanelID = panelID;
                    }
                } catch (...) {
                    // Skip invalid IDs
                }
            }
        }
    }
    in.close();
    
    stats.totalPcbs = stats.totalPanels * 24;
    
    return stats;
}

void appendPanelToMaster(const Panel& p) {
    ensureMasterCsvExists();

    std::string masterCsvPath = getAbsolutePath(MASTER_CSV_PATH);
    
    // Temporarily remove read-only attribute if it exists
    DWORD attrs = GetFileAttributesA(masterCsvPath.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_READONLY)) {
        SetFileAttributesA(masterCsvPath.c_str(), attrs & ~FILE_ATTRIBUTE_READONLY);
    }

    std::ofstream out(masterCsvPath, std::ios::app);
    if (!out.is_open()) {
        return; // in v1 we silently fail; can add error handling later
    }

    // Base fields - use Excel formula syntax ="value" to prevent leading zero removal
    out << "=\"" << p.panelID << "\",=\"" << p.panelNumber << "\"";

    // 24 PCB serials - use formula syntax to preserve leading zeros
    for (size_t i = 0; i < p.pcbSerials.size(); ++i) {
        out << ",=\"" << p.pcbSerials[i] << "\"";
    }

    // Operator, timestamp and status
    std::string created = p.createdAt.empty() ? currentTimestamp() : p.createdAt;
    out << ",=\"" << g_currentOperator << "\""
        << ",=\"" << created << "\""
        << ",=\"" << panelStatusToString(p.status) << "\""
        << ",=\"" << p.sourceFile << "\""
        << "\n";

    out.close();
    
    // Set file as read-only to prevent accidental editing
    SetFileAttributesA(masterCsvPath.c_str(), FILE_ATTRIBUTE_READONLY);
}

void moveInputPanelToArchive(const std::string& sourcePath) {
    try {
        std::string archiveDir = getAbsolutePath(WolfTrackConfig::INPUT_PANELS_ARCHIVE);
        fs::create_directories(archiveDir);

        fs::path source(sourcePath);
        if (!fs::exists(source)) {
            return;
        }
        
        // Only archive files, not directories
        if (!fs::is_regular_file(source)) {
            return;
        }

        // Create timestamped filename for archive
        std::string stem = source.stem().string();
        std::string ext = source.extension().string();
        
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        tm = *std::localtime(&t);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
        
        std::string timestampedFilename = stem + "_" + oss.str() + ext;
        fs::path destPath = fs::path(archiveDir) / timestampedFilename;

        // Copy instead of move, so file can be run multiple times
        fs::copy_file(source, destPath, fs::copy_options::overwrite_existing);
    } catch (...) {
        // Silently ignore any filesystem errors
        return;
    }
}

bool loadPanelFromCsvFile(const std::string& csvPath, Panel& outPanel) {
    try {
        std::ifstream file(csvPath);
        if (!file.is_open()) {
            return false;
        }

        // Read header row
        std::string header;
        if (!std::getline(file, header)) {
            return false;
        }

        // Read data row
        std::string dataLine;
        if (!std::getline(file, dataLine)) {
            return false;
        }

        // Split the data row by comma
        std::vector<std::string> fields;
        std::stringstream ss(dataLine);
        std::string field;
        while (std::getline(ss, field, ',')) {
            // Trim trailing whitespace
            while (!field.empty() && (field.back() == '\r' || field.back() == ' ')) {
                field.pop_back();
            }
            while (!field.empty() && field.front() == ' ') {
                field.erase(field.begin());
            }
            fields.push_back(field);
        }

        if (fields.size() < 25) {
            return false;
        }

        // Fill the Panel object
        outPanel.panelID = generateNextPanelID();
        outPanel.panelNumber = fields[0];
        for (size_t i = 0; i < 24; ++i) {
            outPanel.pcbSerials[i] = fields[i + 1];
        }
        outPanel.status = PanelStatus::Detected;
        outPanel.createdAt = currentTimestamp();
        outPanel.laseredAt = "";
        outPanel.sourceFile = csvPath;

        // Close the file before moving it
        file.close();

        // Save to master CSV
        appendPanelToMaster(outPanel);

        // Move input file to archive
        moveInputPanelToArchive(csvPath);

        return true;
    } catch (...) {
        return false;
    }
}

std::string getPanelPendingFolder(const Panel& panel) {
    std::string pendingRoot = getAbsolutePath(WolfTrackConfig::PENDING_ART_ROOT);
    fs::path folder = fs::path(pendingRoot) / panel.panelID;
    
    fs::create_directories(folder);
    
    return folder.string();
}

std::string createPanelArtSvg(const Panel& panel) {
    try {
        std::string folder = getPanelPendingFolder(panel);
        fs::path svgPath = fs::path(folder) / (panel.panelID + "_panel_art.svg");
        
        std::ofstream svg(svgPath);
        if (!svg.is_open()) {
            return "";
        }
        
        // SVG dimensions for full panel artwork
        int svgWidth = 800;
        int svgHeight = 550;
        
        // Grid layout matching GUI
        int originX = 20;
        int originY = 100;
        int cols = 6;
        int rows = 4;
        int slotWidth = 120;
        int slotHeight = 60;
        int hGap = 10;
        int vGap = 10;
        
        svg << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        svg << "<svg width=\"" << svgWidth << "\" height=\"" << svgHeight 
            << "\" xmlns=\"http://www.w3.org/2000/svg\">\n";
        svg << "  <!-- AVO Invents Ltd - WolfTrack Panel Artwork -->\n";
        
        // White background
        svg << "  <rect width=\"" << svgWidth << "\" height=\"" << svgHeight 
            << "\" fill=\"white\"/>\n";
        
        // Header information
        svg << "  <text x=\"20\" y=\"30\" font-family=\"Arial\" font-size=\"18\" "
            << "font-weight=\"bold\" fill=\"black\">Panel ID: " << panel.panelID << "</text>\n";
        svg << "  <text x=\"20\" y=\"55\" font-family=\"Arial\" font-size=\"16\" "
            << "fill=\"black\">Operator: " << g_currentOperator << "</text>\n";
        svg << "  <text x=\"20\" y=\"75\" font-family=\"Arial\" font-size=\"14\" "
            << "fill=\"gray\">Panel Number: " << panel.panelNumber << "</text>\n";
        
        // Draw 24 PCB slots in 6x4 grid
        for (int i = 0; i < 24; ++i) {
            int row = i / cols;
            int col = i % cols;
            
            int x = originX + col * (slotWidth + hGap);
            int y = originY + row * (slotHeight + vGap);
            
            // Draw rectangle for PCB slot
            svg << "  <rect x=\"" << x << "\" y=\"" << y 
                << "\" width=\"" << slotWidth << "\" height=\"" << slotHeight
                << "\" fill=\"none\" stroke=\"black\" stroke-width=\"1.5\"/>\n";
            
            // PCB label and serial (truncated to first 12 chars for space)
            std::string pcbLabel = "PCB" + std::to_string(i + 1);
            std::string serial = panel.pcbSerials[i];
            if (serial.length() > 12) {
                serial = serial.substr(0, 12);
            }
            
            // Draw PCB number
            svg << "  <text x=\"" << (x + 5) << "\" y=\"" << (y + 18)
                << "\" font-family=\"Arial\" font-size=\"12\" font-weight=\"bold\" "
                << "fill=\"black\">" << pcbLabel << "</text>\n";
            
            // Draw serial number
            svg << "  <text x=\"" << (x + 5) << "\" y=\"" << (y + 38)
                << "\" font-family=\"Courier New\" font-size=\"10\" "
                << "fill=\"black\">" << serial << "</text>\n";
        }
        
        // Outer panel border
        int panelWidth = cols * (slotWidth + hGap) - hGap;
        int panelHeight = rows * (slotHeight + vGap) - vGap;
        svg << "  <rect x=\"" << (originX - 5) << "\" y=\"" << (originY - 5)
            << "\" width=\"" << (panelWidth + 10) << "\" height=\"" << (panelHeight + 10)
            << "\" fill=\"none\" stroke=\"blue\" stroke-width=\"2\"/>\n";
        
        svg << "</svg>\n";
        svg.close();
        
        return svgPath.string();
    } catch (...) {
        return "";
    }
}

std::string createPanelBarcodeSvgPlaceholder(const Panel& panel) {
    try {
        std::string folder = getPanelPendingFolder(panel);
        fs::path svgPath = fs::path(folder) / (panel.panelID + "_datamatrix.svg");
        
        std::ofstream svg(svgPath);
        if (!svg.is_open()) {
            return "";
        }
        
        // Write SVG for DataMatrix-style label
        svg << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        svg << "<svg width=\"220\" height=\"260\" xmlns=\"http://www.w3.org/2000/svg\">\n";
        svg << "  <!-- Placeholder DataMatrix Label -->\n";
        svg << "  <rect width=\"220\" height=\"260\" fill=\"white\"/>\n";
        
        // Draw a 10x10 grid pattern to simulate DataMatrix code
        int gridSize = 10;
        int cellSize = 12;
        int startX = 40;
        int startY = 30;
        
        for (int row = 0; row < gridSize; ++row) {
            for (int col = 0; col < gridSize; ++col) {
                // Simple pattern for placeholder
                if ((row + col) % 2 == 0 || row == 0 || col == 0 || row == gridSize-1 || col == gridSize-1) {
                    int x = startX + col * cellSize;
                    int y = startY + row * cellSize;
                    svg << "  <rect x=\"" << x << "\" y=\"" << y 
                        << "\" width=\"" << cellSize << "\" height=\"" << cellSize 
                        << "\" fill=\"black\"/>\n";
                }
            }
        }
        
        // Add border around the code
        svg << "  <rect x=\"" << (startX - 5) << "\" y=\"" << (startY - 5) 
            << "\" width=\"" << (gridSize * cellSize + 10) << "\" height=\"" << (gridSize * cellSize + 10)
            << "\" fill=\"none\" stroke=\"black\" stroke-width=\"2\"/>\n";
        
        // Add panel ID text below the barcode
        svg << "  <text x=\"110\" y=\"190\" font-family=\"Arial\" font-size=\"16\" "
            << "font-weight=\"bold\" text-anchor=\"middle\" fill=\"black\">" 
            << panel.panelID << "</text>\n";
        
        // Add operator text
        svg << "  <text x=\"110\" y=\"215\" font-family=\"Arial\" font-size=\"14\" "
            << "text-anchor=\"middle\" fill=\"black\">Operator: " 
            << g_currentOperator << "</text>\n";
        
        svg << "</svg>\n";
        svg.close();
        
        return svgPath.string();
    } catch (...) {
        return "";
    }
}
