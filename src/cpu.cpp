#include "cpu.hpp"
#include <format>
#include <iostream>

uint8_t CPU::Step() {
  uint8_t opcode = bus.Read(PC++);
  if (!bus.is_boot) {
    // std::cout << std::format("PC: 0x{:04X} | OP: 0x{:02X}\n", PC - 1,
    // opcode);
  }

  switch (opcode) {
  case 0x00: { // NOP
    return 4;
  }
  case 0x01: { // LD BC, n16
    uint8_t lo = bus.Read(PC++);
    uint8_t hi = bus.Read(PC++);
    SetBC((hi << 8) | lo);
    return 12;
  }
  case 0x03: { // INC BC
    SetBC(GetBC() + 1);
    return 8;
  }
  case 0x04: { // INC B
    uint8_t res = B + 1;
    SetFlag(FLAG_Z, res == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (B ^ 1 ^ res) & 0x10);
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
  case 0x12: { // LD [DE], A
    bus.Write(GetDE(), A);
    return 8;
  }
  case 0x13: { // INC DE
    SetDE(GetDE() + 1);
    return 8;
  }
  case 0x14: { // INC D
    uint8_t res = D + 1;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (D ^ 1 ^ res) & 0x10);
    D = res;
    return 4;
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
  case 0x1C: { // INC E
    uint8_t res = E + 1;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (E ^ 1 ^ res) & 0x10);
    E = res;
    return 4;
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
  case 0x1F: { // RRA
    uint8_t CY = A & 0x01;
    A = (A >> 1) | (GetFlag(FLAG_CY) << 7);
    SetFlag(FLAG_Z, false);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, CY);
    return 4;
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
  case 0x25: { // DEC H
    uint8_t res = H - 1;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (H ^ 1 ^ res) & 0x10);
    H = res;
    return 4;
  }
  case 0x26: { // LD H, n8
    H = bus.Read(PC++);
    return 8;
  }
  case 0x27: { // DAA
    uint8_t adj = 0;
    if (GetFlag(FLAG_N)) {
      adj += GetFlag(FLAG_HCY) * 0x06;
      adj += GetFlag(FLAG_CY) * 0x60;
      A -= adj;
    } else {
      adj += (GetFlag(FLAG_HCY) | ((A & 0x0F) > 0x09)) * 0x06;
      uint8_t chk = GetFlag(FLAG_CY) | (A > 0x99);
      adj += chk * 0x60;
      SetFlag(FLAG_CY, chk);
      A += adj;
    }
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_Z, A == 0);
    return 4;
  }
  case 0x28: { // JR Z, e8
    int8_t offset = static_cast<int8_t>(bus.Read(PC++));
    if (GetFlag(FLAG_Z)) {
      PC += offset;
      return 12;
    }
    return 8;
  }
  case 0x29: { // ADD HL, HL
    uint16_t HL = GetHL();
    uint32_t res = HL << 1;
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, ((HL & 0x0FFF) + (HL & 0x0FFF)) > 0x0FFF);
    SetFlag(FLAG_CY, res & 0x00010000);
    SetHL(res);
    return 8;
  }
  case 0x2A: { // LD A, [HL+]
    A = bus.Read(GetHL());
    SetHL(GetHL() + 1);
    return 8;
  }
  case 0x2C: { // INC L
    uint8_t res = L + 1;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (L ^ 1 ^ res) & 0x10);
    L = res;
    return 4;
  }
  case 0x2D: { // DEC L
    uint8_t res = L - 1;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (L ^ 1 ^ res) & 0x10);
    L = res;
    return 4;
  }
  case 0x2E: { // LD L, n8
    L = bus.Read(PC++);
    return 8;
  }
  case 0x30: { // JR NC, e8
    int8_t offset = static_cast<int8_t>(bus.Read(PC++));
    if (!GetFlag(FLAG_CY)) {
      PC += offset;
      return 12;
    }
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
  case 0x33: { // INC SP
    SP++;
    return 8;
  }
  case 0x35: { // DEC [HL]
    uint16_t HL = GetHL();
    uint8_t val = bus.Read(HL);
    uint8_t res = val - 1;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (val ^ 1 ^ res) & 0x10);
    bus.Write(HL, res);
    return 12;
  }
  case 0x38: { // JR C, e8
    int8_t offset = static_cast<int8_t>(bus.Read(PC++));
    if (GetFlag(FLAG_CY)) {
      PC += offset;
      return 12;
    }
    return 8;
  }
  case 0x39: { // ADD HL, SP
    uint16_t HL = GetHL();
    uint32_t res = HL + SP;
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, ((HL & 0x0FFF) + (SP & 0x0FFF)) > 0x0FFF);
    SetFlag(FLAG_CY, res & 0x00010000);
    SetHL(res);
    return 8;
  }
  case 0x3B: { // DEC SP
    SP--;
    return 8;
  }
  case 0x3C: { // INC A
    uint8_t res = A + 1;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (A ^ 1 ^ res) & 0x10);
    A = res;
    return 4;
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
  case 0x46: { // LD B, [HL]
    B = bus.Read(GetHL());
    return 8;
  }
  case 0x47: { // LD B, A
    B = A;
    return 4;
  }
  case 0x4E: { // LD C, [HL]
    C = bus.Read(GetHL());
    return 8;
  }
  case 0x4F: { // LD C, A
    C = A;
    return 4;
  }
  case 0x56: { // LD D, [HL]
    D = bus.Read(GetHL());
    return 8;
  }
  case 0x57: { // LD D, A
    D = A;
    return 4;
  }
  case 0x5D: { // LD E, L
    E = L;
    return 4;
  }
  case 0x5E: { // LD E, [HL]
    E = bus.Read(GetHL());
    return 8;
  }
  case 0x5F: { // LD E, A
    E = A;
    return 4;
  }
  case 0x62: { // LD H, D
    H = D;
    return 4;
  }
  case 0x66: { // LD H, [HL]
    H = bus.Read(GetHL());
    return 8;
  }
  case 0x67: { // LD H, A
    H = A;
    return 4;
  }
  case 0x6B: { // LD L, E
    L = E;
    return 4;
  }
  case 0x6E: { // LD L, [HL]
    L = bus.Read(GetHL());
    return 8;
  }
  case 0x6F: { // LD L, A
    L = A;
    return 4;
  }
  case 0x70: { // LD[HL], B
    bus.Write(GetHL(), B);
    return 8;
  }
  case 0x71: { // LD [HL], C
    bus.Write(GetHL(), C);
    return 8;
  }
  case 0x72: { // LD [HL], D
    bus.Write(GetHL(), D);
    return 8;
  }
  case 0x73: { // LD [HL], E
    bus.Write(GetHL(), E);
    return 8;
  }
  case 0x77: { // LD [HL], A
    bus.Write(GetHL(), A);
    return 8;
  }
  case 0x78: { // LD A, B
    A = B;
    return 4;
  }
  case 0x79: { // LD A, C
    A = C;
    return 4;
  }
  case 0x7A: { // LD A, D
    A = D;
    return 4;
  }
  case 0x7B: { // LD A, E
    A = E;
    return 4;
  }
  case 0x7C: { // LD A, H
    A = H;
    return 4;
  }
  case 0x7D: { // LD A, L
    A = L;
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
  case 0x86: { // ADD A, [HL]
    uint8_t byte = bus.Read(GetHL());
    uint16_t res = A + byte;
    SetFlag(FLAG_Z, (res & 0x00FF) == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (A ^ byte ^ res) & 0x0010);
    SetFlag(FLAG_CY, res & 0x0100);
    A = res;
    return 8;
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
  case 0xA9: { // XOR A, C
    A ^= C;
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
    return 4;
  }
  case 0xAD: { // XOR A, L
    A ^= L;
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
    return 4;
  }
  case 0xAE: { // XOR A, [HL]
    A ^= bus.Read(GetHL());
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
    return 8;
  }
  case 0xAF: { // XOR A, A
    A = 0x00;
    SetFlag(FLAG_Z, true);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
    return 4;
  }
  case 0xB0: { // OR A, B
    A |= B;
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
    return 4;
  }
  case 0xB1: { // OR A, C
    A |= C;
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
    return 4;
  }
  case 0xB6: { // OR A, [HL]
    A |= bus.Read(GetHL());
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
    return 8;
  }
  case 0xB7: { // OR A, A
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
    return 4;
  }
  case 0xB8: { // CP A, B
    uint8_t res = A - B;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A ^ B ^ res) & 0x10);
    SetFlag(FLAG_CY, B > A);
    return 4;
  }
  case 0xB9: { // CP A, C
    uint8_t res = A - C;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A ^ C ^ res) & 0x10);
    SetFlag(FLAG_CY, C > A);
    return 4;
  }
  case 0xBA: { // CP A, D
    uint8_t res = A - D;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A ^ D ^ res) & 0x10);
    SetFlag(FLAG_CY, D > A);
    return 4;
  }
  case 0xBB: { // CP A, E
    uint8_t res = A - E;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A ^ E ^ res) & 0x10);
    SetFlag(FLAG_CY, E > A);
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
  case 0xC2: { // JP NZ, a16
    uint8_t lo = bus.Read(PC++);
    uint8_t hi = bus.Read(PC++);
    if (!GetFlag(FLAG_Z)) {
      PC = (hi << 8) | lo;
      return 16;
    }
    return 12;
  }
  case 0xC3: { // JP a16
    uint8_t lo = bus.Read(PC++);
    uint8_t hi = bus.Read(PC++);
    PC = (hi << 8) | lo;
    return 16;
  }
  case 0xC4: { // CALL NZ, a16
    uint8_t lo = bus.Read(PC++);
    uint8_t hi = bus.Read(PC++);
    if (!GetFlag(FLAG_Z)) {
      bus.Write(--SP, PC >> 8);
      bus.Write(--SP, PC & 0x00FF);
      PC = (hi << 8) | lo;
      return 24;
    }
    return 12;
  }
  case 0xC5: { // PUSH BC
    bus.Write(--SP, B);
    bus.Write(--SP, C);
    return 16;
  }
  case 0xC6: { // ADD A, n8
    uint8_t byte = bus.Read(PC++);
    uint16_t res = A + byte;
    SetFlag(FLAG_Z, (res & 0xFF) == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (A ^ byte ^ res) & 0x10);
    SetFlag(FLAG_CY, res & 0x100);
    A = res;
    return 8;
  }
  case 0xC8: { // RET Z
    if (GetFlag(FLAG_Z)) {
      uint8_t lo = bus.Read(SP++);
      uint8_t hi = bus.Read(SP++);
      PC = (hi << 8) | lo;
      return 20;
    }
    return 8;
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
  case 0xCE: { // ADC A, n8
    uint8_t CY = GetFlag(FLAG_CY);
    uint8_t byte = bus.Read(PC++);
    uint16_t res = A + byte + CY;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (A ^ byte ^ CY ^ res) & 0x10);
    SetFlag(FLAG_CY, res & 0x100);
    A = res;
    return 8;
  }
  case 0xD0: { // RET NC
    if (!GetFlag(FLAG_CY)) {
      uint8_t lo = bus.Read(SP++);
      uint8_t hi = bus.Read(SP++);
      PC = (hi << 8) | lo;
      return 20;
    }
    return 8;
  }
  case 0xD1: { // POP DE
    E = bus.Read(SP++);
    D = bus.Read(SP++);
    return 12;
  }
  case 0xD5: { // PUSH DE
    bus.Write(--SP, D);
    bus.Write(--SP, E);
    return 16;
  }
  case 0xD6: { // SUB A, n8
    uint8_t byte = bus.Read(PC++);
    uint8_t res = A - byte;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A ^ byte ^ res) & 0x10);
    SetFlag(FLAG_CY, byte > A);
    A = res;
    return 8;
  }
  case 0xD8: { // RET C
    if (GetFlag(FLAG_CY)) {
      uint8_t lo = bus.Read(SP++);
      uint8_t hi = bus.Read(SP++);
      PC = (hi << 8) | lo;
      return 20;
    }
    return 8;
  }
  case 0xE0: { // LDH [a8], A
    uint8_t lo = bus.Read(PC++);
    bus.Write(0xFF00 | lo, A);
    return 12;
  }
  case 0xE1: { // POP HL
    L = bus.Read(SP++);
    H = bus.Read(SP++);
    return 12;
  }
  case 0xE2: { // LDH [C], A
    bus.Write(0xFF00 | C, A);
    return 8;
  }
  case 0xE5: { // PUSH HL
    bus.Write(--SP, H);
    bus.Write(--SP, L);
    return 16;
  }
  case 0xE6: { // AND A, n8
    A &= bus.Read(PC++);
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    SetFlag(FLAG_CY, false);
    return 4;
  }
  case 0xE8: { // ADD SP, e8
    int8_t offset = static_cast<int8_t>(bus.Read(PC++));
    uint8_t unsigned_offset = static_cast<uint8_t>(offset);
    uint16_t res = SP + offset;
    SetFlag(FLAG_Z, false);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (SP ^ unsigned_offset ^ res) & 0x10);
    SetFlag(FLAG_CY, ((SP & 0xFF) + unsigned_offset) > 0xFF);
    SP = res;
    return 16;
  }
  case 0xE9: { // JP HL
    PC = GetHL();
    return 4;
  }
  case 0xEA: { // LD [a16], A
    uint8_t lo = bus.Read(PC++);
    uint8_t hi = bus.Read(PC++);
    bus.Write((hi << 8) | lo, A);
    return 16;
  }
  case 0xEE: { // XOR A, n8
    A ^= bus.Read(PC++);
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
    return 8;
  }
  case 0xF0: { // LDH A, [a8]
    uint8_t lo = bus.Read(PC++);
    A = bus.Read(0xFF00 | lo);
    return 12;
  }
  case 0xF1: { // POP AF
    F = bus.Read(SP++) & 0xF0;
    A = bus.Read(SP++);
    return 12;
  }
  case 0xF3: { // DI
    return 4;  // TODO:
  }
  case 0xF5: { // PUSH AF
    bus.Write(--SP, A);
    bus.Write(--SP, F & 0xF0);
    return 16;
  }
  case 0xF8: { // LD HL, SP + e8
    int8_t offset = static_cast<int8_t>(bus.Read(PC++));
    uint8_t unsigned_offset = static_cast<uint8_t>(offset);
    uint16_t res = SP + offset;
    SetFlag(FLAG_Z, false);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (SP ^ unsigned_offset ^ res) & 0x10);
    SetFlag(FLAG_CY, ((SP & 0xFF) + unsigned_offset) > 0xFF);
    SetHL(res);
    return 12;
  }
  case 0xF9: { // LD SP, HL
    SP = GetHL();
    return 8;
  }
  case 0xFA: { // LD A, [a16]
    uint8_t lo = bus.Read(PC++);
    uint8_t hi = bus.Read(PC++);
    A = bus.Read((hi << 8) | lo);
    return 16;
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
  case 0x19: { // RR C
    uint8_t CY = C & 0x01;
    C = (C >> 1) | (GetFlag(FLAG_CY) << 7);
    SetFlag(FLAG_Z, C == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, CY);
    return 8;
  }
  case 0x1A: { // RR D
    uint8_t CY = D & 0x01;
    D = (D >> 1) | (GetFlag(FLAG_CY) << 7);
    SetFlag(FLAG_Z, D == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, CY);
    return 8;
  }
  case 0x1B: { // RR E
    uint8_t CY = E & 0x01;
    E = (E >> 1) | (GetFlag(FLAG_CY) << 7);
    SetFlag(FLAG_Z, E == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, CY);
    return 8;
  }
  case 0x37: { // SWAP A
    A = (A >> 4) | (A << 4);
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
    return 8;
  }
  case 0x38: { // SRL B
    uint8_t CY = B & 0x01;
    B >>= 1;
    SetFlag(FLAG_Z, B == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, CY);
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
