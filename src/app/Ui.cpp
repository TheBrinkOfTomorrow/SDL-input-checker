#include "app/Ui.h"

#include <SDL3/SDL.h>
#include <imgui.h>

#include "app/App.h"
#include "devices/DeviceRegistry.h"
#include "devices/HardwareProbe.h"

void Ui::draw(DeviceRegistry& registry, const HardwareProbe& probe, bool& quitRequested) {
    const auto& findings = probe.findings();
    if (selectedFinding_ >= static_cast<int>(findings.size())) selectedFinding_ = -1;
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const float scale = ImGui::GetStyle().FontScaleDpi;
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(ImVec2(App::kSidebarW * scale, vp->WorkSize.y));

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (!ImGui::Begin("Sidebar", nullptr, flags)) {
        ImGui::End();
        return;
    }

    ImGui::TextUnformatted("SDL Input Checker");
    ImGui::Separator();

    if (std::string n = registry.takeNotice(); !n.empty()) {
        notice_ = n;
        noticeTtl_ = 4.0f;
    }

    ImGui::SeparatorText("Devices");
    const float footerH = ImGui::GetFrameHeightWithSpacing() * 4 + ImGui::GetTextLineHeightWithSpacing();
    if (ImGui::BeginChild("DeviceList", ImVec2(0, -footerH), ImGuiChildFlags_None)) {
        DeviceKind lastKind{};
        bool first = true;
        std::optional<DeviceId> selected = registry.selected();
        for (const DeviceInfo& d : registry.devices()) {
            if (first || d.id.kind != lastKind) {
                if (!first) ImGui::Spacing();
                ImGui::TextDisabled("%s", deviceKindName(d.id.kind));
                lastKind = d.id.kind;
                first = false;
            }
            ImGui::PushID(static_cast<int>(d.id.kind) * 100000 + static_cast<int>(d.id.id));
            const bool isSel = selected && *selected == d.id;
            const float rowH = ImGui::GetTextLineHeight() * 2 + ImGui::GetStyle().ItemSpacing.y;
            const ImVec2 pos = ImGui::GetCursorScreenPos();
            if (ImGui::Selectable("##row", isSel, ImGuiSelectableFlags_None, ImVec2(0, rowH))) {
                registry.select(isSel ? std::nullopt : std::optional<DeviceId>(d.id));
                selectedFinding_ = -1;
            }
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImVec2 pad = ImGui::GetStyle().FramePadding;
            dl->AddText(ImVec2(pos.x + pad.x, pos.y), ImGui::GetColorU32(ImGuiCol_Text), d.name.c_str());
            dl->AddText(ImVec2(pos.x + pad.x, pos.y + ImGui::GetTextLineHeight()),
                        ImGui::GetColorU32(ImGuiCol_TextDisabled), d.detail.c_str());
            ImGui::PopID();
        }

        // Hardware the OS reports but SDL does not list (or lists only partially).
        if (!findings.empty()) {
            ImGui::Spacing();
            ImGui::TextDisabled("Diagnostics");
            for (int i = 0; i < static_cast<int>(findings.size()); ++i) {
                const Finding& f = findings[i];
                ImGui::PushID(1000000 + i);
                const bool isSel = selectedFinding_ == i;
                const float rowH = ImGui::GetTextLineHeight() * 2 + ImGui::GetStyle().ItemSpacing.y;
                const ImVec2 pos = ImGui::GetCursorScreenPos();
                if (ImGui::Selectable("##finding", isSel, ImGuiSelectableFlags_None, ImVec2(0, rowH))) {
                    selectedFinding_ = isSel ? -1 : i;
                    if (selectedFinding_ >= 0) registry.select(std::nullopt);
                }
                const ImU32 col = f.level == FindingLevel::Problem ? IM_COL32(255, 120, 90, 255)
                                : f.level == FindingLevel::Warning ? IM_COL32(255, 190, 80, 255)
                                                                   : ImGui::GetColorU32(ImGuiCol_Text);
                ImDrawList* dl = ImGui::GetWindowDrawList();
                const ImVec2 pad = ImGui::GetStyle().FramePadding;
                dl->AddText(ImVec2(pos.x + pad.x, pos.y), col, f.device.c_str());
                dl->AddText(ImVec2(pos.x + pad.x, pos.y + ImGui::GetTextLineHeight()),
                            ImGui::GetColorU32(ImGuiCol_TextDisabled), f.summary.c_str());
                ImGui::PopID();
            }
        }
    }
    ImGui::EndChild();

    ImGui::Separator();
    ImGui::Text("%d pads  %d sticks  %d kbd  %d mice", registry.count(DeviceKind::Gamepad),
                registry.count(DeviceKind::Joystick), registry.count(DeviceKind::Keyboard),
                registry.count(DeviceKind::Mouse));
    if (noticeTtl_ > 0.0f) {
        noticeTtl_ -= ImGui::GetIO().DeltaTime;
        ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "%s", notice_.c_str());
    } else {
        ImGui::TextDisabled("Hot-plug is live");
    }
    const int v = SDL_GetVersion();
    ImGui::Text("SDL %d.%d.%d   %.0f fps", SDL_VERSIONNUM_MAJOR(v), SDL_VERSIONNUM_MINOR(v),
                SDL_VERSIONNUM_MICRO(v), ImGui::GetIO().Framerate);
    if (ImGui::Button("Quit", ImVec2(-1, 0))) quitRequested = true;

    ImGui::End();

    if (registry.selected()) selectedFinding_ = -1;
    if (selectedFinding_ >= 0) drawFindingPanel(probe);
}

void Ui::drawFindingPanel(const HardwareProbe& probe) {
    const Finding& f = probe.findings()[selectedFinding_];
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const float side = App::kSidebarW * ImGui::GetStyle().FontScaleDpi;
    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + side, vp->WorkPos.y));
    ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x - side, vp->WorkSize.y));
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (ImGui::Begin("Finding", nullptr, flags)) {
        ImGui::Dummy(ImVec2(0, 8));
        ImGui::SetWindowFontScale(1.5f);
        ImGui::TextUnformatted(f.device.c_str());
        ImGui::SetWindowFontScale(1.0f);
        const ImVec4 col = f.level == FindingLevel::Problem ? ImVec4(1.0f, 0.47f, 0.35f, 1.0f)
                         : f.level == FindingLevel::Warning ? ImVec4(1.0f, 0.75f, 0.3f, 1.0f)
                                                            : ImVec4(0.6f, 0.85f, 1.0f, 1.0f);
        ImGui::TextColored(col, "%s", f.summary.c_str());
        ImGui::Separator();
        ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x * 0.95f);
        ImGui::TextUnformatted(f.detail.c_str());
        ImGui::PopTextWrapPos();
        ImGui::Spacing();
        ImGui::SeparatorText("What the OS sees");
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextUnformatted(probe.usbListing().c_str());
        ImGui::PopStyleColor();
    }
    ImGui::End();
}
