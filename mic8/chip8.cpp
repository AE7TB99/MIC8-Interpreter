#include "chip8.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <fstream>
#include <ios>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <vector>

chip8::chip8(const alt_t alt_ops) {
    if (alt_ops.vip_alu) {
        OP_ARR_8[0x1] = &chip8::op_8xy1_VIP;
        OP_ARR_8[0x2] = &chip8::op_8xy2_VIP;
        OP_ARR_8[0x3] = &chip8::op_8xy3_VIP;
    }
    if (alt_ops.chip48_jmp) {
        OP_ARR_MAIN[0xB] = &chip8::op_Bxnn_CHIP48;
    }
    if (alt_ops.chip48_shf) {
        OP_ARR_8[0x6] = &chip8::op_8xy6_CHIP48;
        OP_ARR_8[0xE] = &chip8::op_8xyE_CHIP48;
    }
    switch (alt_ops.ls_mode) {
        case ls_mode::chip8_ls:
            break;
        case ls_mode::chip48_ls:
            OP_ARR_F[0x55] = &chip8::op_Fx55_CHIP48;
            OP_ARR_F[0x65] = &chip8::op_Fx65_CHIP48;
            break;
        case ls_mode::schip11_ls:
            OP_ARR_F[0x55] = &chip8::op_Fx55_SCHIP11;
            OP_ARR_F[0x65] = &chip8::op_Fx65_SCHIP11;
    }
}

void chip8::run_cycle() {
    instruction = mem[pc] << 8u | mem[pc + 1];
    pc += INSTRUCTION_SIZE;
    (this->*OP_ARR_MAIN[(instruction & 0xF000u) >> 12u])();
}

void chip8::decrement_timers() {
    if (dt > 0) { --dt; }
    if (st > 0) { --st; }
}

void chip8::reset() {
    fb.fill(0);
    stack.fill(0);
    reg.fill(0);
    keys.fill(false);
    drw_flag = true;
    hlt_flag = false;
    pc = ROM_ADDR;
    ir = 0;
    sp = 0;
    dt = 0;
    st = 0;
}

void chip8::load_rom(const std::string_view path) {
    std::ifstream file(path.data(), std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::invalid_argument("Failed to open the file!");
    }
    const auto file_size = file.tellg();
    if (file_size == std::ifstream::pos_type(-1)) {
        throw std::invalid_argument("Tell failed!");
    }
    file.seekg(0, std::ios::beg);
    if (!file) {
        throw std::invalid_argument("Seek failed!");
    }
    if (static_cast<unsigned>(file_size) > mem.size() - ROM_ADDR) {
        throw std::invalid_argument("File will not fit in memory!");
    }
    std::vector<std::uint8_t> buffer(file_size);
    file.read(reinterpret_cast<char *>(buffer.data()), file_size); // NOLINT(*-pro-type-reinterpret-cast)
    if (!file) {
        throw std::invalid_argument("Read Failed!");
    }
    std::ranges::copy(buffer, mem.begin() + ROM_ADDR);
}

void chip8::unload_rom() {
    reset();
    std::fill(mem.begin() + ROM_ADDR, mem.end(), 0);
}

void chip8::op_arr_0() { (this->*OP_ARR_0[instruction & 0x00FFu])(); }

void chip8::op_arr_8() { (this->*OP_ARR_8[instruction & 0x000Fu])(); }

void chip8::op_arr_E() { (this->*OP_ARR_E[instruction & 0x000Fu])(); }

void chip8::op_arr_F() { (this->*OP_ARR_F[instruction & 0x00FFu])(); }

void chip8::op_null() {
    instruction_string = std::format("0x{:X} - {:X} -> null", pc, instruction);
    hlt_flag = true;
}

void chip8::op_00E0() {
    instruction_string = std::format("0x{:X} - {:04X} -> clear", pc, instruction);
    fb.fill(0u);
    drw_flag = true;
}

void chip8::op_00EE() {
    instruction_string = std::format("0x{:X} - {:04X} -> return", pc, instruction);
    pc = stack[--sp];
}

void chip8::op_1nnn() {
    const std::uint16_t nnn = instruction & 0x0FFFu;
    instruction_string = std::format("0x{:X} - {:X} -> jump 0x{:03X}", pc, instruction, nnn);
    if (pc == nnn) { hlt_flag = true; }
    pc = nnn;
}

void chip8::op_2nnn() {
    const std::uint16_t nnn = instruction & 0x0FFFu;
    instruction_string = std::format("0x{:X} - {:X} -> :call 0x{:03X}", pc, instruction, nnn);
    stack[sp++] = pc;
    pc = nnn;
}

void chip8::op_3xnn() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    const std::uint8_t nn = instruction & 0x0FFFu;
    instruction_string = std::format("0x{:X} - {:X} -> if v{:X} != {} then", pc, instruction, x, nn);
    if (reg[x] == nn) { pc += INSTRUCTION_SIZE; }
}

void chip8::op_4xnn() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    const std::uint8_t nn = instruction & 0x0FFFu;
    instruction_string = std::format("0x{:X} - {:X} -> if v{:X} == {} then", pc, instruction, x, nn);
    if (reg[x] != nn) { pc += INSTRUCTION_SIZE; }
}

void chip8::op_5xy0() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    const std::uint8_t y = (instruction & 0x00F0u) >> 4u;
    instruction_string = std::format("0x{:X} - {:X} -> if v{:X} != v{:X} then", pc, instruction, x, y);
    if (reg[x] == reg[y]) { pc += INSTRUCTION_SIZE; }
}

void chip8::op_6xnn() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    const std::uint8_t nn = instruction & 0x00FFu;
    instruction_string = std::format("0x{:X} - {:X} -> v{:X} := {}", pc, instruction, x, nn);
    reg[x] = nn;
}

void chip8::op_7xnn() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    const std::uint8_t nn = instruction & 0x00FFu;
    instruction_string = std::format("0x{:X} - {:X} -> v{:X} += {}", pc, instruction, x, nn);
    reg[x] += nn;
}

void chip8::op_8xy0() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    const std::uint8_t y = (instruction & 0x00F0u) >> 4u;
    instruction_string = std::format("0x{:X} - {:X} -> v{:X} := v{:X}", pc, instruction, x, y);
    reg[x] = reg[y];
}

void chip8::op_8xy1() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    const std::uint8_t y = (instruction & 0x00F0u) >> 4u;
    instruction_string = std::format("0x{:X} - {:X} -> v{:X} |= v{:X}", pc, instruction, x, y);
    reg[x] |= reg[y];
}

void chip8::op_8xy2() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    const std::uint8_t y = (instruction & 0x00F0u) >> 4u;
    instruction_string = std::format("0x{:X} - {:X} -> v{:X} &= v{:X}", pc, instruction, x, y);
    reg[x] &= reg[y];
}

void chip8::op_8xy3() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    const std::uint8_t y = (instruction & 0x00F0u) >> 4u;
    instruction_string = std::format("0x{:X} - {:X} -> v{:X} ^= v{:X}", pc, instruction, x, y);
    reg[x] ^= reg[y];
}

void chip8::op_8xy4() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    const std::uint8_t y = (instruction & 0x00F0u) >> 4u;
    instruction_string = std::format("0x{:X} - {:X} -> v{:X} += v{:X}", pc, instruction, x, y);
    const std::uint16_t res = reg[x] + reg[y];
    reg[x] = res;
    reg[0xF] = res >> 8u;
}

void chip8::op_8xy5() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    const std::uint8_t y = (instruction & 0x00F0u) >> 4u;
    instruction_string = std::format("0x{:X} - {:X} -> v{:X} -= v{:X}", pc, instruction, x, y);
    const std::uint16_t res = reg[x] - reg[y];
    reg[x] = res;
    reg[0xF] = static_cast<std::uint8_t>(res <= std::numeric_limits<std::uint8_t>::max());
}

void chip8::op_8xy6() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    const std::uint8_t y = (instruction & 0x00F0u) >> 4u;
    instruction_string = std::format("0x{:X} - {:X} -> v{:X} >>= v{:X}", pc, instruction, x, y);
    const std::uint8_t car = reg[y] & 1u;
    reg[x] = reg[y] >> 1u;
    reg[0xF] = car;
}

void chip8::op_8xy7() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    const std::uint8_t y = (instruction & 0x00F0u) >> 4u;
    instruction_string = std::format("0x{:X} - {:X} -> v{:X} =- v{:X}", pc, instruction, x, y);
    const std::uint16_t res = reg[y] - reg[x];
    reg[x] = res;
    reg[0xF] = static_cast<std::uint8_t>(res <= std::numeric_limits<std::uint8_t>::max());
}

void chip8::op_8xyE() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    const std::uint8_t y = (instruction & 0x00F0u) >> 4u;
    instruction_string = std::format("0x{:X} - {:X} -> v{:X} <<= v{:X}", pc, instruction, x, y);
    const uint8_t car = reg[y] >> 7u;
    reg[x] = reg[y] << 1u;
    reg[0xF] = car;
}

void chip8::op_9xy0() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    const std::uint8_t y = (instruction & 0x00F0u) >> 4u;
    instruction_string = std::format("0x{:X} - {:X} -> if v{:X} == v{:X} then", pc, instruction, x, y);
    if (reg[x] != reg[y]) { pc += INSTRUCTION_SIZE; }
}

void chip8::op_Annn() {
    const std::uint16_t nnn = instruction & 0x0FFFu;
    instruction_string = std::format("0x{:X} - {:X} -> i := 0x{:3X}", pc, instruction, nnn);
    ir = nnn;
}

void chip8::op_Bnnn() {
    const std::uint16_t nnn = instruction & 0x0FFFu;
    instruction_string = std::format("0x{:X} - {:X} -> jump0 0x{:3X}", pc, instruction, nnn);
    if (pc == reg[0x0] + nnn) { hlt_flag = true; }
    pc = reg[0x0] + nnn;
}

void chip8::op_Cxnn() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    const std::uint8_t nn = instruction & 0x00FFu;
    instruction_string = std::format("0x{:X} - {:X} -> v{:X} := random {}", pc, instruction, x, nn);
    reg[x] = std::uniform_int_distribution<>(0, std::numeric_limits<std::uint8_t>::max())(rng) & nn;
}

void chip8::op_Dxyn() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    const std::uint8_t y = (instruction & 0x00F0u) >> 4u;
    const std::uint8_t n = instruction & 0x000Fu;
    instruction_string = std::format("0x{:X} - {:X} -> sprite v{:X} v{:X} {}", pc, instruction, x, y, n);
    const std::uint8_t x_pos = reg[x] & VIDEO_WIDTH - 1;
    const std::uint8_t y_pos = reg[y] & VIDEO_HEIGHT - 1;
    for (unsigned char row = 0; row < n; ++row) {
        if (row + y_pos >= VIDEO_HEIGHT) { break; }
        const std::uint8_t pixels = mem[ir + row];
        for (unsigned char col = 0; col < 8; ++col) {
            if (col + x_pos >= VIDEO_WIDTH) { break; }
            if ((pixels & 0b1000'0000 >> col) == 0) { continue; }
            const std::uint16_t px_pox = x_pos + col + (y_pos + row) * VIDEO_WIDTH;
            reg[0xF] = static_cast<std::uint8_t>(fb[px_pox] == 0xFFFF'FFFF);
            fb[px_pox] ^= 0xFFFF'FFFF;
        }
    }
    drw_flag = true;
}

void chip8::op_Ex9E() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    instruction_string = std::format("0x{:X} - {:X} -> if v{:X} -key then", pc, instruction, x);
    if (keys[reg[x]]) { pc += INSTRUCTION_SIZE; }
}

void chip8::op_ExA1() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    instruction_string = std::format("0x{:X} - {:X} -> if v{:X} key then", pc, instruction, x);
    if (!keys[reg[x]]) { pc += INSTRUCTION_SIZE; }
}

void chip8::op_Fx07() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    instruction_string = std::format("0x{:X} - {:X} -> v{:X} := delay", pc, instruction, x);
    reg[x] = dt;
}

void chip8::op_Fx0A() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    instruction_string = std::format("0x{:X} - {:X} -> v{:X} := key", pc, instruction, x);
    static bool set = false;
    static unsigned char i;
    for (i = 0; !set && i < KEY_COUNT; ++i) {
        set = keys[i];
    }
    if (keys[i] || !set) { pc -= INSTRUCTION_SIZE; } else {
        reg[x] = i;
        set = false;
    }
}

void chip8::op_Fx15() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    instruction_string = std::format("0x{:X} - {:X} -> delay := v{:X}", pc, instruction, x);
    dt = reg[x];
}

void chip8::op_Fx18() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    instruction_string = std::format("0x{:X} - {:X} -> buzzer := v{:X}", pc, instruction, x);
    st = reg[x];
}

void chip8::op_Fx1E() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    instruction_string = std::format("0x{:X} - {:X} -> i += v{:X}", pc, instruction, x);
    ir += reg[x];
    reg[0xF] = static_cast<std::uint8_t>(ir + reg[x] > std::numeric_limits<std::uint8_t>::max());
}

void chip8::op_Fx29() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    instruction_string = std::format("0x{:X} - {:X} -> i := hex v{:X}", pc, instruction, x);
    ir = FONTSET_ADDR + static_cast<std::size_t>(reg[x] * 5);
}

void chip8::op_Fx33() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    instruction_string = std::format("0x{:X} - {:X} -> bcd v{:X}", pc, instruction, x);
    mem[ir] = reg[x] / 100;
    mem[ir + 1] = reg[x] / 10 % 10;
    mem[ir + 2] = reg[x] % 10;
}

void chip8::op_Fx55() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    instruction_string = std::format("0x{:X} - {:X} -> save v{:X}", pc, instruction, x);
    for (unsigned char i = 0; i <= x; ++i) {
        mem[ir + i] = reg[i];
    }
    ir += x + 1;
}

void chip8::op_Fx65() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    instruction_string = std::format("0x{:X} - {:X} -> load v{:X}", pc, instruction, x);
    for (unsigned char i = 0; i <= x; ++i) {
        reg[i] = mem[ir + i];
    }
    ir += x + 1;
}

void chip8::op_8xy1_VIP() {
    op_8xy1();
    reg[0xF] = 0;
}

void chip8::op_8xy2_VIP() {
    op_8xy2();
    reg[0xF] = 0;
}

void chip8::op_8xy3_VIP() {
    op_8xy3();
    reg[0xF] = 0;
}

void chip8::op_8xy6_CHIP48() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    instruction_string = std::format("0x{:X} - {:X} -> v{:X} >>= 1", pc, instruction, x);
    const uint8_t car = reg[x] & 1u;
    reg[x] >>= 1u;
    reg[0xF] = car;
}

void chip8::op_8xyE_CHIP48() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    instruction_string = std::format("0x{:X} - {:X} -> v{:X} <<= 1", pc, instruction, x);
    const std::uint8_t car = reg[x] >> 7u;
    reg[x] <<= 1u;
    reg[0xF] = car;
}

void chip8::op_Bxnn_CHIP48() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    const std::uint8_t nn = instruction & 0x00FFu;
    instruction_string = std::format("0x{:X} - {:X} -> jump0 0x{:2X} + v{:X}", pc, instruction, nn, x);
    pc = reg[x] + (x << 8u | nn);
}

void chip8::op_Fx55_CHIP48() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    instruction_string = std::format("0x{:X} - {:X} -> save v{:X}", pc, instruction, x);
    for (unsigned char i = 0; i <= x; ++i) {
        mem[ir + i] = reg[i];
    }
    ir += x;
}

void chip8::op_Fx65_CHIP48() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    instruction_string = std::format("0x{:X} - {:X} -> load v{:X}", pc, instruction, x);
    for (unsigned char i = 0; i <= x; ++i) {
        reg[i] = mem[ir + i];
    }
    ir += x;
}

void chip8::op_Fx55_SCHIP11() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    instruction_string = std::format("0x{:X} - {:X} -> save v{:X}", pc, instruction, x);
    for (unsigned char i = 0; i <= x; ++i) {
        mem[ir + i] = reg[i];
    }
}

void chip8::op_Fx65_SCHIP11() {
    const std::uint8_t x = (instruction & 0x0F00u) >> 8u;
    instruction_string = std::format("0x{:X} - {:X} -> load v{:X}", pc, instruction, x);
    for (unsigned char i = 0; i <= x; ++i) {
        reg[i] = mem[ir + i];
    }
}
