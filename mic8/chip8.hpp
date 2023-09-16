#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <random>
#include <span>

class chip8 {
public:
    static inline constexpr std::uint16_t MEM_SIZE = 0x1000;
    static inline constexpr std::uint16_t ROM_ADDR = 0x200;
    static inline constexpr std::uint8_t FONTSET_SIZE = 0x50;
    static inline constexpr std::uint8_t FONTSET_ADDR = 0x50;
    static inline constexpr std::uint8_t REG_COUNT = 0x10;
    static inline constexpr std::uint8_t STACK_SIZE = 0x10;
    static inline constexpr std::uint8_t INSTRUCTION_SIZE = 2;
    static inline constexpr std::uint8_t KEY_COUNT = 0x10;
    static inline constexpr std::uint8_t VIDEO_WIDTH = 64;
    static inline constexpr std::uint8_t VIDEO_HEIGHT = 32;

    std::array<bool, 16> keys {};
    bool draw_flag {true};

    enum class ls_mode {
        chip8_ls,
        chip48_ls,
        schip_ls
    };

    chip8(bool chip48_jmp, bool chip48_shf, ls_mode mode);

    void run_cycle();
    void decrement_timers();
    void reset();
    void load_rom(std::string_view path);
    void unload_rom();

    [[nodiscard]] inline constexpr auto get_mem() const -> std::span<const std::uint8_t> { return mem; }

    [[nodiscard]] inline constexpr auto get_fb() const -> std::span<const std::uint32_t> { return fb; }

    [[nodiscard]] inline constexpr auto get_stack() const -> std::span<const std::uint16_t> { return stack; }

    [[nodiscard]] inline constexpr auto get_reg() const -> std::span<const std::uint8_t> { return reg; }

    [[nodiscard]] inline constexpr auto get_instruction() const& -> std::string_view { return instruction.data(); }

    [[nodiscard]] inline constexpr auto get_pc() const& -> const std::uint16_t& { return pc; }

    [[nodiscard]] inline constexpr auto get_ir() const& -> const std::uint16_t& { return ir; }

    [[nodiscard]] inline constexpr auto get_sp() const& -> const std::uint8_t& { return sp; }

    [[nodiscard]] inline constexpr auto get_dt() const& -> const std::uint8_t& { return dt; }

    [[nodiscard]] inline constexpr auto get_st() const& -> const std::uint8_t& { return st; }

private:
    using op_t = void (chip8::*)(const std::uint16_t&);

    std::default_random_engine rng {std::random_device {}()};
    std::array<char, 34> instruction {};


    std::array<std::uint8_t, MEM_SIZE> mem = [] consteval {
        auto m = decltype(mem) {};
        std::array<std::uint8_t, FONTSET_SIZE> fontset {
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
        };
        std::copy(fontset.begin(), fontset.end(), m.begin() + FONTSET_ADDR);
        return m;
    }();

    std::array<std::uint32_t, VIDEO_WIDTH * VIDEO_HEIGHT> fb {};
    std::array<std::uint16_t, STACK_SIZE> stack {};
    std::array<std::uint8_t, REG_COUNT> reg {};
    std::uint16_t pc {ROM_ADDR};
    std::uint16_t ir {};
    std::uint8_t sp {};
    std::uint8_t dt {};
    std::uint8_t st {};

    void op_arr_0(const std::uint16_t& opcode);
    void op_arr_8(const std::uint16_t& opcode);
    void op_arr_E(const std::uint16_t& opcode);
    void op_arr_F(const std::uint16_t& opcode);
    void op_null(const std::uint16_t& opcode);
    void op_00E0(const std::uint16_t& opcode);
    void op_00EE(const std::uint16_t& opcode);
    void op_1nnn(const std::uint16_t& opcode);
    void op_2nnn(const std::uint16_t& opcode);
    void op_3xkk(const std::uint16_t& opcode);
    void op_4xkk(const std::uint16_t& opcode);
    void op_5xy0(const std::uint16_t& opcode);
    void op_6xkk(const std::uint16_t& opcode);
    void op_7xkk(const std::uint16_t& opcode);
    void op_8xy0(const std::uint16_t& opcode);
    void op_8xy1(const std::uint16_t& opcode);
    void op_8xy2(const std::uint16_t& opcode);
    void op_8xy3(const std::uint16_t& opcode);
    void op_8xy4(const std::uint16_t& opcode);
    void op_8xy5(const std::uint16_t& opcode);
    void op_8xy6(const std::uint16_t& opcode);
    void op_8xy7(const std::uint16_t& opcode);
    void op_8xyE(const std::uint16_t& opcode);
    void op_9xy0(const std::uint16_t& opcode);
    void op_Annn(const std::uint16_t& opcode);
    void op_Bnnn(const std::uint16_t& opcode);
    void op_Cxkk(const std::uint16_t& opcode);
    void op_Dxyn(const std::uint16_t& opcode);
    void op_Ex9E(const std::uint16_t& opcode);
    void op_ExA1(const std::uint16_t& opcode);
    void op_Fx07(const std::uint16_t& opcode);
    void op_Fx0A(const std::uint16_t& opcode);
    void op_Fx15(const std::uint16_t& opcode);
    void op_Fx18(const std::uint16_t& opcode);
    void op_Fx1E(const std::uint16_t& opcode);
    void op_Fx29(const std::uint16_t& opcode);
    void op_Fx33(const std::uint16_t& opcode);
    void op_Fx55(const std::uint16_t& opcode);
    void op_Fx65(const std::uint16_t& opcode);

    void op_Bxnn_CHIP48(const std::uint16_t& opcode);
    void op_8xy6_CHIP48(const std::uint16_t& opcode);
    void op_8xyE_CHIP48(const std::uint16_t& opcode);
    void op_Fx55_CHIP48(const std::uint16_t& opcode);
    void op_Fx65_CHIP48(const std::uint16_t& opcode);
    void op_Fx55_SCHIP(const std::uint16_t& opcode);
    void op_Fx65_SCHIP(const std::uint16_t& opcode);

    std::array<op_t, 0xF + 1> OP_ARR_MAIN = [] consteval {
        auto OP_ARR = decltype(OP_ARR_MAIN) {};
        OP_ARR.fill(&chip8::op_null);
        OP_ARR[0x0] = &chip8::op_arr_0;
        OP_ARR[0x1] = &chip8::op_1nnn;
        OP_ARR[0x2] = &chip8::op_2nnn;
        OP_ARR[0x3] = &chip8::op_3xkk;
        OP_ARR[0x4] = &chip8::op_4xkk;
        OP_ARR[0x5] = &chip8::op_5xy0;
        OP_ARR[0x6] = &chip8::op_6xkk;
        OP_ARR[0x7] = &chip8::op_7xkk;
        OP_ARR[0x8] = &chip8::op_arr_8;
        OP_ARR[0x9] = &chip8::op_9xy0;
        OP_ARR[0xA] = &chip8::op_Annn;
        OP_ARR[0xB] = &chip8::op_Bnnn;
        OP_ARR[0xC] = &chip8::op_Cxkk;
        OP_ARR[0xD] = &chip8::op_Dxyn;
        OP_ARR[0xE] = &chip8::op_arr_E;
        OP_ARR[0xF] = &chip8::op_arr_F;
        return OP_ARR;
    }();

    std::array<op_t, 0xE + 1> OP_ARR_0 = [] consteval {
        auto OP_ARR = decltype(OP_ARR_0) {};
        OP_ARR.fill(&chip8::op_null);
        OP_ARR[0x0] = &chip8::op_00E0;
        OP_ARR[0xE] = &chip8::op_00EE;
        return OP_ARR;
    }();

    std::array<op_t, 0xE + 1> OP_ARR_8 = [] consteval {
        auto OP_ARR = decltype(OP_ARR_8) {};
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
        auto OP_ARR = decltype(OP_ARR_E) {};
        OP_ARR.fill(&chip8::op_null);
        OP_ARR[0x1] = &chip8::op_ExA1;
        OP_ARR[0xE] = &chip8::op_Ex9E;
        return OP_ARR;
    }();

    std::array<op_t, 0x65 + 1> OP_ARR_F = [] consteval {
        auto OP_ARR = decltype(OP_ARR_F) {};
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
