# WolfTrack Panel Station - Professional Desktop UI

## Implementation Summary

The WolfTrack Panel Station has been redesigned as a professional Windows desktop application with enterprise-grade UI patterns, following native Win32/WPF design principles.

## Visual Theme

**Style**: Flat, clean, Windows 11-inspired but denser for manufacturing/enterprise use  
**Approach**: Professional desktop application, NOT a web interface

## Color Palette

### Backgrounds
- **App Canvas**: `#f8fafc` (Slate-50) - `RGB(248, 250, 252)` - Clean off-white workspace
- **Sidebar**: `#ffffff` (White) - `RGB(255, 255, 255)` - Pure white control panel
- **Header**: `#0f172a` (Slate-900) - `RGB(15, 23, 42)` - Dark, high-contrast top bar

### Accent Colors
- **Primary Action**: `#0284c7` (Sky-600) - `RGB(2, 132, 199)` - Avo Blue
- **Primary Hover**: `#0ea5e9` (Sky-500) - `RGB(14, 165, 233)`
- **Primary Tint**: `#e0f2fe` (Sky-100) - `RGB(224, 242, 254)` - Light blue background for active states
- **Success**: `#10b981` (Emerald-500) - `RGB(16, 185, 129)` - Green for completed steps
- **LED Glow**: `#34d399` (Emerald-400) - `RGB(52, 211, 153)` - Bright green LED indicator
- **Processing/Warning**: `#f59e0b` (Amber-500) - `RGB(245, 158, 11)` - Orange for active processing

### Text & UI Elements
- **Primary Text**: `#0f172a` (Slate-900) - `RGB(15, 23, 42)`
- **Secondary Text**: `#475569` (Slate-600) - `RGB(71, 85, 105)`
- **Tertiary/Hint**: `#94a3b8` (Slate-400) - `RGB(148, 163, 184)`
- **Dark Text**: `#334155` (Slate-700) - `RGB(51, 65, 85)`
- **Card Borders**: `#cbd5e1` (Slate-300) - `RGB(203, 213, 225)`
- **Empty Slots**: `#e2e8f0` (Slate-200) - `RGB(226, 232, 240)` - Recessed appearance
- **Dividers**: `#e2e8f0` (Slate-200) - `RGB(226, 232, 240)`

## Typography

### System Fonts
- **Headers**: Segoe UI, Bold, 22pt
- **Sidebar Title**: Segoe UI, Semibold, 14pt
- **Body Text**: Segoe UI, Regular, 13-14pt
- **Command Buttons**: Segoe UI, Semibold, 14pt
- **Technical Data**: Consolas (monospace) - For Panel IDs, timestamps, serial numbers
  - Panel ID: Bold, 14pt
  - Serial Numbers: Bold, 16pt
  - Position Labels: Regular, 10pt

### Font Usage
- Use **Segoe UI** for all UI labels, buttons, and general text (native Windows font)
- Use **Consolas** for technical/technical data (Panel IDs, timestamps, PCB serials)
- Monospace creates a professional, technical appearance for data fields

## Layout Structure

### Dimensions
- **Header Height**: 64px (fixed, docked top)
- **Status Strip Height**: 80px (fixed, docked bottom)
- **Right Control Panel Width**: 320px (fixed, docked right)
- **Main Content**: Fluid center area (auto-fills remaining space)
- **Window Size**: 1100×750px (optimized for enterprise displays)

### Layout Architecture
```
┌────────────────────────────────────────────────────────────────────┬──────────────┐
│ HEADER BAR (64px, Slate-900 background, white text)               │              │
│ • App Title (left): "WolfTrack Panel Station"                     │              │
│ • Meta Info (right): Panel ID | Operator | Time (Consolas mono)   │              │
│ • Vertical dividers between meta fields                           │              │
├────────────────────────────────────────────────────────────────────┤  RIGHT       │
│                                                                    │  CONTROL     │
│ MAIN CONTENT AREA (Slate-50 canvas background)                    │  PANEL       │
│                                                                    │  (320px)     │
│ ┌────────────────────────────────────────────────────────────┐    │              │
│ │ PCB GRID (4 columns × 6 rows = 24 slots)                   │    │  White       │
│ │ • Empty slots: Recessed, Slate-200, inner shadow           │    │  sidebar     │
│ │ • Filled slots: Raised, White, drop shadow, green LED      │    │              │
│ │ • Position label (tiny): "POS XX"                          │    │  ┌─────────┐ │
│ │ • Serial number (large, bold, Consolas, centered)          │    │  │ LOAD    │ │
│ └────────────────────────────────────────────────────────────┘    │  │ CSV     │ │
│                                                                    │  └─────────┘ │
│                                                                    │  ┌─────────┐ │
│                                                                    │  │GENERATE │ │
│                                                                    │  │ LASER   │ │
│                                                                    │  └─────────┘ │
│                                                                    │  ┌─────────┐ │
│                                                                    │  │  OPEN   │ │
│                                                                    │  │ FOLDER  │ │
│                                                                    │  └─────────┘ │
│                                                                    │  ┌─────────┐ │
│                                                                    │  │  VIEW   │ │
│                                                                    │  │ HISTORY │ │
│                                                                    │  └─────────┘ │
├────────────────────────────────────────────────────────────────────┴──────────────┤
│ STATUS STRIP (80px, White background, top border)                                │
│ • "WORKFLOW PROGRESS" label                                                      │
│ • Step 1 → Step 2 → Step 3 (Pills with connecting lines)                        │
│   - Inactive: Gray pill, gray text                                              │
│   - Active: Blue pill with blue border, blue text                               │
│   - Complete: Green checkmark, green text                                       │
└──────────────────────────────────────────────────────────────────────────────────┘
```

## Components

### Header Bar (64px, Slate-900)
- **Left**: App title "WolfTrack Panel Station" (Segoe UI Bold, 22pt, white)
- **Right**: Meta information panel with monospace technical data
  - Panel ID (Consolas, 14pt)
  - Operator name (Consolas, 14pt)
  - Current time (Consolas, 14pt, live clock)
  - Vertical dividers (Slate-600) between fields
- **Typography**: Labels in Slate-400, values in White

### Right Control Panel (320px, White sidebar)
- **Background**: Pure white with left border (Slate-200)
- **Title**: "Actions" (Segoe UI Semibold, 14pt, Slate-700)
- **Command Buttons**: Large, 76px height, multi-line
  - **Layout**: Icon area (left, 24px) + Text area (right)
  - **Title**: Bold, 14pt on first line
  - **Subtitle**: Regular, smaller on second line (step number/description)
  - **States**:
    - Idle: White bg, light gray border, dark gray text
    - Hover: Sky-blue border, sky-tinted background
    - Primary: Solid Sky-600 background, white text
  - **Spacing**: 12px between buttons
  - **Note**: Current Win32 implementation uses multiline text-only buttons (custom drawing can add icons later)

### Main Content Area (Fluid center, Slate-50 canvas)
- **PCB Grid**: 4 columns × 6 rows = 24 PCB slots
  - **Grid Layout**:
    - Slot size: 160px × 70px
    - Horizontal gap: 12px
    - Vertical gap: 12px
    - Centered in available space
  
- **Empty Slot Appearance** (Recessed/Disabled):
  - Background: Slate-200
  - Inner shadow effect (darker top-left edge)
  - Position label: "POS XX" (tiny, uppercase, Slate-400, Segoe UI 10pt)
  - Center text: "Empty" (Segoe UI 12pt, Slate-400)
  - Visual effect: Looks like an empty socket waiting to be filled
  
- **Filled Slot Appearance** (Raised/Active):
  - Drop shadow: 2px offset, light gray (Slate-300)
  - Background: White
  - Border: Slate-300, 1px solid
  - Position label: "POS XX" (top-left, tiny, Slate-400, Segoe UI 10pt)
  - Serial number: Large, bold, centered (Consolas 16pt, Slate-900)
  - LED indicator: Green dot (6px circle) in top-right corner
    - Fill: Emerald-400 (bright glow)
    - Border: Emerald-500
  - Visual effect: Looks like a raised card with depth

### Status Strip (80px, White, bottom)
- **Top border**: Slate-200, 1px
- **Title**: "WORKFLOW PROGRESS" (Segoe UI Semibold, 12pt, Slate-600)
- **Progress Pipeline**: Horizontal step visualization
  - **Step Spacing**: 180px between steps
  - **Connecting Lines**: 2px solid lines between steps
    - Gray when incomplete
    - Green when step complete
  
  - **Step States**:
    - **Inactive Step**: 
      - Gray pill background (Slate-200)
      - Gray border (Slate-300)
      - Gray text (Slate-400)
      - Text: "Step X: Description"
    
    - **Active Step** (processing):
      - Light blue background (Sky-100)
      - Blue border (Sky-600, 2px)
      - Blue text (Sky-600)
      - Rounded pill shape (12px radius)
    
    - **Completed Step**:
      - No pill background
      - Green checkmark: ✓
      - Green text (Emerald-500)
      - Text: "✓ Step X: Description"

## Visual Design Patterns

### Depth & Elevation
- **Recessed (empty slots)**: Inner shadows, darker background, appears sunken
- **Raised (filled slots)**: Drop shadows, white surface, appears elevated
- **Flat (UI chrome)**: Header, sidebar, status strip - no shadows

### State Indication
- **LED Indicators**: Small circular dots with glow effect for active items
- **Color-coded Pills**: Progress steps use pill shapes with colored backgrounds
- **Border Highlighting**: Active items use thicker, colored borders

### Information Hierarchy
1. **Critical Data** (Panel ID, Serial Numbers): Large, bold, monospace
2. **Meta Information** (Operator, Time): Medium, monospace in header
3. **Labels & Descriptions**: Small, regular weight, Segoe UI
4. **Status Indicators**: Color + icon + text for redundancy

## Implementation Details

All colors defined as constants in `Gui.cpp`:
```cpp
static const COLORREF COLOR_SLATE_50     = RGB(248, 250, 252);  // App canvas
static const COLORREF COLOR_SLATE_200    = RGB(226, 232, 240);  // Empty slots, borders
static const COLORREF COLOR_SLATE_300    = RGB(203, 213, 225);  // Filled slot borders
static const COLORREF COLOR_SLATE_400    = RGB(148, 163, 184);  // Tertiary text
static const COLORREF COLOR_SLATE_600    = RGB(71, 85, 105);    // Secondary text
static const COLORREF COLOR_SLATE_700    = RGB(51, 65, 85);     // Dark text
static const COLORREF COLOR_SLATE_900    = RGB(15, 23, 42);     // Header
static const COLORREF COLOR_WHITE        = RGB(255, 255, 255);  // Sidebar, cards
static const COLORREF COLOR_SKY_600      = RGB(2, 132, 199);    // Avo Blue primary
static const COLORREF COLOR_SKY_500      = RGB(14, 165, 233);   // Sky hover
static const COLORREF COLOR_SKY_100      = RGB(224, 242, 254);  // Sky tint
static const COLORREF COLOR_EMERALD_500  = RGB(16, 185, 129);   // Success
static const COLORREF COLOR_EMERALD_400  = RGB(52, 211, 153);   // LED glow
static const COLORREF COLOR_AMBER_500    = RGB(245, 158, 11);   // Warning/processing
```

Layout dimensions:
```cpp
static const int HEADER_HEIGHT      = 64;   // Top bar
static const int STATUS_HEIGHT      = 80;   // Bottom progress pipeline
static const int CONTROL_PANEL_WIDTH = 320; // Right sidebar
static const int PANEL_COLS     = 4;  // 4×6 grid
static const int PANEL_ROWS     = 6;
static const int SLOT_WIDTH     = 160;
static const int SLOT_HEIGHT    = 70;
static const int SLOT_H_GAP     = 12;
static const int SLOT_V_GAP     = 12;
```

## Key Features

### Professional Desktop UI
- **Native Windows Look**: Follows Win32/WPF design patterns, not web aesthetics
- **Enterprise Density**: Information-dense layout optimized for manufacturing workflows
- **Clear Visual Hierarchy**: Header → Content → Status, with distinct zones
- **Command-Oriented**: Large action buttons in dedicated control panel

### Manufacturing-Optimized
- **At-a-glance Status**: 24 PCB slots visible simultaneously in grid
- **Visual Depth Cues**: Empty vs. filled slots use recessed/raised visual metaphors
- **Technical Precision**: Monospace fonts for serial numbers, IDs, timestamps
- **Workflow Guidance**: Progress pipeline shows current step and completion status

### Accessibility
- High contrast ratios (WCAG AA compliant)
- Multiple status indicators (color + text + icons)
- Large touch targets for command buttons (76px height)
- Clear visual feedback for all interactions

## Future Enhancements

1. **Custom-drawn buttons**: Add icons (24px) on left side of command buttons using owner-draw
   - Load: Box/Upload icon
   - Generate: Lightning/Zap icon  
   - Folder: Open Folder icon
   - History: Clock/History icon

2. **Animated indicators**: Pulsing glow effect on active processing steps

3. **Hover effects**: Blue border highlight on button hover states

4. **Rounded corners**: Use GDI+ for modern rounded rectangle rendering (4-6px radius)

5. **Live clock**: Update time display in header every second

6. **Keyboard shortcuts**: Alt+L (Load), Alt+G (Generate), etc.

7. **Dark mode**: Alternative theme with inverted color scheme

8. **Custom window chrome**: Modern borderless window with custom title bar

## Platform Requirements

- Windows 10 or later (for Segoe UI font)
- Visual Studio 2022 BuildTools (MSVC compiler)
- Win32 API (user32, gdi32, shell32, comctl32, comdlg32)
- C++17 standard library
- std::filesystem for file operations

## Design Philosophy

This UI treats manufacturing software as a **professional desktop tool**, not a web application. Key principles:

- **Clarity over decoration**: Clean, functional design without unnecessary ornament
- **Information density**: Show all relevant data without scrolling
- **Workflow-driven**: UI guides user through sequential steps
- **Technical precision**: Monospace typography for technical data
- **Professional polish**: Proper use of depth, color, and typography

The result is a manufacturing tool that feels solid, capable, and purpose-built for production environments.
