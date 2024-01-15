#pragma once

#include "chip8.hpp"
#include "imgui.h"
#include "imgui_memory_editor.h"
#include "ImGuiFileDialog.h"

#include <GLFW/glfw3.h>

struct instance {
public:
    instance(instance& other) {
    }

    chip8 interpreter;
    unsigned id = 0;
    GLuint tex_id;
    ImGuiFileDialog file_dlg;
    MemoryEditor mem_edit;
    unsigned char ipf;
    bool running;
    bool loaded;
    bool input_enabled;
    std::deque<std::string> op_log;
    unsigned char op_log_max;

    bool show_controller;
    bool show_file_dlg;
    bool show_fb;
    bool show_cpu_view;
    bool show_mem_view;
    bool show_op_log;

private:
     //@formatter:off
    std::array<ImGuiKey, 16> input {
        ImGuiKey_X,
        ImGuiKey_1,
        ImGuiKey_2,
        ImGuiKey_3,
        ImGuiKey_Q,
        ImGuiKey_W,
        ImGuiKey_E,
        ImGuiKey_A,
        ImGuiKey_S,
        ImGuiKey_D,
        ImGuiKey_Z,
        ImGuiKey_C,
        ImGuiKey_4,
        ImGuiKey_R,
        ImGuiKey_F,
        ImGuiKey_V
    };
    //@formatter:on
};
