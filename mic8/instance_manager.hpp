#pragma once

#include "imgui.h"
#include "chip8.hpp"
#include "imgui_memory_editor.h"
#include "ImGuiFileDialog.h"

#include <GLFW/glfw3.h>

#include <array>
#include <string>
#include <string_view>
#include <vector>

class instance_manager {
public:
    void instance_manager_window();

private:
    class instance {
    public:
        enum class state : unsigned char {
            EMPTY,
            LOADED,
            RUNNING
        };

        static inline constexpr std::array<const char*, 3> state_strings = {"Empty", "Loaded", "Running"};

        bool selected {};

        instance(size_t id, chip8::alt_t alt_ops);

        [[nodiscard]] constexpr auto get_id() const -> size_t { return id; }

        [[nodiscard]] constexpr auto get_state() const -> state { return state; }

        [[nodiscard]] constexpr auto get_alt_ops() const -> chip8::alt_t { return alt_ops; }

        void fb_window();
        void cpu_view_window();


    private:
        chip8 interpreter;
        GLuint tex_id {};

        size_t id;
        state state {};
        bool input_enabled {};

        chip8::alt_t alt_ops;

        struct {
            bool show_controller {true};
            bool show_file_dlg {true};
            bool show_fb {true};
            bool show_cpu_view {true};
            bool show_mem_view {true};
            bool show_op_log {true};
        } windows;

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

    std::vector<instance> instances {};

    [[nodiscard]] auto instance_search() const -> size_t;
    [[nodiscard]] auto selected_search() const -> ssize_t;
};


