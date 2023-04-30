#include "chip8.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <stdexcept>
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include "ImGuiFileDialog.h"
#include "imgui_memory_editor.h"
#include <GLFW/glfw3.h>
#include <queue>

namespace {
    enum member_index : std::uint8_t {
        INTERPRETER,
        ID,
        TEX_ID,
        FILE_DLG,
        MEM_EDIT,
        IPF,
        RUNNING,
        LOADED,
        INPUT,
        OP_LOG,
        VIEWS
    };

    enum view_index: std::uint8_t {
        SHOW_CONTROLLER,
        SHOW_FILE_DLG,
        SHOW_FB,
        SHOW_CPU_VIEW,
        SHOW_MEM_VIEW,
        SHOW_OP_LOG,
        VIEW_COUNT
    };

    std::vector<std::tuple<chip8, std::string, GLuint, ImGuiFileDialog, MemoryEditor, std::uint8_t, bool, bool, bool, std::deque<std::string>, std::array<bool, VIEW_COUNT>>> instances {};

    decltype(instances.data()) input_p1 { nullptr };
    decltype(instances.data()) input_p2 { nullptr };

    std::uint8_t input_count {0};

    inline constexpr std::uint8_t OP_LOG_MAX = 40;

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

    auto instance_search() -> std::uint8_t {
        int l = 0, r = instances.size() - 1;
        while (l <= r) {
            int m = l + (r - 1) / 2;
            auto& n = std::get<ID>(instances[m]);
            std::uint8_t m_val;
            try {
                m_val = std::stoi(n);
            } catch (const std::invalid_argument& e) {
                ++l;
                continue;
            } catch (const std::out_of_range& e) {
                ++l;
                continue;
            } if (m_val == m) l = m + 1;
            else r = m - 1;
        }
        return l;
    }
    
    void help_marker(const char* desc)
    {
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) && ImGui::BeginTooltip())
        {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    void add_input(decltype(instances)::value_type& instance) {
        switch (input_count) {
            case 0: input_p1 = &instance; break;
            case 1: input_p2 = &instance;
        }
        ++input_count;
    }

    void remove_input(decltype(instances)::value_type& instance) {
        switch (input_count) {
            case 1: input_p1 = nullptr; input_p2 = nullptr; break;
            case 2: if (input_p1 == &instance) input_p1 = input_p2; input_p2 = nullptr;
        }
        --input_count;
    }

    void glfw_error_callback(int code, const char* description) {
        fprintf(stderr, "GLFW Error %d: %s\n", code, description);
    }
}

auto main(int, char** argv) -> int {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    GLFWwindow* window = glfwCreateWindow(1280, 720, "MIC8 Interpreter", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();

    ImVec4 clear_color = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    
    bool creation_popup { false };

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Instances")) {
                if (ImGui::MenuItem("Create New Instance")) creation_popup = true;
                if (ImGui::BeginMenu("Delete Instances")) {
                    for (auto it = instances.begin(); it != instances.end();) {
                        if (ImGui::MenuItem(("Delete Instance " + std::get<ID>(*it)).c_str())) {
                            if (std::get<INPUT>(*it)) --input_count;
                            if (it == --instances.end()) instances.erase(it);
                            else instances.erase(it++);
                        } else ++it;
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Views")) {
                if (ImGui::BeginMenu("Controllers")) {
                    for (auto& instance : instances) ImGui::MenuItem(("Controller " + std::get<ID>(instance)).c_str(), nullptr, &std::get<VIEWS>(instance)[SHOW_CONTROLLER]);
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Frame Buffers")) {
                    for (auto& instance : instances) ImGui::MenuItem(("Frame Buffer " + std::get<ID>(instance)).c_str(), nullptr, &std::get<VIEWS>(instance)[SHOW_FB]);
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Memory Viewers")) {
                    for (auto& instance : instances) ImGui::MenuItem(("Memory View " + std::get<ID>(instance)).c_str(), nullptr, &std::get<VIEWS>(instance)[SHOW_MEM_VIEW]);
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("CPU viewers")) {
                    for (auto& instance : instances) ImGui::MenuItem(("CPU View " + std::get<ID>(instance)).c_str(), nullptr, &std::get<VIEWS>(instance)[SHOW_CPU_VIEW]);
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Logs")) {
                    for (auto& instance : instances) ImGui::MenuItem(("Log " + std::get<ID>(instance)).c_str(), nullptr, &std::get<VIEWS>(instance)[SHOW_OP_LOG]);
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("ROMS")) {
                for (auto& instance : instances) {
                    const std::string& id = std::get<ID>(instance);
                    if (ImGui::MenuItem(("Load ROM into instance " + id).c_str(), nullptr, &std::get<VIEWS>(instance)[SHOW_FILE_DLG])) {
                        std::get<FILE_DLG>(instance).OpenDialog("dlg_key_" + id, "Load ROM into instance " + id, ".ch8", "./roms/");
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Input")) {
                for (auto& instance : instances) {
                    const std::string& id = std::get<ID>(instance);
                    bool& input = std::get<INPUT>(instance);
                    if (input_count < 2) {
                        if (ImGui::MenuItem(("Toggle input for instance " + id).c_str(), nullptr, &input)) {
                            if (input) add_input(instance);
                            else remove_input(instance);
                        }
                    }
                    else if (input) {
                        if (ImGui::MenuItem(("Toggle input for instance " + id).c_str(), nullptr, &input)) {
                            remove_input(instance); 
                        }
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("EXIT")) glfwSetWindowShouldClose(window, true);
            ImGui::EndMainMenuBar();
        }

        if (creation_popup) {
            ImGui::SetNextWindowSize(ImVec2(245, 213));
            ImGui::OpenPopup("Create Instance");
            creation_popup = false;
        }
        if (ImGui::BeginPopupModal("Create Instance", nullptr, ImGuiWindowFlags_NoResize)) {
            const std::uint8_t pos = instance_search();
            static bool chip48_jmp { false };
            static bool chip48_shf { true };
            static chip8::ls_mode mode { chip8::ls_mode::chip48_ls };

            ImGui::SeparatorText("Alternative Instrucions");
            ImGui::Checkbox("CHIP48 Jump", &chip48_jmp);
            ImGui::SameLine();
            help_marker("BNNN is replaced by BXNN, which jumps to address XNN + the value in VX (instead of address NNN + the value in V0)");
            ImGui::Checkbox("CHIP48 Shift", &chip48_shf);
            ImGui::SameLine();
            help_marker("8XY6 / 8XYE shift VX and ignore VY");
            ImGui::Separator();
            if (ImGui::RadioButton("CHIP8 Load/Store", mode == chip8::ls_mode::chip8_ls)) mode = chip8::ls_mode::chip8_ls;
            if (ImGui::RadioButton("CHIP48 Load/Store", mode == chip8::ls_mode::chip48_ls)) mode = chip8::ls_mode::chip48_ls;
            ImGui::SameLine();
            help_marker("FX55 / FX65 increment I by one less than they should (if X is 0, I is not incremented at all)");
            if (ImGui::RadioButton("SUPER-CHIP 1.1 Load/Store", mode == chip8::ls_mode::schip_ls)) mode = chip8::ls_mode::schip_ls;
            ImGui::SameLine();
            help_marker("FX55 / FX65 no longer increment I at all");
            ImGui::Separator();
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
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

                instances.insert(instances.begin() + pos, { chip8(chip48_jmp, chip48_shf, mode), std::to_string(pos), texture_id, ImGuiFileDialog(), mem_editor, 0, false, false, false, std::deque<std::string> {}, std::array<bool, VIEW_COUNT> {} });
                ImGui::CloseCurrentPopup();
                chip48_jmp = false;
                chip48_shf = true;
                mode = chip8::ls_mode::chip48_ls;
            }
            ImGui::EndPopup();
        }

        for (auto& instance : instances) {
            chip8& interpreter = std::get<INTERPRETER>(instance);
            const std::string& id = std::get<ID>(instance);
            const GLuint& tex_id = std::get<TEX_ID>(instance);
            ImGuiFileDialog& file_dlg = std::get<FILE_DLG>(instance);
            MemoryEditor& mem_edit = std::get<MEM_EDIT>(instance);
            std::uint8_t& ipf = std::get<IPF>(instance);
            bool& running = std::get<RUNNING>(instance);
            bool& loaded = std::get<LOADED>(instance);
            auto& op_log = std::get<OP_LOG>(instance);
            auto& views = std::get<VIEWS>(instance);
            
            if (running) {
                for (int i = 0; i < ipf; ++i) {
                    interpreter.run_cycle();
                    if (op_log.size() > OP_LOG_MAX) {
                        op_log.pop_back();
                        op_log.push_front(interpreter.get_instruction().data());
                    }
                    else op_log.push_front(interpreter.get_instruction().data());
                }
                interpreter.decrement_timers();
            }
            if (input_p1 != nullptr) {
                for (int i = 0; i < keypad_1.size(); ++i) {
                    std::get<INTERPRETER>(*input_p1).keys[i] = ImGui::IsKeyDown(keypad_1[i]);
                }
            }
            if (input_p2 != nullptr) {
                for (int i = 0; i < keypad_2.size(); ++i) {
                    std::get<INTERPRETER>(*input_p2).keys[i] = ImGui::IsKeyDown(keypad_2[i]);
                }
            }
            if (views[SHOW_CONTROLLER]) {
                ImGui::SetNextWindowSize(ImVec2(355, 111), ImGuiCond_Once);
                if (ImGui::Begin(("controller " + id).c_str(), &views[SHOW_CONTROLLER])) {
                    const std::uint8_t min = 0u;
                    const std::uint8_t max = 30u;
                    ImGui::SliderScalar("Emulation speed", ImGuiDataType_U8, &ipf, &min, &max, "%u ipf");
                    const auto ipf_running = running ? ipf : 0; 
                    ImGui::Text("IPF: %u", ipf_running);
                    ImGui::Text("IPS: %f", ipf_running * io.Framerate);
                    ImGui::BeginDisabled(!loaded); {
                        if (running) {
                            if (ImGui::Button("Stop")) running = false;
                        } else {
                            if (ImGui::Button("Run")) running = true;
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Step")) interpreter.run_cycle();
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
            if (file_dlg.Display("dlg_key_" + id, ImGuiWindowFlags_NoCollapse, ImVec2(125, 87), ImVec2(625, 435))) {
                if (file_dlg.IsOk()) {
                    running = false;
                    op_log.clear();
                    interpreter.unload_rom();
                    interpreter.load_rom(file_dlg.GetFilePathName());
                    loaded = true;
                }
                file_dlg.Close();
                views[SHOW_FILE_DLG] = false;
            }
            if (views[SHOW_FB]) {
                if (interpreter.draw_flag) {
#if defined(GL_UNPACK_ROW_LENGHT) && !defined(__EMSCRIPTEM__)
                    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
                    glBindTexture(GL_TEXTURE_2D, tex_id);
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, chip8::VIDEO_WIDTH, chip8::VIDEO_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, interpreter.get_fb().data());
                    glBindTexture(GL_TEXTURE_2D, 0);

                    interpreter.draw_flag = false;
                }

                ImGui::SetNextWindowSizeConstraints(ImVec2(chip8::VIDEO_WIDTH, chip8::VIDEO_HEIGHT + ImGui::GetFrameHeight()), ImVec2(FLT_MAX, FLT_MAX), [](ImGuiSizeCallbackData* data) {
                    data->DesiredSize = ImVec2(data->DesiredSize.x, data->DesiredSize.x * 0.5 + ImGui::GetFrameHeight());
                });
                if (ImGui::Begin(("frame_buffer " + id).c_str(), &views[SHOW_FB])) {
                    ImGui::Image((void*)(intptr_t) tex_id, ImGui::GetContentRegionAvail());
                }
                ImGui::End();
            }
            if (views[SHOW_CPU_VIEW]) {
                ImGui::SetNextWindowSize(ImVec2(152, 425), ImGuiCond_Once);
                if (ImGui::Begin(("cpu_view " + id).c_str(), &views[SHOW_CPU_VIEW])) {
                    if (ImGui::BeginTable("registers", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders, ImVec2(60.0f, 0.0f))) {
                        ImGui::TableSetupColumn("REG");
                        ImGui::TableSetupColumn("VAL");
                        ImGui::TableHeadersRow();
                        for (int i = 0; const auto& reg : interpreter.get_reg()) {
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
                        for (int i = 0; const auto& addr : interpreter.get_stack()) {
                            ImGui::TableNextColumn();
                            if (!addr) {
                                ImGui::TextDisabled("%x", i);
                                ImGui::TableNextColumn();
                                ImGui::TextDisabled("%04x", addr);
                            } else if (interpreter.get_sp() != i) {
                                ImGui::Text("%x", i);
                                ImGui::TableNextColumn();
                                ImGui::Text("%04x", addr);
                            } else {
                                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_HeaderHovered));                            
                                ImGui::Text("%x", i);
                                ImGui::TableNextColumn();
                                ImGui::Text("%04x", addr);
                            }
                            ++i;
                        }
                        ImGui::EndTable();
                    }
                    if (ImGui::BeginChild("##", ImVec2(0,0), true)) {
                        ImGui::Text("PC: %X", interpreter.get_pc());
                        ImGui::Text("IR: %X", interpreter.get_ir());
                        ImGui::Text("SP: %X", interpreter.get_st());
                        ImGui::Text("DT: %X", interpreter.get_dt());
                        ImGui::Text("ST: %X", interpreter.get_st());
                    }
                    ImGui::EndChild();
                }
                ImGui::End();
            }
            if (views[SHOW_MEM_VIEW]) {
                if (ImGui::Begin(("mem_view " + id).c_str(), &views[SHOW_MEM_VIEW])) {
                    mem_edit.HighlightMin = interpreter.get_pc();
                    mem_edit.HighlightMax = interpreter.get_pc() + chip8::INSTRUCTION_SIZE;
                    mem_edit.DrawContents((void*)interpreter.get_mem().data(), interpreter.MEM_SIZE);
                }
                ImGui::End();
            }
            if (views[SHOW_OP_LOG]) {
                ImGui::SetNextWindowSize(ImVec2(240, 730), ImGuiCond_Once);
                if (ImGui::Begin(("op_log " + id).c_str())) {
                    for (const auto& op : op_log) {
                        ImGui::Text("%s", op.c_str());
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
