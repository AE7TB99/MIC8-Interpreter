#pragma once

#include "chip8.hpp"
#include "imgui.h"
#include "imgui_memory_editor.h"
#include "ImGuiFileDialog.h"

#include <GL/gl.h>

#include <array>
#include <cstddef>
#include <deque>
#include <string>
#include <string_view>
#include <vector>

class instance_manager {
public:
    void run();

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

        [[nodiscard]] constexpr auto get_id() const -> std::size_t { return id; }

        [[nodiscard]] constexpr auto get_state() const -> state { return state; }

        [[nodiscard]] constexpr auto get_input_enabled() const -> bool { return input_enabled; }

        [[nodiscard]] constexpr auto get_alt_ops() const -> chip8::alt_t { return alt_ops; }

        void run();
        void reset();
        void load(std::string_view path);
        void process_input();

        void controller_window();
        void fb_window();
        void cpu_view_window();
        void mem_view_window();
        void instruction_log_window();

    private:
        chip8 interpreter;
        GLuint tex_id {};
        MemoryEditor mem_edit;
        std::deque<std::string> instruction_log;
        decltype(instruction_log)::size_type instruction_log_max {1000};
        bool scroll_flag;

        std::size_t id;
        state state {};
        unsigned short ips {15};
        unsigned char multiplier {1};
        bool input_enabled {};

        //this is kind of ugly to be honest...
        std::chrono::time_point<std::chrono::steady_clock> last_timer_time {std::chrono::steady_clock::now()};
        std::chrono::time_point<std::chrono::steady_clock> last_cycle_time {std::chrono::steady_clock::now()};

        chip8::alt_t alt_ops;

        struct {
            bool show_controller {true};
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
    ssize_t selected_id {-1};

    void instance_manager_window();

    [[nodiscard]] auto instance_search() const -> std::size_t;
    [[nodiscard]] auto selected_search() const -> ssize_t;
};


