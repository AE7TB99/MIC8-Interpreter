#include "instance_manager.hpp"
#include "chip8.hpp"
#include "imgui.h"
#include "ImGuiFileDialog.h"
#include <GL/gl.h>
#include <chrono>
#include <cmath>
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

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, chip8::VIDEO_WIDTH, chip8::VIDEO_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, interpreter.get_fb().data());

    glBindTexture(GL_TEXTURE_2D, 0);
}

void instance_manager::run() {
    selected_id = selected_search();

    instance_manager_window();

    if (selected_id != -1) {
        instances[selected_id].controller_window();
        instances[selected_id].fb_window();
        instances[selected_id].cpu_view_window();
        instances[selected_id].mem_view_window();
        instances[selected_id].instruction_log_window();
    }

    for (auto& instance: instances) {
        if (instance.get_state() == instance::state::RUNNING) {
            instance.run();
            if (instance.get_input_enabled()) { instance.process_input(); }
        }
    }
}

void instance_manager::instance_manager_window() {
    if (!ImGui::Begin("Instance Manager")) {
        ImGui::End();
        return;
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

void instance_manager::instance::run() {
    if (ips == 0) { return; }

    static constexpr std::chrono::nanoseconds timer_interval(16'666'667);
    std::chrono::nanoseconds cycle_interval(static_cast<unsigned>(std::round(1e9 / ips)));

    static auto last_timer_time = std::chrono::steady_clock::now();
    static auto last_cycle_time = std::chrono::steady_clock::now();

    auto current_time = std::chrono::steady_clock::now();
    auto elapsed_timer_time = current_time - last_timer_time;
    auto elapsed_cycle_time = current_time - last_cycle_time;

    if (elapsed_timer_time >= timer_interval) {
        interpreter.decrement_timers();
        last_timer_time = current_time;
    }

    if (elapsed_cycle_time >= cycle_interval) {
        for(unsigned char i = 0; i < multiplier; ++i) {
            interpreter.run_cycle();
            while (instruction_log.size() >= instruction_log_max) { instruction_log.pop_front(); }
            instruction_log.emplace_back(interpreter.get_instruction());
        }
        scroll_flag = true;
        last_cycle_time = current_time;
    }
}

void instance_manager::instance::reset() {
    interpreter.reset();
    instruction_log.clear();
}

void instance_manager::instance::load(std::string_view path) {
    state = state::LOADED;
    interpreter.unload_rom();
    interpreter.load_rom(path);
}

void instance_manager::instance::process_input() {
    for (std::size_t i = 0; i < chip8::KEY_COUNT; ++i) {
        interpreter.keys[i] = ImGui::IsKeyDown(input[i]);
    }
}

void instance_manager::instance::controller_window() {
    if (!ImGui::Begin(("Controller"), &windows.show_controller)) {
        ImGui::End();
        return;
    }
    constexpr unsigned short speed_min = 0;
    constexpr unsigned short speed_max = 144;
    constexpr unsigned char multiplier_min = 1;
    constexpr unsigned char multiplier_max = 50;
    ImGui::SliderScalar("Exection Speed", ImGuiDataType_U16, &ips, &speed_min, &speed_max, "%u ips");
    ImGui::SliderScalar("Speed Multiplier", ImGuiDataType_U8, &multiplier, &multiplier_min, &multiplier_max, "x%u");
    ImGui::BeginDisabled(state == state::EMPTY);
    ImGui::BeginDisabled(state == state::RUNNING);
    if (ImGui::Button("Run")) { state = state::RUNNING; }
    if (ImGui::Button("Step")) { run(); }
    ImGui::EndDisabled();
    if (ImGui::Button("Stop")) { state = state::LOADED; }
    if (ImGui::Button("Reset")) { reset(); }
    if (ImGui::Button("Reset + Stop")) {
        reset();
        state = state::LOADED;
    }
    ImGui::EndDisabled();
    ImGui::Checkbox("Enable Input", &input_enabled);
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
    const auto fb_window_height = ImGui::GetContentRegionAvail().y;
    ImGui::Image(reinterpret_cast<void*>(static_cast<std::uintptr_t>(tex_id)),
                 ImVec2(fb_window_height * 2, fb_window_height)); // NOLINT(*-pro-type-reinterpret-cast, *-no-int-to-ptr)
    ImGui::End();
}

void instance_manager::instance::cpu_view_window() {
    if (windows.show_cpu_view) {
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
                } else if (i == interpreter.get_sp() - 1) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_HeaderHovered));
                    ImGui::Text("%d", i);
                    ImGui::TableNextColumn();
                    ImGui::Text("%04x", addr);
                } else {
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
    mem_edit.DrawContents(reinterpret_cast<void*>(const_cast<unsigned char*>(interpreter.get_mem().data())),
                          chip8::MEM_SIZE); // NOLINT(*-pro-type-const-cast, *-pro-type-reinterpret-cast)
    ImGui::End();
}

void instance_manager::instance::instruction_log_window() {
    if (!ImGui::Begin("Instruction Log")) {
        ImGui::End();
        return;
    }
    for (const auto& instruction: instruction_log) {
        ImGui::Text("%s", instruction.data());
    }
    if (scroll_flag == true) {
        ImGui::SetScrollY(ImGui::GetScrollMaxY());
        scroll_flag = false;
    }
    ImGui::End();
}
