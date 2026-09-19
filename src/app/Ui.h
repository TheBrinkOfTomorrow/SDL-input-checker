#pragma once
#include <string>

class DeviceRegistry;

// ImGui sidebar: device list, selection, status.
class Ui {
public:
    void draw(DeviceRegistry& registry, bool& quitRequested);

private:
    std::string notice_;
    float noticeTtl_ = 0.0f;
};
