#pragma once

#include <string>
#include <array>

enum class PanelStatus {
    Detected,
    LabelPrinted,
    ReadyForLaser,
    Lasered
};

struct Panel {
    std::string panelID;                 // WT-P-00001 etc
    std::string panelNumber;             // From CSV (Julian's PanelNumber)
    std::array<std::string, 24> pcbSerials; // PCB1..PCB24
    std::string createdAt;               // Timestamp when imported
    std::string laseredAt;               // Timestamp when lasered
    PanelStatus status;                  // Current status
    std::string sourceFile;              // Path to original CSV
};

// Small helper to turn status into text
std::string panelStatusToString(PanelStatus status);
