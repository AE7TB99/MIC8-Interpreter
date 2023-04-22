#include "chip8.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <limits>
#include <string_view>

chip8::chip8(const bool chip48_jmp, const bool chip48_shf, const ls_mode mode) {
    if(chip48_jmp) OP_ARR_MAIN[0xB] = &chip8::op_Bxnn_CHIP48;
    if(chip48_shf) {
        OP_ARR_8[0x6] = &chip8::op_8xy6_CHIP48;
        OP_ARR_8[0xE] = &chip8::op_8xyE_CHIP48;
    }
    switch(mode) {
        case ls_mode::chip8_ls: break;
        case ls_mode::chip48_ls: 
            OP_ARR_F[0x55] = &chip8::op_Fx55_CHIP48;
            OP_ARR_F[0x65] = &chip8::op_Fx65_CHIP48;
            break;
        case ls_mode::schip_ls:
            OP_ARR_F[0x55] = &chip8::op_Fx55_SCHIP;
            OP_ARR_F[0x65] = &chip8::op_Fx65_SCHIP;
            break;
    }
}

void chip8::run_cycle() {
    opcode = (mem[pc] << 8u) | mem[pc + 1];
    pc += INSTRUCTION_SIZE;
    (this->*OP_ARR_MAIN[(opcode & 0xF000u) >> 12u])();
    if (dt > 0) --dt;
    if (st > 0) --st;
}

void chip8::reset() {
    std::fill(fb.begin(), fb.end(), 0);
    pc = ROM_ADDR;
}

void chip8::load_rom(std::string_view path) {
    std::ifstream file(path.data(), std::ios::binary | std::ios::ate);
    if (!file) throw std::invalid_argument("Failed to open the file!");
    auto file_size = file.tellg();
    if (file_size == std::ifstream::pos_type(-1)) throw std::invalid_argument("Tell failed!");
    file.seekg(0, std::ios::beg);
    if (!file) throw std::invalid_argument("Seek failed!");
    if (static_cast<unsigned>(file_size) > mem.size() - ROM_ADDR) throw std::invalid_argument("File will not fit in memory!");
    std::vector<std::uint8_t> buffer(file_size);
    file.read(reinterpret_cast<char*>(buffer.data()), file_size);
    if (!file) throw std::invalid_argument("Read Failed!");
    std::copy(buffer.begin(), buffer.end(), mem.begin() + ROM_ADDR);
}

void chip8::unload_rom() {
    reset();
    std::fill(mem.begin() + ROM_ADDR, mem.end(), 0);
}

void chip8::op_arr_0() {
    (this->*OP_ARR_0[opcode & 0x000Fu])();
}

void chip8::op_arr_8() {
    (this->*OP_ARR_8[opcode & 0x000Fu])();
}

void chip8::op_arr_E() {
    (this->*OP_ARR_E[opcode & 0x000Fu])();
}

void chip8::op_arr_F() {
    (this->*OP_ARR_F[opcode & 0x00FFu])();
}

void chip8::op_null() {
    //something should happen... probably...
}

void chip8::op_00E0() {
    fb.fill(false);
    draw_flag = true;
}

void chip8::op_00EE() {
    pc = stack[--sp];
}

void chip8::op_1nnn() {
    const std::uint16_t nnn = opcode & 0x0FFFu;
    pc = nnn;
}

void chip8::op_2nnn() {
    const std::uint16_t nnn = opcode & 0x0FFFu;
    stack[sp++] = pc;
    pc = nnn;
}

void chip8::op_3xkk() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    const std::uint8_t kk = opcode & 0x0FFFu;
    if (reg[x] == kk) pc += INSTRUCTION_SIZE;
}

void chip8::op_4xkk() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    const std::uint8_t kk = opcode & 0x0FFFu;
    if (reg[x] != kk) pc += INSTRUCTION_SIZE;
}

void chip8::op_5xy0() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    const std::uint8_t y = (opcode & 0x00F0u) >> 4u;
    if (reg[x] == reg[y]) pc += INSTRUCTION_SIZE;
}

void chip8::op_6xkk() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    const std::uint8_t kk = opcode & 0x00FFu;
    reg[x] = kk;
}

void chip8::op_7xkk() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    const std::uint8_t kk = opcode & 0x00FFu;
    reg[x] += kk;
}

void chip8::op_8xy0() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    const std::uint8_t y = (opcode & 0x00F0u) >> 4u;
    reg[x] = reg[y];
}

void chip8::op_8xy1() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    const std::uint8_t y = (opcode & 0x00F0u) >> 4u;
    reg[x] |= reg[y];
}

void chip8::op_8xy2() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    const std::uint8_t y = (opcode & 0x00F0u) >> 4u;
    reg[x] &= reg[y];
}

void chip8::op_8xy3() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    const std::uint8_t y = (opcode & 0x00F0u) >> 4u;
    reg[x] ^= reg[y];
}


void chip8::op_8xy4() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    const std::uint8_t y = (opcode & 0x00F0u) >> 4u;
    reg[0xF] = (reg[x] + reg[y]) > std::numeric_limits<std::uint8_t>::max();
    reg[x] += reg[y];
}


void chip8::op_8xy5() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    const std::uint8_t y = (opcode & 0x00F0u) >> 4u;
    reg[0xF] = reg[x] > reg[y];
    reg[x] -= reg[y];
}

void chip8::op_8xy6() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    const std::uint8_t y = (opcode & 0x00F0u) >> 4u;
    reg[0xF] = reg[y] & 0x1u;
    reg[x] = reg[y] >> 1u;
}

void chip8::op_8xy7() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    const std::uint8_t y = (opcode & 0x00F0u) >> 4u;
    reg[0xF] = reg[y] > reg[x];
    reg[x] = reg[y] - reg[x];
}

void chip8::op_8xyE() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    const std::uint8_t y = (opcode & 0x00F0u) >> 4u;
    reg[0xF] = reg[x] >> 7u;
    reg[x] = reg[y] << 1u;
}

void chip8::op_9xy0() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    const std::uint8_t y = (opcode & 0x00F0u) >> 4u;
    if (reg[x] != reg[y]) pc += INSTRUCTION_SIZE;
}

void chip8::op_Annn() {
    const std::uint16_t nnn = opcode & 0x0FFFu;
    ir = nnn;
}

void chip8::op_Bnnn() {
    const std::uint16_t nnn = opcode & 0x0FFFu;
    pc = reg[0x0] + nnn;
}

void chip8::op_Cxkk() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    const std::uint8_t kk = opcode & 0x00FFu;
    reg[x] = std::uniform_int_distribution<>(0, std::numeric_limits<std::uint8_t>::max())(rng) & kk;
}

void chip8::op_Dxyn() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    const std::uint8_t y = (opcode & 0x00F0u) >> 4u;
    const std::uint8_t n = opcode & 0x000Fu;
    const std::uint8_t x_pos = reg[x] % VIDEO_WIDTH;
    const std::uint8_t y_pos = reg[y] % VIDEO_HEIGHT;
    for (std::uint8_t y_line = 0; y_line < n; ++y_line) {
        if (y_pos + y_line >= VIDEO_HEIGHT) break;
        std::uint8_t pixels = mem[ir + y_line];
        for (std::uint8_t x_line = 0; x_line < 8; ++x_line) {
            if (x_pos + x_line >= VIDEO_WIDTH) continue;
            if (pixels & (0b1000'0000u >> x_line)) {
                const std::uint16_t pos = x_pos + x_line + (y_pos + y_line) * VIDEO_WIDTH;
                reg[0xF] = fb[pos] == 0xFFFF'FFFF;
                fb[pos] ^= 0xFFFF'FFFF;
            }
        }
    }
    draw_flag = true;
}

void chip8::op_Ex9E() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    if (keys[reg[x]]) pc += INSTRUCTION_SIZE;
}

void chip8::op_ExA1() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    if (!keys[reg[x]]) pc += INSTRUCTION_SIZE;
}

void chip8::op_Fx07() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    reg[x] = dt;
}

void chip8::op_Fx0A() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    bool set = false;
    for (std::uint8_t i = 0; !set && i < KEY_COUNT; ++i) {
        if (keys[i]) {
            reg[x] = i;
            set = true;
        }
    }
}

void chip8::op_Fx15() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    dt = reg[x];
}

void chip8::op_Fx18() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    st = reg[x];
}

void chip8::op_Fx1E() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    reg[0xF] = (ir + reg[x]) > std::numeric_limits<std::uint8_t>::max();
    ir += reg[x];
}

void chip8::op_Fx29() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    ir = FONTSET_ADDR + (5 * reg[x]);
}

void chip8::op_Fx33() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    mem[ir] = reg[x] / 100;
    mem[ir + 1] = (reg[x] / 10) % 10;
    mem[ir + 2] = reg[x] % 10;
}

void chip8::op_Fx55() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    for (std::uint8_t i = 0; i <= x; ++i) {
        mem[ir + i] = reg[i];
    }
    ir += x + 1;
}

void chip8::op_Fx65() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    for (std::uint8_t i = 0; i <= x; ++i) {
        reg[i] = mem[ir + i];
    }
    ir += x + 1;
}

void chip8::op_Bxnn_CHIP48() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    const std::uint8_t nn = (opcode & 0x00FFu);
    pc = reg[x] + (x << 8u | nn); 
}

void chip8::op_Fx55_CHIP48() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    for (std::uint8_t i = 0; i <= x; ++i) {
        mem[ir + i] = reg[i];
    }
    ir += x;
}

void chip8::op_Fx65_CHIP48() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    for (std::uint8_t i = 0; i <= x; ++i) {
        reg[i] = mem[ir + i];
    }
    ir += x;
}

void chip8::op_8xy6_CHIP48() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    reg[0xF] = reg[x] & 0x1u;
    reg[x] >>= 1u;
}

void chip8::op_8xyE_CHIP48() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    reg[0xF] = reg[x] >> 7u;
    reg[x] <<= 1u;
}

void chip8::op_Fx55_SCHIP() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    for (std::uint8_t i = 0; i <= x; ++i) {
        mem[ir + i] = reg[i];
    }
}

void chip8::op_Fx65_SCHIP() {
    const std::uint8_t x = (opcode & 0x0F00u) >> 8u;
    for (std::uint8_t i = 0; i <= x; ++i) {
        reg[i] = mem[ir + i];
    }
}
