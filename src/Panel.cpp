#include "Panel.h"

std::string panelStatusToString(PanelStatus status) {
    switch (status) {
        case PanelStatus::Detected:      return "Detected";
        case PanelStatus::LabelPrinted:  return "LabelPrinted";
        case PanelStatus::ReadyForLaser: return "ReadyForLaser";
        case PanelStatus::Lasered:       return "Lasered";
        default:                         return "Unknown";
    }
}
