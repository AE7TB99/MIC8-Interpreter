#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <chrono>

class chip8 {
public:
    static constexpr std::size_t MEM_SIZE{0x1000};
    static constexpr std::size_t ROM_ADDR{0x200};
    static constexpr std::size_t FONTSET_SIZE{0x50};
    static constexpr std::size_t FONTSET_ADDR{0x50};
    static constexpr std::size_t REG_COUNT{0x10};
    static constexpr std::size_t STACK_SIZE{0x10};
    static constexpr std::size_t INSTRUCTION_SIZE{2};
    static constexpr std::size_t KEY_COUNT{0x10};
    static constexpr std::size_t VIDEO_WIDTH{64};
    static constexpr std::size_t VIDEO_HEIGHT{32};

    enum class ls_mode : unsigned char {
        chip8_ls,
        chip48_ls,
        schip11_ls
    };

    using alt_t = struct {
        bool vip_alu {};
        bool chip48_jmp {};
        bool chip48_shf {true};
        ls_mode ls_mode {ls_mode::chip48_ls};
    };

    std::array<bool, KEY_COUNT> keys {};
    bool drw_flag {true};

    explicit chip8(alt_t alt_ops);

    [[nodiscard]] constexpr auto get_instruction() const -> std::string { return instruction; }
    [[nodiscard]] constexpr auto get_mem() const -> std::span<const std::uint8_t> { return mem; }
    [[nodiscard]] constexpr auto get_fb() const -> std::span<const std::uint32_t> { return fb; }
    [[nodiscard]] constexpr auto get_stack() const -> std::span<const std::uint16_t> { return stack; }
    [[nodiscard]] constexpr auto get_reg() const -> std::span<const std::uint8_t> { return reg; }
    [[nodiscard]] constexpr auto get_pc() const -> std::uint16_t { return pc; }
    [[nodiscard]] constexpr auto get_ir() const -> std::uint16_t { return ir; }
    [[nodiscard]] constexpr auto get_sp() const -> std::uint8_t { return sp; }
    [[nodiscard]] constexpr auto get_dt() const -> std::uint8_t { return dt; }
    [[nodiscard]] constexpr auto get_st() const -> std::uint8_t { return st; }
    [[nodiscard]] constexpr auto get_halt_flag() const -> bool { return hlt_flag; }

    void run_cycle();
    void decrement_timers();
    void reset();
    void load_rom(std::string_view path);
    void unload_rom();

private:
    using op_t = void (chip8::*)(std::uint16_t);

    std::default_random_engine rng{std::random_device{}()};
    std::string instruction;

    std::array<std::uint8_t, MEM_SIZE> mem = [] consteval {
        auto m = decltype(mem){};
        std::array<std::uint8_t, FONTSET_SIZE> fontset{
                //@formatter:off
            0xF0, 0x90, 0x90, 0x90, 0xF0,
            0x20, 0x60, 0x20, 0x20, 0x70,
            0xF0, 0x10, 0xF0, 0x80, 0xF0,
            0xF0, 0x10, 0xF0, 0x10, 0xF0,
            0x90, 0x90, 0xF0, 0x10, 0x10,
            0xF0, 0x80, 0xF0, 0x10, 0xF0,
            0xF0, 0x80, 0xF0, 0x90, 0xF0,
            0xF0, 0x10, 0x20, 0x40, 0x40,
            0xF0, 0x90, 0xF0, 0x90, 0xF0,
            0xF0, 0x90, 0xF0, 0x10, 0xF0,
            0xF0, 0x90, 0xF0, 0x90, 0x90,
            0xE0, 0x90, 0xE0, 0x90, 0xE0,
            0xF0, 0x80, 0x80, 0x80, 0xF0,
            0xE0, 0x90, 0x90, 0x90, 0xE0,
            0xF0, 0x80, 0xF0, 0x80, 0xF0,
            0xF0, 0x80, 0xF0, 0x80, 0x80
            //@formatter:on
        };
        std::ranges::copy(fontset, m.begin() + FONTSET_ADDR);
        return m;
    }();

    std::array<std::uint32_t, VIDEO_WIDTH * VIDEO_HEIGHT> fb{};
    std::array<std::uint16_t, STACK_SIZE> stack{};
    std::array<std::uint8_t, REG_COUNT> reg{};
    std::uint16_t pc = {ROM_ADDR};
    std::uint16_t ir{};
    std::uint8_t sp{};
    std::uint8_t dt{};
    std::uint8_t st{};

    bool hlt_flag {false};

    void op_arr_0(std::uint16_t opcode);
    void op_arr_8(std::uint16_t opcode);
    void op_arr_E(std::uint16_t opcode);
    void op_arr_F(std::uint16_t opcode);

    void op_null(std::uint16_t opcode);
    void op_00E0(std::uint16_t opcode);
    void op_00EE(std::uint16_t opcode);
    void op_1nnn(std::uint16_t opcode);
    void op_2nnn(std::uint16_t opcode);
    void op_3xnn(std::uint16_t opcode);
    void op_4xnn(std::uint16_t opcode);
    void op_5xy0(std::uint16_t opcode);
    void op_6xnn(std::uint16_t opcode);
    void op_7xnn(std::uint16_t opcode);
    void op_8xy0(std::uint16_t opcode);
    void op_8xy1(std::uint16_t opcode);
    void op_8xy2(std::uint16_t opcode);
    void op_8xy3(std::uint16_t opcode);
    void op_8xy4(std::uint16_t opcode);
    void op_8xy5(std::uint16_t opcode);
    void op_8xy6(std::uint16_t opcode);
    void op_8xy7(std::uint16_t opcode);
    void op_8xyE(std::uint16_t opcode);
    void op_9xy0(std::uint16_t opcode);
    void op_Annn(std::uint16_t opcode);
    void op_Bnnn(std::uint16_t opcode);
    void op_Cxnn(std::uint16_t opcode);
    void op_Dxyn(std::uint16_t opcode);
    void op_Ex9E(std::uint16_t opcode);
    void op_ExA1(std::uint16_t opcode);
    void op_Fx07(std::uint16_t opcode);
    void op_Fx0A(std::uint16_t opcode);
    void op_Fx15(std::uint16_t opcode);
    void op_Fx18(std::uint16_t opcode);
    void op_Fx1E(std::uint16_t opcode);
    void op_Fx29(std::uint16_t opcode);
    void op_Fx33(std::uint16_t opcode);
    void op_Fx55(std::uint16_t opcode);
    void op_Fx65(std::uint16_t opcode);

    void op_8xy1_VIP(std::uint16_t opcode);
    void op_8xy2_VIP(std::uint16_t opcode);
    void op_8xy3_VIP(std::uint16_t opcode);

    void op_8xy6_CHIP48(std::uint16_t opcode);
    void op_8xyE_CHIP48(std::uint16_t opcode);
    void op_Bxnn_CHIP48(std::uint16_t opcode);
    void op_Fx55_CHIP48(std::uint16_t opcode);
    void op_Fx65_CHIP48(std::uint16_t opcode);

    void op_Fx55_SCHIP11(std::uint16_t opcode);
    void op_Fx65_SCHIP11(std::uint16_t opcode);

    std::array<op_t, 0xF + 1> OP_ARR_MAIN = [] consteval {
        auto OP_ARR = decltype(OP_ARR_MAIN){};
        OP_ARR.fill(&chip8::op_null);
        OP_ARR[0x0] = &chip8::op_arr_0;
        OP_ARR[0x1] = &chip8::op_1nnn;
        OP_ARR[0x2] = &chip8::op_2nnn;
        OP_ARR[0x3] = &chip8::op_3xnn;
        OP_ARR[0x4] = &chip8::op_4xnn;
        OP_ARR[0x5] = &chip8::op_5xy0;
        OP_ARR[0x6] = &chip8::op_6xnn;
        OP_ARR[0x7] = &chip8::op_7xnn;
        OP_ARR[0x8] = &chip8::op_arr_8;
        OP_ARR[0x9] = &chip8::op_9xy0;
        OP_ARR[0xA] = &chip8::op_Annn;
        OP_ARR[0xB] = &chip8::op_Bnnn;
        OP_ARR[0xC] = &chip8::op_Cxnn;
        OP_ARR[0xD] = &chip8::op_Dxyn;
        OP_ARR[0xE] = &chip8::op_arr_E;
        OP_ARR[0xF] = &chip8::op_arr_F;
        return OP_ARR;
    }();

    std::array<op_t, 0xEE + 1> OP_ARR_0 = [] consteval {
        auto OP_ARR = decltype(OP_ARR_0){};
        OP_ARR.fill(&chip8::op_null);
        OP_ARR[0xE0] = &chip8::op_00E0;
        OP_ARR[0xEE] = &chip8::op_00EE;
        return OP_ARR;
    }();

    std::array<op_t, 0xE + 1> OP_ARR_8 = [] consteval {
        auto OP_ARR = decltype(OP_ARR_8){};
        OP_ARR.fill(&chip8::op_null);
        OP_ARR[0x0] = &chip8::op_8xy0;
        OP_ARR[0x1] = &chip8::op_8xy1;
        OP_ARR[0x2] = &chip8::op_8xy2;
        OP_ARR[0x3] = &chip8::op_8xy3;
        OP_ARR[0x4] = &chip8::op_8xy4;
        OP_ARR[0x5] = &chip8::op_8xy5;
        OP_ARR[0x6] = &chip8::op_8xy6;
        OP_ARR[0x7] = &chip8::op_8xy7;
        OP_ARR[0xE] = &chip8::op_8xyE;
        return OP_ARR;
    }();

    std::array<op_t, 0xE + 1> OP_ARR_E = [] consteval {
        auto OP_ARR = decltype(OP_ARR_E){};
        OP_ARR.fill(&chip8::op_null);
        OP_ARR[0x1] = &chip8::op_ExA1;
        OP_ARR[0xE] = &chip8::op_Ex9E;
        return OP_ARR;
    }();

    std::array<op_t, 0x65 + 1> OP_ARR_F = [] consteval {
        auto OP_ARR = decltype(OP_ARR_F){};
        OP_ARR.fill(&chip8::op_null);
        OP_ARR[0x07] = &chip8::op_Fx07;
        OP_ARR[0x0A] = &chip8::op_Fx0A;
        OP_ARR[0x15] = &chip8::op_Fx15;
        OP_ARR[0x18] = &chip8::op_Fx18;
        OP_ARR[0x1E] = &chip8::op_Fx1E;
        OP_ARR[0x29] = &chip8::op_Fx29;
        OP_ARR[0x33] = &chip8::op_Fx33;
        OP_ARR[0x55] = &chip8::op_Fx55;
        OP_ARR[0x65] = &chip8::op_Fx65;
        return OP_ARR;
    }();
};
