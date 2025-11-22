#include "MasterData.h"
#include <fstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

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
    if (!fs::exists(MASTER_CSV_PATH)) {
        std::ofstream out(MASTER_CSV_PATH, std::ios::out);
        out << "PanelID,PanelNumber";
        for (int i = 1; i <= 24; ++i) {
            out << ",PCB" << i;
        }
        out << ",CreatedAt,LaseredAt,Status,SourceFile\n";
        out.close();
    }
}

std::string generateNextPanelID() {
    ensureMasterCsvExists();

    std::ifstream in(MASTER_CSV_PATH);
    if (!in.is_open()) {
        return "WT-P-00001";
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(line);
    }
    in.close();

    // If only header or empty, return first ID
    if (lines.size() <= 1) {
        return "WT-P-00001";
    }

    // Get the last non-empty data row
    std::string lastRow;
    for (int i = static_cast<int>(lines.size()) - 1; i >= 1; --i) {
        if (!lines[i].empty()) {
            lastRow = lines[i];
            break;
        }
    }

    if (lastRow.empty()) {
        return "WT-P-00001";
    }

    // Extract first field (PanelID)
    size_t commaPos = lastRow.find(',');
    if (commaPos == std::string::npos) {
        return "WT-P-00001";
    }

    std::string lastPanelID = lastRow.substr(0, commaPos);

    // Expected format: "WT-P-XXXXX"
    if (lastPanelID.size() < 6 || lastPanelID.substr(0, 5) != "WT-P-") {
        return "WT-P-00001";
    }

    // Extract numeric part
    std::string numStr = lastPanelID.substr(5);
    int num = 0;
    try {
        num = std::stoi(numStr);
    } catch (...) {
        return "WT-P-00001";
    }

    // Increment and format
    num++;
    std::ostringstream oss;
    oss << "WT-P-" << std::setfill('0') << std::setw(5) << num;
    return oss.str();
}

void appendPanelToMaster(const Panel& p) {
    ensureMasterCsvExists();

    std::ofstream out(MASTER_CSV_PATH, std::ios::app);
    if (!out.is_open()) {
        return; // in v1 we silently fail; can add error handling later
    }

    // Base fields
    out << p.panelID << "," << p.panelNumber;

    // 24 PCB serials
    for (size_t i = 0; i < p.pcbSerials.size(); ++i) {
        out << "," << p.pcbSerials[i];
    }

    // Timestamps and status
    std::string created = p.createdAt.empty() ? currentTimestamp() : p.createdAt;
    std::string lasered = p.laseredAt;
    out << "," << created
        << "," << lasered
        << "," << panelStatusToString(p.status)
        << "," << p.sourceFile
        << "\n";

    out.close();
}

void moveInputPanelToArchive(const std::string& sourcePath) {
    try {
        fs::create_directories("InputPanelsArchive");

        fs::path source(sourcePath);
        if (!fs::exists(source)) {
            return;
        }

        std::string filename = source.filename().string();
        fs::path destPath = fs::path("InputPanelsArchive") / filename;

        fs::rename(source, destPath);
    } catch (...) {
        // Silently ignore any filesystem errors
        return;
    }
}
