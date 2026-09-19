#include "app/Ui.h"

#include <SDL3/SDL.h>
#include <imgui.h>

#include "app/App.h"
#include "devices/DeviceRegistry.h"

void Ui::draw(DeviceRegistry& registry, bool& quitRequested) {
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
            }
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImVec2 pad = ImGui::GetStyle().FramePadding;
            dl->AddText(ImVec2(pos.x + pad.x, pos.y), ImGui::GetColorU32(ImGuiCol_Text), d.name.c_str());
            dl->AddText(ImVec2(pos.x + pad.x, pos.y + ImGui::GetTextLineHeight()),
                        ImGui::GetColorU32(ImGuiCol_TextDisabled), d.detail.c_str());
            ImGui::PopID();
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
}
