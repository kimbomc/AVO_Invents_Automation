#pragma once

#include <string>
#include "Panel.h"

// Path to the master CSV file
const std::string MASTER_CSV_PATH = "MasterData\\wolftrack_panels_master.csv";

// Statistics from master CSV
struct MasterStats {
    int totalPanels;
    int totalPcbs;
    std::string lastPanelID;
};

// Ensure the master CSV exists and has a header row
void ensureMasterCsvExists();

// Compute statistics from master CSV
MasterStats computeMasterStats();

// Generate the next sequential PanelID
std::string generateNextPanelID();

// Append a panel as a new row in the master CSV
void appendPanelToMaster(const Panel& p);

// Move input panel CSV to archive folder
void moveInputPanelToArchive(const std::string& sourcePath);

// Load a panel from CSV file, process it, and return success
bool loadPanelFromCsvFile(const std::string& csvPath, Panel& outPanel);

// Get the pending art folder path for a panel (creates if needed)
std::string getPanelPendingFolder(const Panel& panel);

// Create full panel artwork SVG for LightBurn
std::string createPanelArtSvg(const Panel& panel);

// Create a placeholder DataMatrix SVG label
std::string createPanelBarcodeSvgPlaceholder(const Panel& panel);
