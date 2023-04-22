#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include "chip8.hpp"
#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GLFW/glfw3.h>
#include <iostream>
#include "imgui_memory_editor.h"
#include "ImGuiFileDialog.h"

static inline constexpr std::uint8_t VIEW_COUNT = 6;

enum views {
    rom,
    load_rom,
    fb,
    mem_view,
    cpu_view,
    disassembler
};

static std::vector<std::tuple<chip8, std::string, GLuint, ImGuiFileDialog, MemoryEditor, std::array<bool, VIEW_COUNT>>> instances {};

static auto instance_search() -> std::uint8_t {
    int l = 0;
    int r = instances.size() - 1;
    while(l <= r) {
        int m = l + (r - 1) / 2;
        auto& [_, str, __, ___, ____, _____] = instances[m];
        std::uint8_t m_val;
        try {
            m_val = std::stoi(str);
        } catch (const std::invalid_argument& e) {
            ++l;
            continue;
        } catch (const std::out_of_range& e) {
            ++l;
            continue;
        }
        if(m_val == m) l = m + 1; 
        else r = m - 1;
    }
    return l;
}

static void glfw_error_callback(int code, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", code, description);
}

auto main(int, char** argv) -> int {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;

    GLFWwindow* window = glfwCreateWindow(1280, 720, "MIC8 Interpreter", nullptr, nullptr);
    if (window == nullptr) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    //ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    
    bool creation_popup {false};

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        if(ImGui::BeginMainMenuBar()) {
            if(ImGui::BeginMenu("Instances")) {
                if(ImGui::MenuItem("Create New Instance")) creation_popup = true;
                if(ImGui::BeginMenu("Delete Instances")) {
                    for (auto it = instances.begin(); it != instances.end();) {
                        if(ImGui::MenuItem(("Delete Instance " + std::get<std::string>(*it)).c_str())){
                            if(it == --instances.end()) instances.erase(it);
                            else instances.erase(it++);
                        }
                        else ++it;
                    }
                    ImGui::EndMenu();                    
                }
                ImGui::EndMenu();
            }        
            if(ImGui::BeginMenu("Views")) {
                if(ImGui::BeginMenu("Frame Buffers")) {
                    for(auto& instance : instances) ImGui::MenuItem(("Frame Buffer " + std::get<std::string>(instance)).c_str(), nullptr, &std::get<std::array<bool, VIEW_COUNT>>(instance)[fb]);
                    ImGui::EndMenu();
                }
                if(ImGui::BeginMenu("Memory Viewers")) {
                    for(auto& instance : instances) ImGui::MenuItem(("Memory View " + std::get<std::string>(instance)).c_str(), nullptr, &std::get<std::array<bool, VIEW_COUNT>>(instance)[mem_view]);
                    ImGui::EndMenu();
                }
                if(ImGui::BeginMenu("CPU viewers")) {
                    for(auto& instance : instances) ImGui::MenuItem(("CPU View " + std::get<std::string>(instance)).c_str(), nullptr, &std::get<std::array<bool, VIEW_COUNT>>(instance)[cpu_view]);
                    ImGui::EndMenu();
                }
                if(ImGui::BeginMenu("Disassembler")) {
                    for(auto& instance : instances) ImGui::MenuItem(("Disassembler " + std::get<std::string>(instance)).c_str(), nullptr, &std::get<std::array<bool, VIEW_COUNT>>(instance)[disassembler]);
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("ROMS")) {
                for(auto& instance : instances) {
                    if(ImGui::MenuItem(("Load ROM into instance " + std::get<std::string>(instance)).c_str(), nullptr, &std::get<std::array<bool, VIEW_COUNT>>(instance)[load_rom])) {
                        std::get<ImGuiFileDialog>(instance).OpenDialog("dlg_key_" + std::get<std::string>(instance), "Load ROM into instance " + std::get<std::string>(instance), ".ch8", ".");
                    }                    
                }
                ImGui::EndMenu();
            }
            if(ImGui::MenuItem("EXIT")) glfwSetWindowShouldClose(window, true);
            ImGui::EndMainMenuBar();
        }
        
        if(creation_popup) {
            ImGui::OpenPopup("Create Instance");
            creation_popup = false;
        }
        if(ImGui::BeginPopupModal("Create Instance")) {
            std::uint8_t name = instance_search();
        
            static bool chip48_jmp {false};
            static bool chip48_shf {true};

            static chip8::ls_mode mode {chip8::ls_mode::chip48_ls};
        
            ImGui::Text("Alternate Instrucions:");
            ImGui::Separator();
            ImGui::Checkbox("CHIP48 Jump", &chip48_jmp);        
            ImGui::Checkbox("CHIP48 Shift", &chip48_shf);
            ImGui::Separator();
            if(ImGui::RadioButton("CHIP8 Load/Store", mode == chip8::ls_mode::chip8_ls)) mode = chip8::ls_mode::chip8_ls;        
            if(ImGui::RadioButton("CHIP48 Load/Store", mode == chip8::ls_mode::chip48_ls)) mode = chip8::ls_mode::chip48_ls;        
            if(ImGui::RadioButton("SUPER-CHIP 1.1 Load/Store", mode == chip8::ls_mode::schip_ls)) mode = chip8::ls_mode::schip_ls;
            if(ImGui::Button("Close")) {
                ImGui::CloseCurrentPopup();
                chip48_jmp = false;
                chip48_shf = true;
                mode = chip8::ls_mode::chip48_ls;
            }
            ImGui::SameLine();
            if(ImGui::Button("Ok")){
                instances.insert(instances.begin() + name, {chip8(chip48_jmp, chip48_shf, mode), std::to_string(name), GLuint{}, ImGuiFileDialog(), MemoryEditor(), std::array<bool,VIEW_COUNT>{}});
                ImGui::CloseCurrentPopup();
                std::get<MemoryEditor>(instances[name]).ReadOnly = true;
                //std::get<MemoryEditor>(instances[name]).;
                chip48_jmp = false;
                chip48_shf = true;
                mode = chip8::ls_mode::chip48_ls;
            }
            ImGui::EndPopup();
        }

        ImGui::ShowMetricsWindow();

        for(auto& instance : instances) {
            if(std::get<std::array<bool, VIEW_COUNT>>(instance)[rom]) std::get<chip8>(instance).run_cycle();
            if(std::get<ImGuiFileDialog>(instance).Display("dlg_key_" + std::get<std::string>(instance))) {
                if(std::get<ImGuiFileDialog>(instance).IsOk()) {
                    std::get<chip8>(instance).unload_rom();
                    std::get<chip8>(instance).load_rom(std::get<ImGuiFileDialog>(instance).GetFilePathName());
                    std::get<std::array<bool,VIEW_COUNT>>(instance)[rom] = true;
                }
                std::get<ImGuiFileDialog>(instance).Close();
                std::get<std::array<bool, VIEW_COUNT>>(instance)[load_rom] = false;
            }
            if(std::get<std::array<bool, VIEW_COUNT>>(instance)[fb]) {     
                if(std::get<chip8>(instance).draw_flag) {
                    glGenTextures(1, &std::get<GLuint>(instance));
                    glBindTexture(GL_TEXTURE_2D, std::get<GLuint>(instance));

                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                    #if defined(GL_UNPACK_ROW_LENGHT) && !defined(__EMSCRIPTEM__)
                        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
                    #endif
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, chip8::VIDEO_WIDTH, chip8::VIDEO_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, std::get<chip8>(instance).get_fb().data());

                    std::get<chip8>(instance).draw_flag = false;
                }
  
                ImGui::Begin(("frame_buffer " + std::get<std::string>(instance)).c_str(), &std::get<std::array<bool, VIEW_COUNT>>(instance)[fb]);
                //ImGui::SetWindowSize(ImVec2(ImGui::GetWindowWidth(), ImGui::GetWindowWidth() / 2 + ImGui::GetFrameHeight()));
                ImGui::SetWindowSize(ImVec2((ImGui::GetWindowHeight() - ImGui::GetFrameHeight()) * 2, ImGui::GetWindowHeight()));
                ImGui::Image((void*)(intptr_t)std::get<GLuint>(instance), ImGui::GetContentRegionAvail());
                ImGui::End();
            }
            if(std::get<std::array<bool, VIEW_COUNT>>(instance)[mem_view]) {
                ImGui::Begin(("mem_view " + std::get<std::string>(instance)).c_str(), &std::get<std::array<bool, VIEW_COUNT>>(instance)[mem_view]);
                std::get<MemoryEditor>(instance).DrawContents((void*)std::get<chip8>(instance).get_mem().data(), std::get<chip8>(instance).MEM_SIZE);
                ImGui::End();
            }
            if(std::get<std::array<bool, VIEW_COUNT>>(instance)[cpu_view]) {
                ImGui::Begin(("cpu_view  " + std::get<std::string>(instance)).c_str(), &std::get<std::array<bool, VIEW_COUNT>>(instance)[cpu_view]);
                    if(ImGui::BeginTable("registers", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders, ImVec2(60.0f, 0.0f))) {
                        ImGui::TableSetupColumn("REG");
                        ImGui::TableSetupColumn("VAL");
                        ImGui::TableHeadersRow();
                        for(int i = 0; const auto& reg : std::get<chip8>(instance).get_reg()) {
                            ImGui::TableNextColumn();
                            ImGui::Text("V%X", i);
                            ImGui::TableNextColumn();
                            ImGui::Text("%d", reg);
                            ++i;
                        }
                        ImGui::EndTable();
                    }
                    ImGui::SameLine();
                    if(ImGui::BeginTable("stack", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders, ImVec2(63.0f, 0.0f))) {
                        ImGui::TableSetupColumn("SP");
                        ImGui::TableSetupColumn("ADDR");
                        ImGui::TableHeadersRow();
                        for(int i = 0; const auto& addr : std::get<chip8>(instance).get_stack()) {
                            ImGui::TableNextColumn();
                            ImGui::Text("%x", i);
                            ImGui::TableNextColumn();
                            ImGui::Text("%04x", addr);
                            ++i;
                        }
                        ImGui::EndTable();
                        ImGui::Text("PC: %X", std::get<chip8>(instance).get_pc());
                        ImGui::Text("IR: %X", std::get<chip8>(instance).get_ir());
                        ImGui::Text("SP: %X", std::get<chip8>(instance).get_sp());
                        ImGui::Text("DT: %X", std::get<chip8>(instance).get_dt());
                        ImGui::Text("ST: %X", std::get<chip8>(instance).get_st());
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
