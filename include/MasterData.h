#pragma once

#include <string>
#include "Panel.h"

// Path to the master CSV file
const std::string MASTER_CSV_PATH = "MasterData/wolftrack_panels_master.csv";

// Ensure the master CSV exists and has a header row
void ensureMasterCsvExists();

// Generate the next sequential PanelID
std::string generateNextPanelID();

// Append a panel as a new row in the master CSV
void appendPanelToMaster(const Panel& p);

// Move input panel CSV to archive folder
void moveInputPanelToArchive(const std::string& sourcePath);
