#include "chip8.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include "imgui_memory_editor.h"
#include "ImGuiFileDialog.h"

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#ifdef _WIN32
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#include <GLFW/glfw3.h>

namespace {
    struct instance {
        chip8 interpreter;
        unsigned id;
        GLuint tex_id;
        ImGuiFileDialog file_dlg;
        MemoryEditor mem_edit;
        unsigned char ipf;
        bool running;
        bool loaded;
        bool input;
        std::deque<std::string> op_log;
        unsigned char op_log_max;

        bool show_controller;
        bool show_file_dlg;
        bool show_fb;
        bool show_cpu_view;
        bool show_mem_view;
        bool show_op_log;
    };

    std::deque<instance> instances {};

    decltype(&instances[0]) input_p1 {nullptr};
    decltype(&instances[0]) input_p2 {nullptr};

    unsigned char input_count {0};

    //@formatter:off
    std::array<ImGuiKey, 16> keypad_1 {
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

    std::array<ImGuiKey, 16> keypad_2 {
        ImGuiKey_Comma,
        ImGuiKey_7,
        ImGuiKey_8,
        ImGuiKey_9,
        ImGuiKey_U,
        ImGuiKey_I,
        ImGuiKey_O,
        ImGuiKey_J,
        ImGuiKey_K,
        ImGuiKey_L,
        ImGuiKey_M,
        ImGuiKey_Period,
        ImGuiKey_0,
        ImGuiKey_P,
        ImGuiKey_Semicolon,
        ImGuiKey_Slash
    };
    //@formatter:on

    auto instance_search() -> unsigned char {
        std::size_t l = 0, r = instances.size();
        while (l < r) {
            const auto m = l + (r - l) / 2;
            auto& n = instances[m].id;
            std::uint8_t m_val = n;
            if (m_val == m) { l = m + 1; }
            else { r = m; }
        }
        return l;
    }

    void help_marker(const char* desc) {
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) && ImGui::BeginTooltip()) {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    void add_input(decltype(instances)::value_type& instance) {
        switch (input_count) {
            case 0:
                input_p1 = &instance;
                break;
            case 1:
                input_p2 = &instance;
                break;
            default:
                break; //this should never happen
        }
        ++input_count;
    }

    void remove_input(const decltype(instances)::value_type& instance) {
        switch (input_count) {
            case 1:
                input_p1 = nullptr;
                input_p2 = nullptr;
                break;
            case 2:
                if (input_p1 == &instance) { input_p1 = input_p2; }
                input_p2 = nullptr;
                break;
            default:
                break; //this should never happen
        }
        --input_count;
    }

    void glfw_error_callback(const int code, const char* description) {
        fprintf(stderr, "GLFW Error %d: %s\n", code, description);
    }
}

auto main() -> int {
    glfwSetErrorCallback(glfw_error_callback);
    if (glfwInit() == GLFW_FALSE) { return 1; }

    GLFWwindow* window = glfwCreateWindow(1280, 720, "MIC8 Interpreter", nullptr, nullptr);
    if (window == nullptr) { return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();

    ImVec4 clear_color = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);

    bool creation_popup {false};

    unsigned char op_log_default_size = 20;

    while (glfwWindowShouldClose(window) == 0) {
        glfwPollEvents();

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Instances")) {
                if (ImGui::MenuItem("Create New Instance")) { creation_popup = true; }
                if (ImGui::BeginMenu("Delete Instances")) {
                    for (auto it = instances.begin(); it != instances.end();) {
                        if (ImGui::MenuItem(("Delete Instance " + std::to_string(it->id)).c_str())) {
                            if (it->input) { --input_count; }
                            if (it == --instances.end()) { instances.erase(it); }
                            else { instances.erase(it++); }
                        } else { ++it; }
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Views")) {
                if (ImGui::BeginMenu("Controllers")) {
                    for (auto& instance: instances) {
                        ImGui::MenuItem(("Controller " + std::to_string(instance.id)).c_str(), nullptr, &instance.show_controller);
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Frame Buffers")) {
                    for (auto& instance: instances) {
                        ImGui::MenuItem(("Frame Buffer " + std::to_string(instance.id)).c_str(), nullptr, &instance.show_fb);
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Memory Viewers")) {
                    for (auto& instance: instances) {
                        ImGui::MenuItem(("Memory View " + std::to_string(instance.id)).c_str(), nullptr, &instance.show_mem_view);
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("CPU viewers")) {
                    for (auto& instance: instances) {
                        ImGui::MenuItem(("CPU View " + std::to_string(instance.id)).c_str(), nullptr, &instance.show_cpu_view);
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Logs")) {
                    for (auto& instance: instances) {
                        ImGui::MenuItem(("Log " + std::to_string(instance.id)).c_str(), nullptr, &instance.show_op_log);
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("ROMS")) {
                for (auto& instance: instances) {
                    if (const auto& id = std::to_string(instance.id); ImGui::MenuItem(("Load ROM into instance " + id).c_str(), nullptr,
                                                                                      &instance.show_file_dlg)) {
                        instance.file_dlg.OpenDialog("dlg_key_" + id, "Load ROM into instance " + id, ".ch8", "./roms/");
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Input")) {
                for (auto& instance: instances) {
                    const std::string& id = std::to_string(instance.id);
                    bool& input = instance.input;
                    if (input_count < 2) {
                        if (ImGui::MenuItem(("Toggle input for instance " + id).c_str(), nullptr, &input)) {
                            if (input) { add_input(instance); }
                            else { remove_input(instance); }
                        }
                    } else if (input) {
                        if (ImGui::MenuItem(("Toggle input for instance " + id).c_str(), nullptr, &input)) { remove_input(instance); }
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("EXIT")) { glfwSetWindowShouldClose(window, 1); }
            ImGui::EndMainMenuBar();
        }

        if (creation_popup) {
            ImGui::SetNextWindowSize(ImVec2(245, 223));
            ImGui::OpenPopup("Create Instance");
            creation_popup = false;
        }
        if (ImGui::BeginPopupModal("Create Instance", nullptr, ImGuiWindowFlags_NoResize)) {
            const unsigned char pos = instance_search();
            static bool vip_alu {false};
            static bool chip48_jmp {false};
            static bool chip48_shf {true};
            static chip8::ls_mode mode {chip8::ls_mode::chip48_ls};

            ImGui::SeparatorText("Alternative Instructions");
            ImGui::Checkbox("COSMAC VIP Logic", &vip_alu);
            ImGui::SameLine();
            help_marker("8XY1 / 8XY2 / 8XY3 set VF to 0");
            ImGui::Checkbox("CHIP48 Jump", &chip48_jmp);
            ImGui::SameLine();
            help_marker("BNNN is replaced by BXNN, which jumps to address XNN + the value in VX (instead of address NNN + the value in V0)");
            ImGui::Checkbox("CHIP48 Shift", &chip48_shf);
            ImGui::SameLine();
            help_marker("8XY6 / 8XYE shift VX and ignore VY");
            ImGui::Separator();
            if (ImGui::RadioButton("CHIP8 Load/Store", mode == chip8::ls_mode::chip8_ls)) { mode = chip8::ls_mode::chip8_ls; }
            if (ImGui::RadioButton("CHIP48 Load/Store", mode == chip8::ls_mode::chip48_ls)) { mode = chip8::ls_mode::chip48_ls; }
            ImGui::SameLine();
            help_marker("FX55 / FX65 increment I by one less than they should (if X is 0, I is not incremented at all)");
            if (ImGui::RadioButton("SUPER-CHIP 1.1 Load/Store", mode == chip8::ls_mode::schip11_ls)) { mode = chip8::ls_mode::schip11_ls; }
            ImGui::SameLine();
            help_marker("FX55 / FX65 no longer increment I at all");
            ImGui::Separator();
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
                vip_alu = false;
                chip48_jmp = false;
                chip48_shf = true;
                mode = chip8::ls_mode::chip48_ls;
            }
            ImGui::SameLine();
            if (ImGui::Button("Ok")) {
                GLuint texture_id;
                glGenTextures(1, &texture_id);
                glBindTexture(GL_TEXTURE_2D, texture_id);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, chip8::VIDEO_WIDTH, chip8::VIDEO_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

                glBindTexture(GL_TEXTURE_2D, 0);

                MemoryEditor mem_editor;
                mem_editor.ReadOnly = true;

                instances.insert(instances.begin() + pos,
                                 instance {chip8(vip_alu, chip48_jmp, chip48_shf, mode), pos, texture_id, ImGuiFileDialog(), mem_editor, 0, false,
                                           false, false, std::deque<std::string> {}, op_log_default_size, true, false, true, true, true, true});
                ImGui::CloseCurrentPopup();
                vip_alu = false;
                chip48_jmp = false;
                chip48_shf = true;
                mode = chip8::ls_mode::chip48_ls;
            }
            ImGui::EndPopup();
        }

        for (auto& instance: instances) {
            chip8& interpreter = instance.interpreter;
            const std::string& id = std::to_string(instance.id);
            const GLuint& tex_id = instance.tex_id;
            ImGuiFileDialog& file_dlg = instance.file_dlg;
            MemoryEditor& mem_edit = instance.mem_edit;
            unsigned char& ipf = instance.ipf;
            bool& running = instance.running;
            bool& loaded = instance.loaded;
            auto& op_log = instance.op_log;
            unsigned char& op_log_max = instance.op_log_max;

            bool& show_controller = instance.show_controller;
            bool& show_file_dlg = instance.show_file_dlg;
            bool& show_fb = instance.show_fb;
            bool& show_cpu_view = instance.show_cpu_view;
            bool& show_mem_view = instance.show_mem_view;
            bool& show_op_log = instance.show_op_log;

            if (running) {
                for (unsigned char i = 0; i < ipf; ++i) {
                    interpreter.run_cycle();
                    if (interpreter.get_st() == 0) {
                        // hang
                    }
                    if (op_log_max != 0) {
                        while (op_log.size() >= op_log_max) { op_log.pop_front(); }
                        op_log.emplace_back(interpreter.get_instruction());
                    } else { op_log.clear(); }
                }
                interpreter.decrement_timers();
            }
            if (input_p1 != nullptr) {
                for (decltype(keypad_1)::size_type i = 0; i < keypad_1.size(); ++i) {
                    input_p1->interpreter.keys[i] = ImGui::IsKeyDown(keypad_1[i]);
                }
            }
            if (input_p2 != nullptr) {
                for (decltype(keypad_2)::size_type i = 0; i < keypad_2.size(); ++i) {
                    input_p2->interpreter.keys[i] = ImGui::IsKeyDown(keypad_2[i]);
                }
            }
            if (show_controller) {
                ImGui::SetNextWindowSize(ImVec2(355, 111), ImGuiCond_Once);
                if (ImGui::Begin(("controller " + id).c_str(), &show_controller)) {
                    constexpr unsigned char min = 0;
                    constexpr unsigned char max = 30;
                    ImGui::SliderScalar("Emulation speed", ImGuiDataType_U8, &ipf, &min, &max, "%u ipf");
                    const auto ipf_running = running ? ipf : 0;
                    ImGui::Text("IPF: %u", ipf_running);
                    ImGui::Text("IPS: %f", static_cast<float>(ipf_running) * io.Framerate);
                    ImGui::BeginDisabled(!loaded);
                    {
                        if (running) {
                            if (ImGui::Button("Stop")) { running = false; }
                        } else {
                            if (ImGui::Button("Run")) { running = true; }
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Step")) {
                            interpreter.run_cycle();
                            if (op_log_max != 0) {
                                while (op_log.size() >= op_log_max) { op_log.pop_front(); }
                                op_log.emplace_back(interpreter.get_instruction());
                            } else { op_log.clear(); }
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Reset")) {
                            op_log.clear();
                            interpreter.reset();
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Reset + Stop")) {
                            op_log.clear();
                            interpreter.reset();
                            running = false;
                        }
                        ImGui::EndDisabled();
                    }
                }
                ImGui::End();
            }
            if (file_dlg.Display("dlg_key_" + id, ImGuiWindowFlags_NoCollapse, ImVec2(625, 435))) {
                if (file_dlg.IsOk()) {
                    running = false;
                    op_log.clear();
                    interpreter.unload_rom();
                    interpreter.load_rom(file_dlg.GetFilePathName());
                    loaded = true;
                }
                file_dlg.Close();
                show_file_dlg = false;
            }
            if (show_fb) {
                if (interpreter.draw_flag) {
#if defined(GL_UNPACK_ROW_LENGHT) && !defined(__EMSCRIPTEM__)
                    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
                    glBindTexture(GL_TEXTURE_2D, tex_id);
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, chip8::VIDEO_WIDTH, chip8::VIDEO_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE,
                                    interpreter.get_fb().data());
                    glBindTexture(GL_TEXTURE_2D, 0);

                    interpreter.draw_flag = false;
                }

                ImGui::SetNextWindowSizeConstraints(ImVec2(chip8::VIDEO_WIDTH, chip8::VIDEO_HEIGHT + ImGui::GetFrameHeight()),
                                                    ImVec2(FLT_MAX, FLT_MAX), [](ImGuiSizeCallbackData* data) {
                            data->DesiredSize = ImVec2(data->DesiredSize.x, data->DesiredSize.x * 0.5f + ImGui::GetFrameHeight());
                        });
                if (ImGui::Begin(("frame_buffer " + id).c_str(), &show_fb)) {
                    ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(tex_id)),
                                 ImGui::GetContentRegionAvail()); // NOLINT(*-pro-type-reinterpret-cast)
                }
                ImGui::End();
            }
            if (show_cpu_view) {
                ImGui::SetNextWindowSize(ImVec2(152, 425), ImGuiCond_Once);
                if (ImGui::Begin(("cpu_view " + id).c_str(), &show_cpu_view)) {
                    if (ImGui::BeginTable("registers", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders, ImVec2(60.0f, 0.0f))) {
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
                    if (ImGui::BeginTable("stack", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders, ImVec2(68.0f, 0.0f))) {
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
                    if (ImGui::BeginChild("##", ImVec2(0, 0), true)) {
                        ImGui::Text("PC: %X", interpreter.get_pc());
                        ImGui::Text("IR: %X", interpreter.get_ir());
                        ImGui::Text("SP: %X", interpreter.get_sp());
                        ImGui::Text("DT: %X", interpreter.get_dt());
                        ImGui::Text("ST: %X", interpreter.get_st());
                    }
                    ImGui::EndChild();
                }
                ImGui::End();
            }
            if (show_mem_view) {
                if (ImGui::Begin(("mem_view " + id).c_str(), &show_mem_view)) {
                    mem_edit.HighlightMin = interpreter.get_pc();
                    mem_edit.HighlightMax = interpreter.get_pc() + chip8::INSTRUCTION_SIZE;
                    mem_edit.DrawContents((void*) interpreter.get_mem().data(), chip8::MEM_SIZE);
                }
                ImGui::End();
            }
            if (show_op_log) {
                ImGui::SetNextWindowSize(ImVec2(250, static_cast<float>(58 + op_log_max * 17)), ImGuiCond_Always);
                if (ImGui::Begin(("op_log " + id).c_str())) {
                    constexpr unsigned char buf_size = 2 + 1;
                    //char buf[buf_size];
                    std::array<char, buf_size> buf {};
                    snprintf(buf.data(), buf_size, "%u", op_log_max);
                    ImGui::PushItemWidth(22);
                    ImGui::InputText("Number of shown instructions", buf.data(), buf_size);
                    ImGui::PopItemWidth();
                    ImGui::Separator();
                    try {
                        op_log_max = std::stoi(buf.data());
                    } catch (const std::invalid_argument& e) {
                        op_log_max = op_log_default_size;
                    } catch (const std::out_of_range& e) {
                        op_log_max = op_log_default_size;
                    }
                    for (const auto& opcode: op_log) {
                        ImGui::Text("%s", opcode.c_str());
                    }
                }
                ImGui::End();
            }
        }

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

        glfwMakeContextCurrent(window);
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
