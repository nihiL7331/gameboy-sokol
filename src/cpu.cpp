#include "cpu.hpp"
#include <format>
#include <iostream>

uint8_t CPU::Step() {
  uint8_t opcode = bus.Read(PC++);
  if (PC > 0x000C) {
    std::cout << std::format("PC: 0x{:04X} | OP: 0x{:02X}\n", PC - 1, opcode);
  }

  switch (opcode) {
  case 0x01: { // LD BC, n16
    uint8_t lo = bus.Read(PC++);
    uint8_t hi = bus.Read(PC++);
    SetBC((hi << 8) | lo);
    return 12;
  }
  case 0x04: { // INC B
    uint8_t res = B + 1;
    SetFlag(FLAG_Z, res == 0x00);
    SetFlag(FLAG_HCY, (B ^ 1 ^ res) & 0x10);
    SetFlag(FLAG_N, false);
    B = res;
    return 4;
  }
  case 0x05: { // DEC B
    uint8_t res = B - 1;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (B ^ 1 ^ res) & 0x10);
    B = res;
    return 4;
  }
  case 0x06: { // LD B, n8
    uint8_t byte = bus.Read(PC++);
    B = byte;
    return 8;
  }
  case 0x08: { // LD [a16], SP
    uint8_t lo = bus.Read(PC++);
    uint8_t hi = bus.Read(PC++);
    uint16_t addr = (hi << 8) | lo;
    bus.Write(addr, SP & 0xFF);
    bus.Write(addr + 1, SP >> 8);
    return 20;
  }
  case 0x0C: { // INC C
    uint8_t res = C + 1;
    SetFlag(FLAG_Z, res == 0x00);
    SetFlag(FLAG_HCY, (C ^ 1 ^ res) & 0x10);
    SetFlag(FLAG_N, false);
    C = res;
    return 4;
  }
  case 0x0D: { // DEC C
    uint8_t res = C - 1;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (C ^ 1 ^ res) & 0x10);
    C = res;
    return 4;
  }
  case 0x0E: { // LD C, n8
    C = bus.Read(PC++);
    return 8;
  }
  case 0x11: { // LD DE, n16
    uint8_t lo = bus.Read(PC++);
    uint8_t hi = bus.Read(PC++);
    SetDE((hi << 8) | lo);
    return 12;
  }
  case 0x13: { // INC DE
    SetDE(GetDE() + 1);
    return 8;
  }
  case 0x15: { // DEC D
    uint8_t res = D - 1;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (D ^ 1 ^ res) & 0x10);
    D = res;
    return 4;
  }
  case 0x16: { // LD D, n8
    D = bus.Read(PC++);
    return 8;
  }
  case 0x17: { // RLA
    uint8_t CY = GetFlag(FLAG_CY);
    uint8_t res = (A << 1) | CY;
    SetFlag(FLAG_Z, false);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, A & 0x80);
    A = res;
    return 4;
  }
  case 0x18: { // JR e8
    int8_t offset = static_cast<int8_t>(bus.Read(PC++));
    PC += offset;
    return 12;
  }
  case 0x1A: { // LD A, [DE]
    A = bus.Read(GetDE());
    return 8;
  }
  case 0x1D: { // DEC E
    uint8_t res = E - 1;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (E ^ 1 ^ res) & 0x10);
    E = res;
    return 4;
  }
  case 0x1E: { // LD E, n8
    E = bus.Read(PC++);
    return 8;
  }
  case 0x20: { // JR NZ, e8
    int8_t offset = static_cast<int8_t>(bus.Read(PC++));
    if (!GetFlag(FLAG_Z)) {
      PC += offset;
      return 12;
    }
    return 8;
  }
  case 0x21: { // LD HL, n16
    uint8_t lo = bus.Read(PC++);
    uint8_t hi = bus.Read(PC++);
    H = hi;
    L = lo;
    return 12;
  }
  case 0x22: { // LD [HL+], A
    bus.Write(GetHL(), A);
    SetHL(GetHL() + 1);
    return 8;
  }
  case 0x23: { // INC HL
    SetHL(GetHL() + 1);
    return 8;
  }
  case 0x24: { // INC H
    uint8_t res = H + 1;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (H ^ 1 ^ res) & 0x10);
    H = res;
    return 4;
  }
  case 0x26: { // LD H, n8
    H = bus.Read(PC++);
    return 8;
  }
  case 0x28: { // JR Z, e8
    int8_t offset = static_cast<int8_t>(bus.Read(PC++));
    if (GetFlag(FLAG_Z)) {
      PC += offset;
      return 12;
    }
    return 8;
  }
  case 0x2E: { // LD L, n8
    L = bus.Read(PC++);
    return 8;
  }
  case 0x31: { // LD SP, n16
    uint8_t lo = bus.Read(PC++);
    uint8_t hi = bus.Read(PC++);
    SP = (hi << 8) | lo;
    return 12;
  }
  case 0x32: { // LD [HL-], A
    bus.Write(GetHL(), A);
    SetHL(GetHL() - 1);
    return 8;
  }
  case 0x3D: { // DEC A
    uint8_t res = A - 1;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A ^ 1 ^ res) & 0x10);
    A = res;
    return 4;
  }
  case 0x3E: { // LD A, n8
    A = bus.Read(PC++);
    return 8;
  }
  case 0x4F: { // LD C, A
    C = A;
    return 4;
  }
  case 0x57: { // LD D, A
    D = A;
    return 4;
  }
  case 0x67: { // LD H, A
    H = A;
    return 4;
  }
  case 0x77: { // LD [HL], A
    bus.Write(GetHL(), A);
    return 8;
  }
  case 0x7B: { // LD A, E
    A = E;
    return 4;
  }
  case 0x7C: { // LD A, H
    A = H;
    return 4;
  }
  case 0x7E: { // LD A, [HL]
    A = bus.Read(GetHL());
    return 8;
  }
  case 0x80: { // ADD A, B
    uint16_t res = A + B;
    SetFlag(FLAG_Z, (res & 0x00FF) == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (A ^ B ^ res) & 0x0010);
    SetFlag(FLAG_CY, res & 0x0100);
    A = res;
    return 4;
  }
  case 0x90: { // SUB A, B
    uint8_t res = A - B;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A ^ B ^ res) & 0x10);
    SetFlag(FLAG_CY, B > A);
    A = res;
    return 4;
  }
  case 0xAF: { // XOR A, A
    A = 0x00;
    SetFlag(FLAG_Z, true);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
    return 4;
  }
  case 0xBE: { // CP A, [HL]
    uint8_t byte = bus.Read(GetHL());
    uint8_t res = A - byte;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A ^ byte ^ res) & 0x10);
    SetFlag(FLAG_CY, byte > A);
    return 8;
  }
  case 0xC1: { // POP BC
    C = bus.Read(SP++);
    B = bus.Read(SP++);
    return 12;
  }
  case 0xC5: { // PUSH BC
    bus.Write(--SP, B);
    bus.Write(--SP, C);
    return 16;
  }
  case 0xC9: { // RET
    uint8_t lo = bus.Read(SP++);
    uint8_t hi = bus.Read(SP++);
    PC = (hi << 8) | lo;
    return 16;
  }
  case 0xCB: { // PREFIX
    return ExecuteCB();
  }
  case 0xCD: { // CALL a16
    uint8_t lo = bus.Read(PC++);
    uint8_t hi = bus.Read(PC++);
    bus.Write(--SP, PC >> 8);
    bus.Write(--SP, PC & 0x00FF);
    PC = (hi << 8) | lo;
    return 24;
  }
  case 0xE0: { // LDH [a8], A
    uint8_t lo = bus.Read(PC++);
    bus.Write(0xFF00 | lo, A);
    return 12;
  }
  case 0xE2: { // LDH [C], A
    bus.Write(0xFF00 | C, A);
    return 8;
  }
  case 0xEA: { // LD [a16], A
    uint8_t lo = bus.Read(PC++);
    uint8_t hi = bus.Read(PC++);
    bus.Write((hi << 8) | lo, A);
    return 16;
  }
  case 0xF0: { // LDH A, [a8]
    uint8_t lo = bus.Read(PC++);
    A = bus.Read(0xFF00 | lo);
    return 12;
  }
  case 0xFE: { // CP A, n8
    uint8_t byte = bus.Read(PC++);
    uint8_t res = A - byte;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A ^ byte ^ res) & 0x10);
    SetFlag(FLAG_CY, byte > A);
    return 8;
  }
  default:
    std::cout << std::format("Unimplemented opcode {:02x} at PC={:04x}\n",
                             static_cast<unsigned int>(opcode), PC - 1);
    exit(1);
  }
}

uint8_t CPU::ExecuteCB() {
  uint8_t cb_opcode = bus.Read(PC++);

  switch (cb_opcode) {
  case 0x11: { // RL C
    uint8_t CY = GetFlag(FLAG_CY);
    uint8_t res = (C << 1) | CY;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, C & 0x80);
    C = res;
    return 8;
  }
  case 0x7C: { // BIT 7, H
    SetFlag(FLAG_Z, (H & 0x80) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  default:
    std::cout << std::format("Unimplemented CB 0x{:02X}", cb_opcode)
              << std::endl;
    exit(1);
  }
}
