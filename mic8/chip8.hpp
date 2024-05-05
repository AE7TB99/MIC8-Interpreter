#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <random>
#include <span>
#include <string>
#include <string_view>

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
        bool vip_alu{};
        bool chip48_jmp{};
        bool chip48_shf{true};
        ls_mode ls_mode{ls_mode::chip48_ls};
    };

    std::array<bool, KEY_COUNT> keys{};
    bool drw_flag{true};

    explicit chip8(alt_t alt_ops);

    [[nodiscard]] constexpr auto get_instruction() const -> std::string { return instruction_string; }
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

    auto run_cycle() -> void;

    auto decrement_timers() -> void;

    auto reset() -> void;

    auto load_rom(std::string_view path) -> void;

    auto unload_rom() -> void;

private:
    using op_type = void (chip8::*)();

    std::default_random_engine rng{std::random_device{}()};
    std::string instruction_string;
    std::uint16_t instruction{};

    std::array<std::uint8_t, MEM_SIZE> mem = [] consteval {
        auto mem_ = decltype(mem){};
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
        std::ranges::copy(fontset, mem_.begin() + FONTSET_ADDR);
        return mem_;
    }();

    std::array<std::uint32_t, VIDEO_WIDTH * VIDEO_HEIGHT> fb{};
    std::array<std::uint16_t, STACK_SIZE> stack{};
    std::array<std::uint8_t, REG_COUNT> reg{};
    std::uint16_t pc{ROM_ADDR};
    std::uint16_t ir{};
    std::uint8_t sp{};
    std::uint8_t dt{};
    std::uint8_t st{};

    bool hlt_flag{false};

    void op_arr_0();

    void op_arr_8();

    void op_arr_E();

    void op_arr_F();

    void op_null();

    void op_00E0();

    void op_00EE();

    void op_1nnn();

    void op_2nnn();

    void op_3xnn();

    void op_4xnn();

    void op_5xy0();

    void op_6xnn();

    void op_7xnn();

    void op_8xy0();

    void op_8xy1();

    void op_8xy2();

    void op_8xy3();

    void op_8xy4();

    void op_8xy5();

    void op_8xy6();

    void op_8xy7();

    void op_8xyE();

    void op_9xy0();

    void op_Annn();

    void op_Bnnn();

    void op_Cxnn();

    void op_Dxyn();

    void op_Ex9E();

    void op_ExA1();

    void op_Fx07();

    void op_Fx0A();

    void op_Fx15();

    void op_Fx18();

    void op_Fx1E();

    void op_Fx29();

    void op_Fx33();

    void op_Fx55();

    void op_Fx65();

    void op_8xy1_VIP();

    void op_8xy2_VIP();

    void op_8xy3_VIP();

    void op_8xy6_CHIP48();

    void op_8xyE_CHIP48();

    void op_Bxnn_CHIP48();

    void op_Fx55_CHIP48();

    void op_Fx65_CHIP48();

    void op_Fx55_SCHIP11();

    void op_Fx65_SCHIP11();

    std::array<op_type, 0xF + 1> OP_ARR_MAIN = [] consteval {
        auto OP_ARR_MAIN_ = decltype(OP_ARR_MAIN){};
        OP_ARR_MAIN_.fill(&chip8::op_null);
        OP_ARR_MAIN_[0x0] = &chip8::op_arr_0;
        OP_ARR_MAIN_[0x1] = &chip8::op_1nnn;
        OP_ARR_MAIN_[0x2] = &chip8::op_2nnn;
        OP_ARR_MAIN_[0x3] = &chip8::op_3xnn;
        OP_ARR_MAIN_[0x4] = &chip8::op_4xnn;
        OP_ARR_MAIN_[0x5] = &chip8::op_5xy0;
        OP_ARR_MAIN_[0x6] = &chip8::op_6xnn;
        OP_ARR_MAIN_[0x7] = &chip8::op_7xnn;
        OP_ARR_MAIN_[0x8] = &chip8::op_arr_8;
        OP_ARR_MAIN_[0x9] = &chip8::op_9xy0;
        OP_ARR_MAIN_[0xA] = &chip8::op_Annn;
        OP_ARR_MAIN_[0xB] = &chip8::op_Bnnn;
        OP_ARR_MAIN_[0xC] = &chip8::op_Cxnn;
        OP_ARR_MAIN_[0xD] = &chip8::op_Dxyn;
        OP_ARR_MAIN_[0xE] = &chip8::op_arr_E;
        OP_ARR_MAIN_[0xF] = &chip8::op_arr_F;
        return OP_ARR_MAIN_;
    }();

    std::array<op_type, 0xEE + 1> OP_ARR_0 = [] consteval {
        auto OP_ARR_0_ = decltype(OP_ARR_0){};
        OP_ARR_0_.fill(&chip8::op_null);
        OP_ARR_0_[0xE0] = &chip8::op_00E0;
        OP_ARR_0_[0xEE] = &chip8::op_00EE;
        return OP_ARR_0_;
    }();

    std::array<op_type, 0xE + 1> OP_ARR_8 = [] consteval {
        auto OP_ARR_8_ = decltype(OP_ARR_8){};
        OP_ARR_8_.fill(&chip8::op_null);
        OP_ARR_8_[0x0] = &chip8::op_8xy0;
        OP_ARR_8_[0x1] = &chip8::op_8xy1;
        OP_ARR_8_[0x2] = &chip8::op_8xy2;
        OP_ARR_8_[0x3] = &chip8::op_8xy3;
        OP_ARR_8_[0x4] = &chip8::op_8xy4;
        OP_ARR_8_[0x5] = &chip8::op_8xy5;
        OP_ARR_8_[0x6] = &chip8::op_8xy6;
        OP_ARR_8_[0x7] = &chip8::op_8xy7;
        OP_ARR_8_[0xE] = &chip8::op_8xyE;
        return OP_ARR_8_;
    }();

    std::array<op_type, 0xE + 1> OP_ARR_E = [] consteval {
        auto OP_ARR_E_ = decltype(OP_ARR_E){};
        OP_ARR_E_.fill(&chip8::op_null);
        OP_ARR_E_[0x1] = &chip8::op_ExA1;
        OP_ARR_E_[0xE] = &chip8::op_Ex9E;
        return OP_ARR_E_;
    }();

    std::array<op_type, 0x65 + 1> OP_ARR_F = [] consteval {
        auto OP_ARR_F_ = decltype(OP_ARR_F){};
        OP_ARR_F_.fill(&chip8::op_null);
        OP_ARR_F_[0x07] = &chip8::op_Fx07;
        OP_ARR_F_[0x0A] = &chip8::op_Fx0A;
        OP_ARR_F_[0x15] = &chip8::op_Fx15;
        OP_ARR_F_[0x18] = &chip8::op_Fx18;
        OP_ARR_F_[0x1E] = &chip8::op_Fx1E;
        OP_ARR_F_[0x29] = &chip8::op_Fx29;
        OP_ARR_F_[0x33] = &chip8::op_Fx33;
        OP_ARR_F_[0x55] = &chip8::op_Fx55;
        OP_ARR_F_[0x65] = &chip8::op_Fx65;
        return OP_ARR_F_;
    }();
};
