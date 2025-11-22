#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include "Panel.h"
#include "MasterData.h"
#include "Gui.h"

int main() {
    std::cout << "AVO Invents Automation test build OK\n";

    // Path to the test file
    std::string csvPath = "InputPanels/panel_test.csv";

    std::ifstream file(csvPath);
    if (!file.is_open()) {
        std::cout << "ERROR: Could not open " << csvPath << "\n";
        return 1;
    }

    // Read header row
    std::string header;
    if (!std::getline(file, header)) {
        std::cout << "ERROR: File is empty.\n";
        return 1;
    }

    std::cout << "Header row: " << header << "\n";

    // Read data row (the single panel row)
    std::string dataLine;
    if (!std::getline(file, dataLine)) {
        std::cout << "ERROR: No data row found after header.\n";
        return 1;
    }

    // Split the data row by tab (your file uses tab separators)
    std::vector<std::string> fields;
    {
        std::stringstream ss(dataLine);
        std::string field;
        while (std::getline(ss, field, '\t')) {
            // Trim possible leading/trailing spaces
            while (!field.empty() && (field.back() == '\r' || field.back() == ' ')) {
                field.pop_back();
            }
            while (!field.empty() && field.front() == ' ') {
                field.erase(field.begin());
            }
            fields.push_back(field);
        }
    }

    if (fields.size() < 25) { // 1 for PanelNumber + 24 PCB serials
        std::cout << "ERROR: Expected at least 25 fields (PanelNumber + 24 PCB serials), got "
                  << fields.size() << "\n";
        return 1;
    }

    // Fill a Panel object from the fields
    Panel p;
    p.panelID = generateNextPanelID();
    p.panelNumber = fields[0];    // first column is PanelNumber
    for (size_t i = 0; i < 24; ++i) {
        p.pcbSerials[i] = fields[i + 1]; // PCB1..PCB24
    }
    p.status = PanelStatus::Detected;
    p.createdAt = "";
    p.laseredAt = "";
    p.sourceFile = csvPath;

    // Close the input file before moving it
    file.close();

    // Print a small summary to prove parsing worked
    std::cout << "Parsed panel:\n";
    std::cout << "  PanelID:    " << p.panelID << "\n";
    std::cout << "  PanelNumber:" << p.panelNumber << "\n";
    std::cout << "  PCB1:       " << p.pcbSerials[0] << "\n";
    std::cout << "  PCB24:      " << p.pcbSerials[23] << "\n";
    std::cout << "  Status:     " << panelStatusToString(p.status) << "\n";


    // Save to master CSV
    appendPanelToMaster(p);
    std::cout << "Panel appended to master CSV.\n";

    moveInputPanelToArchive(csvPath);
    std::cout << "Input panel moved to archive.\n";

    std::cout << "DEBUG: End of main reached\n";

    runPanelViewerGui(p);

    return 0;
}
