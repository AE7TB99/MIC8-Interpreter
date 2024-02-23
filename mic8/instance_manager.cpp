#include "instance_manager.hpp"
#include "chip8.hpp"
#include "imgui.h"
#include "ImGuiFileDialog.h"
#include <GL/gl.h>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <sys/types.h>

namespace {
    void help_marker(const char* desc) {
        ImGui::TextDisabled("(?)");
        if (ImGui::BeginItemTooltip()) {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
}

auto instance_manager::instance_search() const -> std::size_t {
    std::size_t l = 0;
    std::size_t r = instances.size();
    while (l < r) {
        const auto m = l + (r - l) / 2;
        const auto m_val = instances[m].get_id();
        if (m_val == m) { l = m + 1; }
        else { r = m; }
    }
    return l;
}

auto instance_manager::selected_search() const -> ssize_t {
    for (const auto& instance: instances) {
        if (instance.selected) { return static_cast<ssize_t>(instance.get_id()); }
    }
    return -1;
}

instance_manager::instance::instance(const std::size_t id, const chip8::alt_t alt_ops) : interpreter(chip8(alt_ops)), id(id), alt_ops(alt_ops) {
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, chip8::VIDEO_WIDTH, chip8::VIDEO_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void instance_manager::instance_manager_window() {
    ImGui::SetNextWindowDockID(0x00000001);
    if (!ImGui::Begin("Instance Manager")) {
        ImGui::End();
        return;
    }

    const ssize_t selected_id = selected_search();

    if (selected_id != -1) {
        instances[selected_id].controller_window();
        instances[selected_id].fb_window();
        instances[selected_id].cpu_view_window();
        instances[selected_id].mem_view_window();
    }

    if (ImGui::CollapsingHeader("Create Instance", ImGuiTreeNodeFlags_DefaultOpen)) {
        static chip8::alt_t alt_ops;

        ImGui::SeparatorText("Alternative Instructions");
        ImGui::Checkbox("COSMAC VIP Logic", &alt_ops.vip_alu);
        ImGui::SameLine();
        help_marker("8XY1 / 8XY2 / 8XY3 set VF to 0");
        ImGui::Checkbox("CHIP48 Jump", &alt_ops.chip48_jmp);
        ImGui::SameLine();
        help_marker("BNNN is replaced by BXNN, which jumps to address XNN + the value in VX (instead of address NNN + the value in V0)");
        ImGui::Checkbox("CHIP48 Shift", &alt_ops.chip48_shf);
        ImGui::SameLine();
        help_marker("8XY6 / 8XYE shift VX and ignore VY");
        ImGui::Separator();
        if (ImGui::RadioButton("CHIP8 Load/Store", alt_ops.ls_mode == chip8::ls_mode::chip8_ls)) {
            alt_ops.ls_mode = chip8::ls_mode::chip8_ls;
        }
        if (ImGui::RadioButton("CHIP48 Load/Store", alt_ops.ls_mode == chip8::ls_mode::chip48_ls)) {
            alt_ops.ls_mode = chip8::ls_mode::chip48_ls;
        }
        ImGui::SameLine();
        help_marker("FX55 / FX65 increment I by one less than they should (if X is 0, I is not incremented at all)");
        if (ImGui::RadioButton("SUPER-CHIP 1.1 Load/Store", alt_ops.ls_mode == chip8::ls_mode::schip11_ls)) {
            alt_ops.ls_mode = chip8::ls_mode::schip11_ls;
        }
        ImGui::SameLine();
        help_marker("FX55 / FX65 no longer increment I at all");
        ImGui::Separator();
        if (ImGui::Button("Create", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            const auto pos = instance_search();
            instances.insert(instances.begin() + static_cast<decltype(instances)::difference_type>(pos), instance(pos, alt_ops));
            alt_ops = {};
        }
        ImGui::Spacing();
    }

    if (ImGui::CollapsingHeader("Selected Instance", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SeparatorText("Instance Attributes");

        std::string vip_alu = "Unknown";
        std::string chip48_jmp = "Unknown";
        std::string chip48_shf = "Unknown";
        std::string ls_mode = "Unknown";

        if (selected_id != -1) {
            vip_alu = instances[selected_id].get_alt_ops().vip_alu ? "Yes" : "No";
            chip48_jmp = instances[selected_id].get_alt_ops().chip48_jmp ? "Yes" : "No";
            chip48_shf = instances[selected_id].get_alt_ops().chip48_shf ? "Yes" : "No";
            switch (instances[selected_id].get_alt_ops().ls_mode) {
                case chip8::ls_mode::chip8_ls:
                    ls_mode = "CHIP8";
                    break;
                case chip8::ls_mode::chip48_ls:
                    ls_mode = "CHIP48";
                    break;
                case chip8::ls_mode::schip11_ls:
                    ls_mode = "SUPER-CHIP 1.1";
            }
        }

        static constexpr ImGuiTableFlags flags =
                (ImGuiTableFlags_Borders ^ ImGuiTableFlags_BordersInnerV) | ImGuiTableFlags_Resizable | ImGuiTableFlags_NoBordersInBodyUntilResize;
        if (ImGui::BeginTable("instance_table", 2, flags)) {
            ImGui::TableNextColumn();
            ImGui::Text("COSMAC VIP Logic:");
            ImGui::TableNextColumn();
            ImGui::Text("%s", vip_alu.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("CHIP48 Jump:");
            ImGui::TableNextColumn();
            ImGui::Text("%s", chip48_jmp.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("CHIP48 Shift:");
            ImGui::TableNextColumn();
            ImGui::Text("%s", chip48_shf.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("L/S Mode:");
            ImGui::TableNextColumn();
            ImGui::Text("%s", ls_mode.c_str());
            ImGui::EndTable();
        }
        ImGui::Separator();

        auto button_width = ImGui::GetContentRegionAvail().x / 2;
        ImGui::BeginDisabled(selected_id == -1);

        if (ImGui::Button("Load", ImVec2(button_width, 0))) {
            IGFD::FileDialogConfig file_dlg_config;
            file_dlg_config.path = "./roms/";
            file_dlg_config.countSelectionMax = 1;
            file_dlg_config.flags = ImGuiFileDialogFlags_Modal;
            ImGuiFileDialog::Instance()->OpenDialog("load_dlg_key", "Load ROM", ".ch8", file_dlg_config);
        };

        if (ImGuiFileDialog::Instance()->Display("load_dlg_key")) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                instances[selected_id].load(ImGuiFileDialog::Instance()->GetFilePathName());
            }
            ImGuiFileDialog::Instance()->Close();
        }

        ImGui::SameLine();

        if (ImGui::Button("Delete", ImVec2(button_width, 0))) {
            instances.erase(instances.begin() + selected_id);
        }
        ImGui::EndDisabled();
        ImGui::Spacing();
    }

    if (ImGui::CollapsingHeader("Current Instances", ImGuiTreeNodeFlags_DefaultOpen)) {
        static constexpr ImGuiTableFlags flags = (ImGuiTableFlags_Borders ^ ImGuiTableFlags_BordersInnerV) | ImGuiTableFlags_ScrollY;
        if (ImGui::BeginTable("instances_table", 2, flags)) {
            ImGui::TableSetupColumn("ID");
            ImGui::TableSetupColumn("State");
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();
            ImGui::TableNextRow();
            for (auto& instance: instances) {
                const auto& id = instance.get_id();
                ImGui::TableNextColumn();
                if (ImGui::Selectable(std::to_string(id).c_str(), &instance.selected, ImGuiSelectableFlags_SpanAllColumns)) {
                    if (selected_id != -1) { instances[selected_id].selected = false; }
                }
                ImGui::TableNextColumn();
                ImGui::Text("%s", instance::state_strings[static_cast<int>(instance.get_state())]);
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

//void instance_manager::instance::run_cycle() {
//    interpreter.run_cycle();
//}

void instance_manager::instance::load(std::string_view path) {
    state = state::LOADED;
    interpreter.unload_rom();
    interpreter.load_rom(path);
}

void instance_manager::instance::controller_window() {
    if (!ImGui::Begin(("Controller"), &windows.show_controller)) {
        ImGui::End();
        return;
    }
    constexpr unsigned char min = 0;
    constexpr unsigned char max = 100;
    static unsigned char delay;
    ImGui::SliderScalar("Emulation speed", ImGuiDataType_U8, &delay, &min, &max, "%u aaaaaaa");
    if (ImGui::Button("Run")) { state = state::RUNNING; }
    static unsigned char d = 0;
    if (state == state::RUNNING) {
        constexpr std::chrono::nanoseconds targetInterval(1000000000 / 60);
        auto currentTime = std::chrono::steady_clock::now();
        static auto lastExecutionTime = std::chrono::steady_clock::now();
        auto elapsedTime = currentTime - lastExecutionTime;

        if (elapsedTime >= targetInterval) {
            interpreter.decrement_timers();
            lastExecutionTime = currentTime;
        }
        ++d;
        if (d > delay) {
            d = 0;
            interpreter.run_cycle();
        }
    }
    ImGui::End();
}


void instance_manager::instance::fb_window() {
    if (interpreter.drw_flag) {
#if defined(GL_UNPACK_ROW_LENGHT) && !defined(__EMSCRIPTEM__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        glBindTexture(GL_TEXTURE_2D, tex_id);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, chip8::VIDEO_WIDTH, chip8::VIDEO_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, interpreter.get_fb().data());
        glBindTexture(GL_TEXTURE_2D, 0);
        interpreter.drw_flag = false;
    }
    if (!ImGui::Begin("Frame Buffer", &windows.show_fb)) {
        ImGui::End();
        return;
    }
    ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(tex_id)), ImGui::GetContentRegionAvail()); // NOLINT(*-pro-type-reinterpret-cast)
    ImGui::End();
}

void instance_manager::instance::cpu_view_window() {
    if (windows.show_cpu_view) {
        //        ImGui::SetNextWindowSize(ImVec2(152, 425), ImGuiCond_Once);
        if (!ImGui::Begin("CPU View", &windows.show_cpu_view)) {
            ImGui::End();
            return;
        }
        if (ImGui::BeginTable("general_registers", 2, ImGuiTableFlags_Borders, ImVec2(60.0f, 0.0f))) {
            ImGui::TableSetupColumn("REG");
            ImGui::TableSetupColumn("VAL");
            ImGui::TableHeadersRow();
            for (unsigned char i = 0; const auto& reg: interpreter.get_reg()) {
                ImGui::TableNextColumn();
                ImGui::Text("V%X", i);
                ImGui::TableNextColumn();
                ImGui::Text("%d", reg);
                ++i;
            }
            ImGui::EndTable();
        }
        ImGui::SameLine();
        if (ImGui::BeginTable("stack", 2, ImGuiTableFlags_Borders, ImVec2(68.0f, 0.0f))) {
            ImGui::TableSetupColumn("LVL");
            ImGui::TableSetupColumn("ADDR");
            ImGui::TableHeadersRow();
            for (unsigned char i = 0; const auto& addr: interpreter.get_stack()) {
                ImGui::TableNextColumn();
                if (i < interpreter.get_sp() - 1) {
                    ImGui::Text("%d", i);
                    ImGui::TableNextColumn();
                    ImGui::Text("%04x", addr);
                }
                else if (i == interpreter.get_sp() - 1) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_HeaderHovered));
                    ImGui::Text("%d", i);
                    ImGui::TableNextColumn();
                    ImGui::Text("%04x", addr);
                }
                else {
                    ImGui::TextDisabled("%d", i);
                    ImGui::TableNextColumn();
                    ImGui::TextDisabled("%04x", addr);
                }
                ++i;
            }
            ImGui::EndTable();
        }
        if (ImGui::BeginTable("other_registers", 2, 0, ImVec2(128.0f, 0.0f))) {
            ImGui::TableNextColumn();
            ImGui::Text("PC: %X", interpreter.get_pc());
            ImGui::TableNextColumn();
            ImGui::Text("DT: %X", interpreter.get_dt());
            ImGui::TableNextColumn();
            ImGui::Text("IR: %X", interpreter.get_ir());
            ImGui::TableNextColumn();
            ImGui::Text("ST: %X", interpreter.get_st());
            ImGui::TableNextColumn();
            ImGui::Text("SP: %X", interpreter.get_sp());
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

void instance_manager::instance::mem_view_window() {
    if (!ImGui::Begin("MEM View", &windows.show_mem_view)) {
        ImGui::End();
        return;
    }
    mem_edit.HighlightMin = interpreter.get_pc();
    mem_edit.HighlightMax = interpreter.get_pc() + chip8::INSTRUCTION_SIZE;
    mem_edit.DrawContents(const_cast<unsigned char*>(interpreter.get_mem().data()), chip8::MEM_SIZE); // NOLINT(*-pro-type-const-cast)
    ImGui::End();
}
