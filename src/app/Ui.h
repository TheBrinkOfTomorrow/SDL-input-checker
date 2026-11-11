#pragma once
#include <string>

class DeviceRegistry;
class HardwareProbe;

// ImGui sidebar: device list, selection, status.
class Ui {
public:
    void draw(DeviceRegistry& registry, const HardwareProbe& probe, bool& quitRequested);
    // True while a diagnostics entry (rather than a device) is shown in the canvas area.
    bool findingSelected() const { return selectedFinding_ >= 0; }
    void selectFinding(int i) { selectedFinding_ = i; }

private:
    std::string notice_;
    float noticeTtl_ = 0.0f;
    int selectedFinding_ = -1;

    void drawFindingPanel(const HardwareProbe& probe);
};
