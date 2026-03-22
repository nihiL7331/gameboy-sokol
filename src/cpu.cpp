#include "cpu.hpp"
#include <format>
#include <iostream>

uint8_t CPU::HandleInterrupts() {
  uint8_t cycles = 0;

  uint8_t IE = bus.Read(0xFFFF);
  uint8_t IF = bus.Read(0xFF0F);
  uint8_t pending = IE & IF & 0x1F;
  if (pending)
    HALT = false;
  if (IME) {
    if (pending) {
      IME = false;
      bus.Write(--SP, PC >> 8);
      bus.Write(--SP, PC & 0x00FF);
      if (pending & 0x01) { // VBlank
        PC = 0x0040;
        bus.Write(0xFF0F, IF & 0xFE);
      } else if (pending & 0x02) { // LCD
        PC = 0x0048;
        bus.Write(0xFF0F, IF & 0xFD);
      } else if (pending & 0x04) { // Timer
        PC = 0x0050;
        bus.Write(0xFF0F, IF & 0xFB);
      } else if (pending & 0x08) { // Serial
        PC = 0x0058;
        bus.Write(0xFF0F, IF & 0xF7);
      } else if (pending & 0x10) { // Joypad
        PC = 0x0060;
        bus.Write(0xFF0F, IF & 0xEF);
      }

      cycles = 20;
    }
  }

  if (enabling_IME) {
    IME = true;
    enabling_IME = false;
  }

  return cycles;
}

uint8_t CPU::Step() {
  uint8_t opcode = bus.Read(PC);

  if (HALT_BUG)
    HALT_BUG = false;
  else
    PC++;

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
  case 0x02: { // LD [BC], A
    bus.Write(GetBC(), A);
    return 8;
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
  case 0x07: { // RLCA
    A = (A << 1) | (A >> 7);
    SetFlag(FLAG_Z, false);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, A & 0x01);
    return 4;
  }
  case 0x08: { // LD [a16], SP
    uint8_t lo = bus.Read(PC++);
    uint8_t hi = bus.Read(PC++);
    uint16_t addr = (hi << 8) | lo;
    bus.Write(addr, SP & 0xFF);
    bus.Write(addr + 1, SP >> 8);
    return 20;
  }
  case 0x09: { // ADD HL, BC
    uint16_t HL = GetHL();
    uint16_t BC = GetBC();
    uint32_t res = HL + BC;
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, ((HL & 0xFFF) + (BC & 0xFFF)) > 0xFFF);
    SetFlag(FLAG_CY, res & 0x00010000);
    SetHL(res & 0x0000FFFF);
    return 8;
  }
  case 0x0A: { // LD A, [BC]
    A = bus.Read(GetBC());
    return 8;
  }
  case 0x0B: { // DEC BC
    SetBC(GetBC() - 1);
    return 8;
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
  case 0x0F: { // RRCA
    uint8_t CY = A & 0x01;
    A = (A >> 1) | (A << 7);
    SetFlag(FLAG_Z, false);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, CY);
    return 4;
  }
  case 0x10: { // STOP n8
    PC++;
    return 4;
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
  case 0x19: { // ADD HL, DE
    uint16_t HL = GetHL();
    uint16_t DE = GetDE();
    uint32_t res = HL + DE;
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, ((HL & 0xFFF) + (DE & 0xFFF)) > 0xFFF);
    SetFlag(FLAG_CY, res & 0x00010000);
    SetHL(res & 0x0000FFFF);
    return 8;
  }
  case 0x1A: { // LD A, [DE]
    A = bus.Read(GetDE());
    return 8;
  }
  case 0x1B: { // DEC DE
    SetDE(GetDE() - 1);
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
    SetFlag(FLAG_HCY, ((HL & 0xFFF) << 1) > 0xFFF);
    SetFlag(FLAG_CY, res & 0x00010000);
    SetHL(res);
    return 8;
  }
  case 0x2A: { // LD A, [HL+]
    A = bus.Read(GetHL());
    SetHL(GetHL() + 1);
    return 8;
  }
  case 0x2B: { // DEC HL
    SetHL(GetHL() - 1);
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
  case 0x2F: { // CPL
    A = ~A;
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, true);
    return 4;
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
  case 0x34: { // INC [HL]
    uint16_t HL = GetHL();
    uint8_t byte = bus.Read(HL);
    uint8_t res = byte + 1;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (byte ^ 1 ^ res) & 0x10);
    bus.Write(HL, res);
    return 12;
  }
  case 0x35: { // DEC [HL]
    uint16_t HL = GetHL();
    uint8_t byte = bus.Read(HL);
    uint8_t res = byte - 1;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (byte ^ 1 ^ res) & 0x10);
    bus.Write(HL, res);
    return 12;
  }
  case 0x36: { // LD [HL], n8
    bus.Write(GetHL(), bus.Read(PC++));
    return 12;
  }
  case 0x37: { // SCF
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, true);
    return 4;
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
  case 0x3A: { // LD A, [HL-]
    uint16_t HL = GetHL();
    A = bus.Read(HL);
    SetHL(HL - 1);
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
  case 0x3F: { // CCF
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, !GetFlag(FLAG_CY));
    return 4;
  }
  case 0x40: { // LD B, B
    return 4;
  }
  case 0x41: { // LD B, C
    B = C;
    return 4;
  }
  case 0x42: { // LD B, D
    B = D;
    return 4;
  }
  case 0x43: { // LD B, E
    B = E;
    return 4;
  }
  case 0x44: { // LD B, H
    B = H;
    return 4;
  }
  case 0x45: { // LD B, L
    B = L;
    return 4;
  }
  case 0x46: { // LD B, [HL]
    B = bus.Read(GetHL());
    return 8;
  }
  case 0x47: { // LD B, A
    B = A;
    return 4;
  }
  case 0x48: { // LD C, B
    C = B;
    return 4;
  }
  case 0x49: { // LD C, C
    return 4;
  }
  case 0x4A: { // LD C, D
    C = D;
    return 4;
  }
  case 0x4B: { // LD C, E
    C = E;
    return 4;
  }
  case 0x4C: { // LD C, H
    C = H;
    return 4;
  }
  case 0x4D: { // LD C, L
    C = L;
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
  case 0x50: { // LD D, B
    D = B;
    return 4;
  }
  case 0x51: { // LD D, C
    D = C;
    return 4;
  }
  case 0x52: { // LD D, D
    return 4;
  }
  case 0x53: { // LD D, E
    D = E;
    return 4;
  }
  case 0x54: { // LD D, H
    D = H;
    return 4;
  }
  case 0x55: { // LD D, L
    D = L;
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
  case 0x58: { // LD E, B
    E = B;
    return 4;
  }
  case 0x59: { // LD E, C
    E = C;
    return 4;
  }
  case 0x5A: { // LD E, D
    E = D;
    return 4;
  }
  case 0x5B: { // LD E, E
    return 4;
  }
  case 0x5C: { // LD E, H
    E = H;
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
  case 0x60: { // LD H, B
    H = B;
    return 4;
  }
  case 0x61: { // LD H, C
    H = C;
    return 4;
  }
  case 0x62: { // LD H, D
    H = D;
    return 4;
  }
  case 0x63: { // LD H, E
    H = E;
    return 4;
  }
  case 0x64: { // LD H, H
    return 4;
  }
  case 0x65: { // LD H, L
    H = L;
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
  case 0x68: { // LD L, B
    L = B;
    return 4;
  }
  case 0x69: { // LD L, C
    L = C;
    return 4;
  }
  case 0x6A: { // LD L, D
    L = D;
    return 4;
  }
  case 0x6B: { // LD L, E
    L = E;
    return 4;
  }
  case 0x6C: { // LD L, H
    L = H;
    return 4;
  }
  case 0x6D: { // LD L, L
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
  case 0x74: { // LD [HL], H
    bus.Write(GetHL(), H);
    return 8;
  }
  case 0x75: { // LD [HL], L
    bus.Write(GetHL(), L);
    return 8;
  }
  case 0x76: { // HALT
    if (!IME &&
        (bus.Read(0xFFFF) & bus.Read(0xFF0F) & 0x1F) > 0) // HALT bug behavior
      HALT_BUG = true;
    else
      HALT = true;
    return 4;
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
  case 0x7F: { // LD A, A
    return 4;
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
  case 0x81: { // ADD A, C
    uint16_t res = A + C;
    SetFlag(FLAG_Z, (res & 0x00FF) == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (A ^ C ^ res) & 0x0010);
    SetFlag(FLAG_CY, res & 0x0100);
    A = res;
    return 4;
  }
  case 0x82: { // ADD A, D
    uint16_t res = A + D;
    SetFlag(FLAG_Z, (res & 0x00FF) == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (A ^ D ^ res) & 0x0010);
    SetFlag(FLAG_CY, res & 0x0100);
    A = res;
    return 4;
  }
  case 0x83: { // ADD A, E
    uint16_t res = A + E;
    SetFlag(FLAG_Z, (res & 0x00FF) == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (A ^ E ^ res) & 0x0010);
    SetFlag(FLAG_CY, res & 0x0100);
    A = res;
    return 4;
  }
  case 0x84: { // ADD A, H
    uint16_t res = A + H;
    SetFlag(FLAG_Z, (res & 0x00FF) == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (A ^ H ^ res) & 0x0010);
    SetFlag(FLAG_CY, res & 0x0100);
    A = res;
    return 4;
  }
  case 0x85: { // ADD A, L
    uint16_t res = A + L;
    SetFlag(FLAG_Z, (res & 0x00FF) == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (A ^ L ^ res) & 0x0010);
    SetFlag(FLAG_CY, res & 0x0100);
    A = res;
    return 4;
  }
  case 0x86: { // ADD A, [HL]
    uint8_t byte = bus.Read(GetHL());
    uint16_t res = A + byte;
    SetFlag(FLAG_Z, (res & 0x00FF) == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (A ^ byte ^ res) & 0x10);
    SetFlag(FLAG_CY, res & 0x0100);
    A = res;
    return 8;
  }
  case 0x87: { // ADD A, A
    uint16_t res = A << 1;
    SetFlag(FLAG_Z, (res & 0x00FF) == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, res & 0x0010);
    SetFlag(FLAG_CY, res & 0x0100);
    A = res;
    return 4;
  }
  case 0x88: { // ADC A, B
    uint8_t CY = GetFlag(FLAG_CY);
    uint16_t res = A + B + CY;
    SetFlag(FLAG_Z, (res & 0x00FF) == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (A ^ B ^ CY ^ res) & 0x0010);
    SetFlag(FLAG_CY, res & 0x0100);
    A = res;
    return 4;
  }
  case 0x89: { // ADC A, C
    uint8_t CY = GetFlag(FLAG_CY);
    uint16_t res = A + C + CY;
    SetFlag(FLAG_Z, (res & 0x00FF) == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (A ^ C ^ CY ^ res) & 0x0010);
    SetFlag(FLAG_CY, res & 0x0100);
    A = res;
    return 4;
  }
  case 0x8A: { // ADC A, D
    uint8_t CY = GetFlag(FLAG_CY);
    uint16_t res = A + D + CY;
    SetFlag(FLAG_Z, (res & 0x00FF) == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (A ^ D ^ CY ^ res) & 0x0010);
    SetFlag(FLAG_CY, res & 0x0100);
    A = res;
    return 4;
  }
  case 0x8B: { // ADC A, E
    uint8_t CY = GetFlag(FLAG_CY);
    uint16_t res = A + E + CY;
    SetFlag(FLAG_Z, (res & 0x00FF) == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (A ^ E ^ CY ^ res) & 0x0010);
    SetFlag(FLAG_CY, res & 0x0100);
    A = res;
    return 4;
  }
  case 0x8C: { // ADC A, H
    uint8_t CY = GetFlag(FLAG_CY);
    uint16_t res = A + H + CY;
    SetFlag(FLAG_Z, (res & 0x00FF) == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (A ^ H ^ CY ^ res) & 0x0010);
    SetFlag(FLAG_CY, res & 0x0100);
    A = res;
    return 4;
  }
  case 0x8D: { // ADC A, L
    uint8_t CY = GetFlag(FLAG_CY);
    uint16_t res = A + L + CY;
    SetFlag(FLAG_Z, (res & 0x00FF) == 0x0000);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (A ^ L ^ CY ^ res) & 0x0010);
    SetFlag(FLAG_CY, res & 0x0100);
    A = res;
    return 4;
  }
  case 0x8E: { // ADC A, [HL]
    uint8_t CY = GetFlag(FLAG_CY);
    uint8_t byte = bus.Read(GetHL());
    uint16_t res = A + byte + CY;
    SetFlag(FLAG_Z, (res & 0x00FF) == 0x0000);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (A ^ byte ^ CY ^ res) & 0x0010);
    SetFlag(FLAG_CY, res & 0x0100);
    A = res;
    return 8;
  }
  case 0x8F: { // ADC A, A
    uint16_t res = (A << 1) | GetFlag(FLAG_CY);
    SetFlag(FLAG_Z, (res & 0x00FF) == 0x0000);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, res & 0x0010);
    SetFlag(FLAG_CY, res & 0x0100);
    A = res;
    return 4;
  }
  case 0x90: { // SUB A, B
    uint8_t res = A - B;
    SetFlag(FLAG_Z, res == 0x00);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A & 0x0F) < (B & 0x0F));
    SetFlag(FLAG_CY, B > A);
    A = res;
    return 4;
  }
  case 0x91: { // SUB A, C
    uint8_t res = A - C;
    SetFlag(FLAG_Z, res == 0x00);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A & 0x0F) < (C & 0x0F));
    SetFlag(FLAG_CY, C > A);
    A = res;
    return 4;
  }
  case 0x92: { // SUB A, D
    uint8_t res = A - D;
    SetFlag(FLAG_Z, res == 0x00);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A & 0x0F) < (D & 0x0F));
    SetFlag(FLAG_CY, D > A);
    A = res;
    return 4;
  }
  case 0x93: { // SUB A, E
    uint8_t res = A - E;
    SetFlag(FLAG_Z, res == 0x00);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A & 0x0F) < (E & 0x0F));
    SetFlag(FLAG_CY, E > A);
    A = res;
    return 4;
  }
  case 0x94: { // SUB A, H
    uint8_t res = A - H;
    SetFlag(FLAG_Z, res == 0x00);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A & 0x0F) < (H & 0x0F));
    SetFlag(FLAG_CY, H > A);
    A = res;
    return 4;
  }
  case 0x95: { // SUB A, L
    uint8_t res = A - L;
    SetFlag(FLAG_Z, res == 0x00);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A & 0x0F) < (L & 0x0F));
    SetFlag(FLAG_CY, L > A);
    A = res;
    return 4;
  }
  case 0x96: { // SUB A, [HL]
    uint8_t byte = bus.Read(GetHL());
    uint8_t res = A - byte;
    SetFlag(FLAG_Z, res == 0x00);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A & 0x0F) < (byte & 0x0F));
    SetFlag(FLAG_CY, byte > A);
    A = res;
    return 8;
  }
  case 0x97: { // SUB A, A
    SetFlag(FLAG_Z, true);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
    A = 0;
    return 4;
  }
  case 0x98: { // SBC A, B
    uint8_t CY = GetFlag(FLAG_CY);
    uint16_t sub = B + CY;
    uint8_t res = A - sub;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A & 0x0F) < ((B & 0x0F) + CY));
    SetFlag(FLAG_CY, sub > A);
    A = res;
    return 4;
  }
  case 0x99: { // SBC A, C
    uint8_t CY = GetFlag(FLAG_CY);
    uint16_t sub = C + CY;
    uint8_t res = A - sub;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A & 0x0F) < ((C & 0x0F) + CY));
    SetFlag(FLAG_CY, sub > A);
    A = res;
    return 4;
  }
  case 0x9A: { // SBC A, D
    uint8_t CY = GetFlag(FLAG_CY);
    uint16_t sub = D + CY;
    uint8_t res = A - sub;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A & 0x0F) < ((D & 0x0F) + CY));
    SetFlag(FLAG_CY, sub > A);
    A = res;
    return 4;
  }
  case 0x9B: { // SBC A, E
    uint8_t CY = GetFlag(FLAG_CY);
    uint16_t sub = E + CY;
    uint8_t res = A - sub;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A & 0x0F) < ((E & 0x0F) + CY));
    SetFlag(FLAG_CY, sub > A);
    A = res;
    return 4;
  }
  case 0x9C: { // SBC A, H
    uint8_t CY = GetFlag(FLAG_CY);
    uint16_t sub = H + CY;
    uint8_t res = A - sub;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A & 0x0F) < ((H & 0x0F) + CY));
    SetFlag(FLAG_CY, sub > A);
    A = res;
    return 4;
  }
  case 0x9D: { // SBC A, L
    uint8_t CY = GetFlag(FLAG_CY);
    uint16_t sub = L + CY;
    uint8_t res = A - sub;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A & 0x0F) < ((L & 0x0F) + CY));
    SetFlag(FLAG_CY, sub > A);
    A = res;
    return 4;
  }
  case 0x9E: { // SBC A, [HL]
    uint8_t byte = bus.Read(GetHL());
    uint8_t CY = GetFlag(FLAG_CY);
    uint16_t sub = byte + CY;
    uint8_t res = A - sub;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A & 0x0F) < ((byte & 0x0F) + CY));
    SetFlag(FLAG_CY, sub > A);
    A = res;
    return 8;
  }
  case 0x9F: { // SBC A, A
    uint8_t CY = GetFlag(FLAG_CY);
    SetFlag(FLAG_Z, CY == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, CY != 0);
    SetFlag(FLAG_CY, CY != 0);
    A = -CY;
    return 4;
  }
  case 0xA0: { // AND A, B
    A &= B;
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    SetFlag(FLAG_CY, false);
    return 4;
  }
  case 0xA1: { // AND A, C
    A &= C;
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    SetFlag(FLAG_CY, false);
    return 4;
  }
  case 0xA2: { // AND A, D
    A &= D;
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    SetFlag(FLAG_CY, false);
    return 4;
  }
  case 0xA3: { // AND A, E
    A &= E;
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    SetFlag(FLAG_CY, false);
    return 4;
  }
  case 0xA4: { // AND A, H
    A &= H;
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    SetFlag(FLAG_CY, false);
    return 4;
  }
  case 0xA5: { // AND A, L
    A &= L;
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    SetFlag(FLAG_CY, false);
    return 4;
  }
  case 0xA6: { // AND A, [HL]
    uint8_t byte = bus.Read(GetHL());
    A &= byte;
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    SetFlag(FLAG_CY, false);
    return 8;
  }
  case 0xA7: { // AND A, A
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    SetFlag(FLAG_CY, false);
    return 4;
  }
  case 0xA8: { // XOR A, B
    A ^= B;
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
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
  case 0xAA: { // XOR A, D
    A ^= D;
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
    return 4;
  }
  case 0xAB: { // XOR A, E
    A ^= E;
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
    return 4;
  }
  case 0xAC: { // XOR A, H
    A ^= H;
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
  case 0xB2: { // OR A, D
    A |= D;
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
    return 4;
  }
  case 0xB3: { // OR A, E
    A |= E;
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
    return 4;
  }
  case 0xB4: { // OR A, H
    A |= H;
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
    return 4;
  }
  case 0xB5: { // OR A, L
    A |= L;
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
    SetFlag(FLAG_HCY, (A & 0x0F) < (B & 0x0F));
    SetFlag(FLAG_CY, B > A);
    return 4;
  }
  case 0xB9: { // CP A, C
    uint8_t res = A - C;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A & 0x0F) < (C & 0x0F));
    SetFlag(FLAG_CY, C > A);
    return 4;
  }
  case 0xBA: { // CP A, D
    uint8_t res = A - D;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A & 0x0F) < (D & 0x0F));
    SetFlag(FLAG_CY, D > A);
    return 4;
  }
  case 0xBB: { // CP A, E
    uint8_t res = A - E;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A & 0x0F) < (E & 0x0F));
    SetFlag(FLAG_CY, E > A);
    return 4;
  }
  case 0xBC: { // CP A, H
    uint8_t res = A - H;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A & 0x0F) < (H & 0x0F));
    SetFlag(FLAG_CY, H > A);
    return 4;
  }
  case 0xBD: { // CP A, L
    uint8_t res = A - L;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A & 0x0F) < (L & 0x0F));
    SetFlag(FLAG_CY, L > A);
    return 4;
  }
  case 0xBE: { // CP A, [HL]
    uint8_t byte = bus.Read(GetHL());
    uint8_t res = A - byte;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A & 0x0F) < (byte & 0x0F));
    SetFlag(FLAG_CY, byte > A);
    return 8;
  }
  case 0xBF: { // CP A, A
    SetFlag(FLAG_Z, true);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
    return 4;
  }
  case 0xC0: { // RET NZ
    if (!GetFlag(FLAG_Z)) {
      uint8_t lo = bus.Read(SP++);
      uint8_t hi = bus.Read(SP++);
      PC = (hi << 8) | lo;
      return 20;
    }
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
  case 0xC7: { // RST $00
    bus.Write(--SP, PC >> 8);
    bus.Write(--SP, PC & 0x00FF);
    PC = 0x0000;
    return 16;
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
  case 0xCA: { // JP Z, a16
    uint8_t lo = bus.Read(PC++);
    uint8_t hi = bus.Read(PC++);
    if (GetFlag(FLAG_Z)) {
      PC = (hi << 8) | lo;
      return 16;
    }
    return 12;
  }
  case 0xCB: { // PREFIX
    return ExecuteCB();
  }
  case 0xCC: { // CALL Z, a16
    uint8_t lo = bus.Read(PC++);
    uint8_t hi = bus.Read(PC++);
    if (GetFlag(FLAG_Z)) {
      bus.Write(--SP, PC >> 8);
      bus.Write(--SP, PC & 0x00FF);
      PC = (hi << 8) | lo;
      return 24;
    }
    return 12;
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
    SetFlag(FLAG_Z, (res & 0x00FF) == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, (A ^ byte ^ CY ^ res) & 0x0010);
    SetFlag(FLAG_CY, res & 0x0100);
    A = res;
    return 8;
  }
  case 0xCF: { // RST $08
    bus.Write(--SP, PC >> 8);
    bus.Write(--SP, PC & 0x00FF);
    PC = 0x0008;
    return 16;
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
  case 0xD2: { // JP NC, a16
    uint8_t lo = bus.Read(PC++);
    uint8_t hi = bus.Read(PC++);
    if (!GetFlag(FLAG_CY)) {
      PC = (hi << 8) | lo;
      return 16;
    }
    return 12;
  }
  case 0xD4: { // CALL NC, a16
    uint8_t lo = bus.Read(PC++);
    uint8_t hi = bus.Read(PC++);
    if (!GetFlag(FLAG_CY)) {
      bus.Write(--SP, PC >> 8);
      bus.Write(--SP, PC & 0x00FF);
      PC = (hi << 8) | lo;
      return 24;
    }
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
    SetFlag(FLAG_HCY, (A & 0x0F) < (byte & 0x0F));
    SetFlag(FLAG_CY, byte > A);
    A = res;
    return 8;
  }
  case 0xD7: { // RST $10
    bus.Write(--SP, PC >> 8);
    bus.Write(--SP, PC & 0x00FF);
    PC = 0x0010;
    return 16;
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
  case 0xD9: { // RETI
    IME = true;
    uint8_t lo = bus.Read(SP++);
    uint8_t hi = bus.Read(SP++);
    PC = (hi << 8) | lo;
    return 16;
  }
  case 0xDA: { // JP C, a16
    uint8_t lo = bus.Read(PC++);
    uint8_t hi = bus.Read(PC++);
    if (GetFlag(FLAG_CY)) {
      PC = (hi << 8) | lo;
      return 16;
    }
    return 12;
  }
  case 0xDC: { // CALL C, a16
    uint8_t lo = bus.Read(PC++);
    uint8_t hi = bus.Read(PC++);
    if (GetFlag(FLAG_CY)) {
      bus.Write(--SP, PC >> 8);
      bus.Write(--SP, PC & 0x00FF);
      PC = (hi << 8) | lo;
      return 24;
    }
    return 12;
  }
  case 0xDE: { // SBC A, n8
    uint8_t CY = GetFlag(FLAG_CY);
    uint8_t byte = bus.Read(PC++);
    uint8_t res = A - byte - CY;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A & 0x0F) < ((byte & 0x0F) + CY));
    SetFlag(FLAG_CY, byte + CY > A);
    A = res;
    return 8;
  }
  case 0xDF: { // RST $18
    bus.Write(--SP, PC >> 8);
    bus.Write(--SP, PC & 0x00FF);
    PC = 0x0018;
    return 16;
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
    return 8;
  }
  case 0xE7: { // RST $20
    bus.Write(--SP, PC >> 8);
    bus.Write(--SP, PC & 0x00FF);
    PC = 0x0020;
    return 16;
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
  case 0xEF: { // RST $28
    bus.Write(--SP, PC >> 8);
    bus.Write(--SP, PC & 0x00FF);
    PC = 0x0028;
    return 16;
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
  case 0xF2: { // LDH A, [C]
    A = bus.Read(0xFF00 | C);
    return 8;
  }
  case 0xF3: { // DI
    IME = false;
    enabling_IME = false;
    return 4;
  }
  case 0xF5: { // PUSH AF
    bus.Write(--SP, A);
    bus.Write(--SP, F & 0xF0);
    return 16;
  }
  case 0xF6: { // OR A, n8
    A |= bus.Read(PC++);
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
    return 8;
  }
  case 0xF7: { // RST $30
    bus.Write(--SP, PC >> 8);
    bus.Write(--SP, PC & 0x00FF);
    PC = 0x0030;
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
  case 0xFB: { // EI
    enabling_IME = true;
    return 4;
  }
  case 0xFE: { // CP A, n8
    uint8_t byte = bus.Read(PC++);
    uint8_t res = A - byte;
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, true);
    SetFlag(FLAG_HCY, (A & 0x0F) < (byte & 0x0F));
    SetFlag(FLAG_CY, byte > A);
    return 8;
  }
  case 0xFF: { // RST $38
    bus.Write(--SP, PC >> 8);
    bus.Write(--SP, PC & 0x00FF);
    PC = 0x0038;
    return 16;
  }
  default:
    std::cout << std::format("Unimplemented opcode 0x{:02X} at PC=0x{:04X}\n",
                             static_cast<unsigned int>(opcode), PC - 1);
    exit(1);
  }
}

uint8_t CPU::ExecuteCB() {
  uint8_t cb_opcode = bus.Read(PC++);

  switch (cb_opcode) {
  case 0x00: { // RLC B
    B = (B << 1) | (B >> 7);
    SetFlag(FLAG_Z, B == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, B & 0x01);
    return 8;
  }
  case 0x01: { // RLC C
    C = (C << 1) | (C >> 7);
    SetFlag(FLAG_Z, C == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, C & 0x01);
    return 8;
  }
  case 0x02: { // RLC D
    D = (D << 1) | (D >> 7);
    SetFlag(FLAG_Z, D == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, D & 0x01);
    return 8;
  }
  case 0x03: { // RLC E
    E = (E << 1) | (E >> 7);
    SetFlag(FLAG_Z, E == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, E & 0x01);
    return 8;
  }
  case 0x04: { // RLC H
    H = (H << 1) | (H >> 7);
    SetFlag(FLAG_Z, H == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, H & 0x01);
    return 8;
  }
  case 0x05: { // RLC L
    L = (L << 1) | (L >> 7);
    SetFlag(FLAG_Z, L == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, L & 0x01);
    return 8;
  }
  case 0x06: { // RLC [HL]
    uint16_t HL = GetHL();
    uint8_t byte = bus.Read(HL);
    byte = (byte << 1) | (byte >> 7);
    SetFlag(FLAG_Z, byte == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, byte & 0x01);
    bus.Write(HL, byte);
    return 16;
  }
  case 0x07: { // RLC A
    A = (A << 1) | (A >> 7);
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, A & 0x01);
    return 8;
  }
  case 0x08: { // RRC B
    uint8_t CY = B & 0x01;
    B = (B >> 1) | (B << 7);
    SetFlag(FLAG_Z, B == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, CY);
    return 8;
  }
  case 0x09: { // RRC C
    uint8_t CY = C & 0x01;
    C = (C >> 1) | (C << 7);
    SetFlag(FLAG_Z, C == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, CY);
    return 8;
  }
  case 0x0A: { // RRC D
    uint8_t CY = D & 0x01;
    D = (D >> 1) | (D << 7);
    SetFlag(FLAG_Z, D == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, CY);
    return 8;
  }
  case 0x0B: { // RRC E
    uint8_t CY = E & 0x01;
    E = (E >> 1) | (E << 7);
    SetFlag(FLAG_Z, E == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, CY);
    return 8;
  }
  case 0x0C: { // RRC H
    uint8_t CY = H & 0x01;
    H = (H >> 1) | (H << 7);
    SetFlag(FLAG_Z, H == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, CY);
    return 8;
  }
  case 0x0D: { // RRC L
    uint8_t CY = L & 0x01;
    L = (L >> 1) | (L << 7);
    SetFlag(FLAG_Z, L == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, CY);
    return 8;
  }
  case 0x0E: { // RRC [HL]
    uint16_t HL = GetHL();
    uint8_t byte = bus.Read(HL);
    uint8_t CY = byte & 0x01;
    byte = (byte >> 1) | (byte << 7);
    SetFlag(FLAG_Z, byte == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, CY);
    bus.Write(HL, byte);
    return 16;
  }
  case 0x0F: { // RRC A
    uint8_t CY = A & 0x01;
    A = (A >> 1) | (A << 7);
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, CY);
    return 8;
  }
  case 0x10: { // RL B
    uint8_t res = (B << 1) | GetFlag(FLAG_CY);
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, B & 0x80);
    B = res;
    return 8;
  }
  case 0x11: { // RL C
    uint8_t res = (C << 1) | GetFlag(FLAG_CY);
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, C & 0x80);
    C = res;
    return 8;
  }
  case 0x12: { // RL D
    uint8_t res = (D << 1) | GetFlag(FLAG_CY);
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, D & 0x80);
    D = res;
    return 8;
  }
  case 0x13: { // RL E
    uint8_t res = (E << 1) | GetFlag(FLAG_CY);
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, E & 0x80);
    E = res;
    return 8;
  }
  case 0x14: { // RL H
    uint8_t res = (H << 1) | GetFlag(FLAG_CY);
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, H & 0x80);
    H = res;
    return 8;
  }
  case 0x15: { // RL L
    uint8_t res = (L << 1) | GetFlag(FLAG_CY);
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, L & 0x80);
    L = res;
    return 8;
  }
  case 0x16: { // RL [HL]
    uint16_t HL = GetHL();
    uint8_t byte = bus.Read(HL);
    uint8_t res = (byte << 1) | GetFlag(FLAG_CY);
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, byte & 0x80);
    bus.Write(HL, res);
    return 16;
  }
  case 0x17: { // RL A
    uint8_t res = (A << 1) | GetFlag(FLAG_CY);
    SetFlag(FLAG_Z, res == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, A & 0x80);
    A = res;
    return 8;
  }
  case 0x18: { // RR B
    uint8_t CY = B & 0x01;
    B = (B >> 1) | (GetFlag(FLAG_CY) << 7);
    SetFlag(FLAG_Z, B == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, CY);
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
  case 0x1C: { // RR H
    uint8_t CY = H & 0x01;
    H = (H >> 1) | (GetFlag(FLAG_CY) << 7);
    SetFlag(FLAG_Z, H == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, CY);
    return 8;
  }
  case 0x1D: { // RR L
    uint8_t CY = L & 0x01;
    L = (L >> 1) | (GetFlag(FLAG_CY) << 7);
    SetFlag(FLAG_Z, L == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, CY);
    return 8;
  }
  case 0x1E: { // RR [HL]
    uint16_t HL = GetHL();
    uint8_t byte = bus.Read(HL);
    uint8_t CY = byte & 0x01;
    byte = (byte >> 1) | (GetFlag(FLAG_CY) << 7);
    SetFlag(FLAG_Z, byte == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, CY);
    bus.Write(HL, byte);
    return 16;
  }
  case 0x1F: { // RR A
    uint8_t CY = A & 0x01;
    A = (A >> 1) | (GetFlag(FLAG_CY) << 7);
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, CY);
    return 8;
  }
  case 0x20: { // SLA B
    SetFlag(FLAG_CY, B >> 7);
    B <<= 1;
    SetFlag(FLAG_Z, B == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    return 8;
  }
  case 0x21: { // SLA C
    SetFlag(FLAG_CY, C >> 7);
    C <<= 1;
    SetFlag(FLAG_Z, C == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    return 8;
  }
  case 0x22: { // SLA D
    SetFlag(FLAG_CY, D >> 7);
    D <<= 1;
    SetFlag(FLAG_Z, D == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    return 8;
  }
  case 0x23: { // SLA E
    SetFlag(FLAG_CY, E >> 7);
    E <<= 1;
    SetFlag(FLAG_Z, E == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    return 8;
  }
  case 0x24: { // SLA H
    SetFlag(FLAG_CY, H >> 7);
    H <<= 1;
    SetFlag(FLAG_Z, H == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    return 8;
  }
  case 0x25: { // SLA L
    SetFlag(FLAG_CY, L >> 7);
    L <<= 1;
    SetFlag(FLAG_Z, L == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    return 8;
  }
  case 0x26: { // SLA [HL]
    uint16_t HL = GetHL();
    uint8_t byte = bus.Read(HL);
    SetFlag(FLAG_CY, byte >> 7);
    byte <<= 1;
    SetFlag(FLAG_Z, byte == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    bus.Write(HL, byte);
    return 16;
  }
  case 0x27: { // SLA A
    SetFlag(FLAG_CY, A >> 7);
    A <<= 1;
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    return 8;
  }
  case 0x28: { // SRA B
    SetFlag(FLAG_CY, B & 0x01);
    B = (B >> 1) | (B & 0x80);
    SetFlag(FLAG_Z, B == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    return 8;
  }
  case 0x29: { // SRA C
    SetFlag(FLAG_CY, C & 0x01);
    C = (C >> 1) | (C & 0x80);
    SetFlag(FLAG_Z, C == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    return 8;
  }
  case 0x2A: { // SRA D
    SetFlag(FLAG_CY, D & 0x01);
    D = (D >> 1) | (D & 0x80);
    SetFlag(FLAG_Z, D == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    return 8;
  }
  case 0x2B: { // SRA E
    SetFlag(FLAG_CY, E & 0x01);
    E = (E >> 1) | (E & 0x80);
    SetFlag(FLAG_Z, E == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    return 8;
  }
  case 0x2C: { // SRA H
    SetFlag(FLAG_CY, H & 0x01);
    H = (H >> 1) | (H & 0x80);
    SetFlag(FLAG_Z, H == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    return 8;
  }
  case 0x2D: { // SRA L
    SetFlag(FLAG_CY, L & 0x01);
    L = (L >> 1) | (L & 0x80);
    SetFlag(FLAG_Z, L == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    return 8;
  }
  case 0x2E: { // SRA [HL]
    uint16_t HL = GetHL();
    uint8_t byte = bus.Read(HL);
    SetFlag(FLAG_CY, byte & 0x01);
    byte = (byte >> 1) | (byte & 0x80);
    SetFlag(FLAG_Z, byte == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    bus.Write(HL, byte);
    return 16;
  }
  case 0x2F: { // SRA A
    SetFlag(FLAG_CY, A & 0x01);
    A = (A >> 1) | (A & 0x80);
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    return 8;
  }
  case 0x30: { // SWAP B
    B = (B >> 4) | (B << 4);
    SetFlag(FLAG_Z, B == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
    return 8;
  }
  case 0x31: { // SWAP C
    C = (C >> 4) | (C << 4);
    SetFlag(FLAG_Z, C == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
    return 8;
  }
  case 0x32: { // SWAP D
    D = (D >> 4) | (D << 4);
    SetFlag(FLAG_Z, D == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
    return 8;
  }
  case 0x33: { // SWAP E
    E = (E >> 4) | (E << 4);
    SetFlag(FLAG_Z, E == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
    return 8;
  }
  case 0x34: { // SWAP H
    H = (H >> 4) | (H << 4);
    SetFlag(FLAG_Z, H == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
    return 8;
  }
  case 0x35: { // SWAP L
    L = (L >> 4) | (L << 4);
    SetFlag(FLAG_Z, L == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
    return 8;
  }
  case 0x36: { // SWAP [HL]
    uint16_t HL = GetHL();
    uint8_t byte = bus.Read(HL);
    byte = (byte >> 4) | (byte << 4);
    SetFlag(FLAG_Z, byte == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, false);
    bus.Write(HL, byte);
    return 16;
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
  case 0x39: { // SRL C
    uint8_t CY = C & 0x01;
    C >>= 1;
    SetFlag(FLAG_Z, C == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, CY);
    return 8;
  }
  case 0x3A: { // SRL D
    uint8_t CY = D & 0x01;
    D >>= 1;
    SetFlag(FLAG_Z, D == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, CY);
    return 8;
  }
  case 0x3B: { // SRL E
    uint8_t CY = E & 0x01;
    E >>= 1;
    SetFlag(FLAG_Z, E == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, CY);
    return 8;
  }
  case 0x3C: { // SRL H
    uint8_t CY = H & 0x01;
    H >>= 1;
    SetFlag(FLAG_Z, H == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, CY);
    return 8;
  }
  case 0x3D: { // SRL L
    uint8_t CY = L & 0x01;
    L >>= 1;
    SetFlag(FLAG_Z, L == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, CY);
    return 8;
  }
  case 0x3E: { // SRL [HL]
    uint16_t HL = GetHL();
    uint8_t byte = bus.Read(HL);
    uint8_t CY = byte & 0x01;
    byte >>= 1;
    SetFlag(FLAG_Z, byte == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, CY);
    bus.Write(HL, byte);
    return 16;
  }
  case 0x3F: { // SRL A
    uint8_t CY = A & 0x01;
    A >>= 1;
    SetFlag(FLAG_Z, A == 0);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, false);
    SetFlag(FLAG_CY, CY);
    return 8;
  }
  case 0x40: { // BIT 0, B
    SetFlag(FLAG_Z, (B & 0x01) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x41: { // BIT 0, C
    SetFlag(FLAG_Z, (C & 0x01) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x42: { // BIT 0, D
    SetFlag(FLAG_Z, (D & 0x01) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x43: { // BIT 0, E
    SetFlag(FLAG_Z, (E & 0x01) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x44: { // BIT 0, H
    SetFlag(FLAG_Z, (H & 0x01) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x45: { // BIT 0, L
    SetFlag(FLAG_Z, (L & 0x01) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x46: { // BIT 0, [HL]
    SetFlag(FLAG_Z, (bus.Read(GetHL()) & 0x01) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 12;
  }
  case 0x47: { // BIT 0, A
    SetFlag(FLAG_Z, (A & 0x01) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x48: { // BIT 1, B
    SetFlag(FLAG_Z, (B & 0x02) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x49: { // BIT 1, C
    SetFlag(FLAG_Z, (C & 0x02) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x4A: { // BIT 1, D
    SetFlag(FLAG_Z, (D & 0x02) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x4B: { // BIT 1, E
    SetFlag(FLAG_Z, (E & 0x02) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x4C: { // BIT 1, H
    SetFlag(FLAG_Z, (H & 0x02) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x4D: { // BIT 1, L
    SetFlag(FLAG_Z, (L & 0x02) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x4E: { // BIT 1, [HL]
    SetFlag(FLAG_Z, (bus.Read(GetHL()) & 0x02) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 12;
  }
  case 0x4F: { // BIT 1, A
    SetFlag(FLAG_Z, (A & 0x02) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x50: { // BIT 2, B
    SetFlag(FLAG_Z, (B & 0x04) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x51: { // BIT 2, C
    SetFlag(FLAG_Z, (C & 0x04) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x52: { // BIT 2, D
    SetFlag(FLAG_Z, (D & 0x04) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x53: { // BIT 2, E
    SetFlag(FLAG_Z, (E & 0x04) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x54: { // BIT 2, H
    SetFlag(FLAG_Z, (H & 0x04) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x55: { // BIT 2, L
    SetFlag(FLAG_Z, (L & 0x04) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x56: { // BIT 2, [HL]
    SetFlag(FLAG_Z, (bus.Read(GetHL()) & 0x04) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 12;
  }
  case 0x57: { // BIT 2, A
    SetFlag(FLAG_Z, (A & 0x04) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x58: { // BIT 3, B
    SetFlag(FLAG_Z, (B & 0x08) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x59: { // BIT 3, C
    SetFlag(FLAG_Z, (C & 0x08) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x5A: { // BIT 3, D
    SetFlag(FLAG_Z, (D & 0x08) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x5B: { // BIT 3, E
    SetFlag(FLAG_Z, (E & 0x08) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x5C: { // BIT 3, H
    SetFlag(FLAG_Z, (H & 0x08) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x5D: { // BIT 3, L
    SetFlag(FLAG_Z, (L & 0x08) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x5E: { // BIT 3, [HL]
    SetFlag(FLAG_Z, (bus.Read(GetHL()) & 0x08) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 12;
  }
  case 0x5F: { // BIT 3, A
    SetFlag(FLAG_Z, (A & 0x08) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x60: { // BIT 4, B
    SetFlag(FLAG_Z, (B & 0x10) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x61: { // BIT 4, C
    SetFlag(FLAG_Z, (C & 0x10) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x62: { // BIT 4, D
    SetFlag(FLAG_Z, (D & 0x10) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x63: { // BIT 4, E
    SetFlag(FLAG_Z, (E & 0x10) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x64: { // BIT 4, H
    SetFlag(FLAG_Z, (H & 0x10) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x65: { // BIT 4, L
    SetFlag(FLAG_Z, (L & 0x10) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x66: { // BIT 4, [HL]
    SetFlag(FLAG_Z, (bus.Read(GetHL()) & 0x10) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 12;
  }
  case 0x67: { // BIT 4, A
    SetFlag(FLAG_Z, (A & 0x10) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x68: { // BIT 5, B
    SetFlag(FLAG_Z, (B & 0x20) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x69: { // BIT 5, C
    SetFlag(FLAG_Z, (C & 0x20) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x6A: { // BIT 5, D
    SetFlag(FLAG_Z, (D & 0x20) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x6B: { // BIT 5, E
    SetFlag(FLAG_Z, (E & 0x20) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x6C: { // BIT 5, H
    SetFlag(FLAG_Z, (H & 0x20) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x6D: { // BIT 5, L
    SetFlag(FLAG_Z, (L & 0x20) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x6E: { // BIT 5, [HL]
    SetFlag(FLAG_Z, (bus.Read(GetHL()) & 0x20) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 12;
  }
  case 0x6F: { // BIT 5, A
    SetFlag(FLAG_Z, (A & 0x20) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x70: { // BIT 6, B
    SetFlag(FLAG_Z, (B & 0x40) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x71: { // BIT 6, C
    SetFlag(FLAG_Z, (C & 0x40) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x72: { // BIT 6, D
    SetFlag(FLAG_Z, (D & 0x40) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x73: { // BIT 6, E
    SetFlag(FLAG_Z, (E & 0x40) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x74: { // BIT 6, H
    SetFlag(FLAG_Z, (H & 0x40) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x75: { // BIT 6, L
    SetFlag(FLAG_Z, (L & 0x40) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x76: { // BIT 6, [HL]
    SetFlag(FLAG_Z, (bus.Read(GetHL()) & 0x40) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 12;
  }
  case 0x77: { // BIT 6, A
    SetFlag(FLAG_Z, (A & 0x40) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x78: { // BIT 7, B
    SetFlag(FLAG_Z, (B & 0x80) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x79: { // BIT 7, C
    SetFlag(FLAG_Z, (C & 0x80) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x7A: { // BIT 7, D
    SetFlag(FLAG_Z, (D & 0x80) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x7B: { // BIT 7, E
    SetFlag(FLAG_Z, (E & 0x80) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x7C: { // BIT 7, H
    SetFlag(FLAG_Z, (H & 0x80) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x7D: { // BIT 7, L
    SetFlag(FLAG_Z, (L & 0x80) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x7E: { // BIT 7, [HL]
    SetFlag(FLAG_Z, (bus.Read(GetHL()) & 0x80) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 12;
  }
  case 0x7F: { // BIT 7, A
    SetFlag(FLAG_Z, (A & 0x80) == 0x00);
    SetFlag(FLAG_N, false);
    SetFlag(FLAG_HCY, true);
    return 8;
  }
  case 0x80: { // RES 0, B
    B &= 0xFE;
    return 8;
  }
  case 0x81: { // RES 0, C
    C &= 0xFE;
    return 8;
  }
  case 0x82: { // RES 0, D
    D &= 0xFE;
    return 8;
  }
  case 0x83: { // RES 0, E
    E &= 0xFE;
    return 8;
  }
  case 0x84: { // RES 0, H
    H &= 0xFE;
    return 8;
  }
  case 0x85: { // RES 0, L
    L &= 0xFE;
    return 8;
  }
  case 0x86: { // RES 0, [HL]
    uint16_t HL = GetHL();
    uint8_t byte = bus.Read(HL);
    byte &= 0xFE;
    bus.Write(HL, byte);
    return 16;
  }
  case 0x87: { // RES 0, A
    A &= 0xFE;
    return 8;
  }
  case 0x88: { // RES 1, B
    B &= 0xFD;
    return 8;
  }
  case 0x89: { // RES 1, C
    C &= 0xFD;
    return 8;
  }
  case 0x8A: { // RES 1, D
    D &= 0xFD;
    return 8;
  }
  case 0x8B: { // RES 1, E
    E &= 0xFD;
    return 8;
  }
  case 0x8C: { // RES 1, H
    H &= 0xFD;
    return 8;
  }
  case 0x8D: { // RES 1, L
    L &= 0xFD;
    return 8;
  }
  case 0x8E: { // RES 1, [HL]
    uint16_t HL = GetHL();
    uint8_t byte = bus.Read(HL);
    byte &= 0xFD;
    bus.Write(HL, byte);
    return 16;
  }
  case 0x8F: { // RES 1, A
    A &= 0xFD;
    return 8;
  }
  case 0x90: { // RES 2, B
    B &= 0xFB;
    return 8;
  }
  case 0x91: { // RES 2, C
    C &= 0xFB;
    return 8;
  }
  case 0x92: { // RES 2, D
    D &= 0xFB;
    return 8;
  }
  case 0x93: { // RES 2, E
    E &= 0xFB;
    return 8;
  }
  case 0x94: { // RES 2, H
    H &= 0xFB;
    return 8;
  }
  case 0x95: { // RES 2, L
    L &= 0xFB;
    return 8;
  }
  case 0x96: { // RES 2, [HL]
    uint16_t HL = GetHL();
    uint8_t byte = bus.Read(HL);
    byte &= 0xFB;
    bus.Write(HL, byte);
    return 16;
  }
  case 0x97: { // RES 2, A
    A &= 0xFB;
    return 8;
  }
  case 0x98: { // RES 3, B
    B &= 0xF7;
    return 8;
  }
  case 0x99: { // RES 3, C
    C &= 0xF7;
    return 8;
  }
  case 0x9A: { // RES 3, D
    D &= 0xF7;
    return 8;
  }
  case 0x9B: { // RES 3, E
    E &= 0xF7;
    return 8;
  }
  case 0x9C: { // RES 3, H
    H &= 0xF7;
    return 8;
  }
  case 0x9D: { // RES 3, L
    L &= 0xF7;
    return 8;
  }
  case 0x9E: { // RES 3, [HL]
    uint16_t HL = GetHL();
    uint8_t byte = bus.Read(HL);
    byte &= 0xF7;
    bus.Write(HL, byte);
    return 16;
  }
  case 0x9F: { // RES 3, A
    A &= 0xF7;
    return 8;
  }
  case 0xA0: { // RES 4, B
    B &= 0xEF;
    return 8;
  }
  case 0xA1: { // RES 4, C
    C &= 0xEF;
    return 8;
  }
  case 0xA2: { // RES 4, D
    D &= 0xEF;
    return 8;
  }
  case 0xA3: { // RES 4, E
    E &= 0xEF;
    return 8;
  }
  case 0xA4: { // RES 4, H
    H &= 0xEF;
    return 8;
  }
  case 0xA5: { // RES 4, L
    L &= 0xEF;
    return 8;
  }
  case 0xA6: { // RES 4, [HL]
    uint16_t HL = GetHL();
    uint8_t byte = bus.Read(HL);
    byte &= 0xEF;
    bus.Write(HL, byte);
    return 16;
  }
  case 0xA7: { // RES 4, A
    A &= 0xEF;
    return 8;
  }
  case 0xA8: { // RES 5, B
    B &= 0xDF;
    return 8;
  }
  case 0xA9: { // RES 5, C
    C &= 0xDF;
    return 8;
  }
  case 0xAA: { // RES 5, D
    D &= 0xDF;
    return 8;
  }
  case 0xAB: { // RES 5, E
    E &= 0xDF;
    return 8;
  }
  case 0xAC: { // RES 5, H
    H &= 0xDF;
    return 8;
  }
  case 0xAD: { // RES 5, L
    L &= 0xDF;
    return 8;
  }
  case 0xAE: { // RES 5, [HL]
    uint16_t HL = GetHL();
    uint8_t byte = bus.Read(HL);
    byte &= 0xDF;
    bus.Write(HL, byte);
    return 16;
  }
  case 0xAF: { // RES 5, A
    A &= 0xDF;
    return 8;
  }
  case 0xB0: { // RES 6, B
    B &= 0xBF;
    return 8;
  }
  case 0xB1: { // RES 6, C
    C &= 0xBF;
    return 8;
  }
  case 0xB2: { // RES 6, D
    D &= 0xBF;
    return 8;
  }
  case 0xB3: { // RES 6, E
    E &= 0xBF;
    return 8;
  }
  case 0xB4: { // RES 6, H
    H &= 0xBF;
    return 8;
  }
  case 0xB5: { // RES 6, L
    L &= 0xBF;
    return 8;
  }
  case 0xB6: { // RES 6, [HL]
    uint16_t HL = GetHL();
    uint8_t byte = bus.Read(HL);
    byte &= 0xBF;
    bus.Write(HL, byte);
    return 16;
  }
  case 0xB7: { // RES 6, A
    A &= 0xBF;
    return 8;
  }
  case 0xB8: { // RES 7, B
    B &= 0x7F;
    return 8;
  }
  case 0xB9: { // RES 7, C
    C &= 0x7F;
    return 8;
  }
  case 0xBA: { // RES 7, D
    D &= 0x7F;
    return 8;
  }
  case 0xBB: { // RES 7, E
    E &= 0x7F;
    return 8;
  }
  case 0xBC: { // RES 7, H
    H &= 0x7F;
    return 8;
  }
  case 0xBD: { // RES 7, L
    L &= 0x7F;
    return 8;
  }
  case 0xBE: { // RES 7, [HL]
    uint16_t HL = GetHL();
    uint8_t byte = bus.Read(HL);
    byte &= 0x7F;
    bus.Write(HL, byte);
    return 16;
  }
  case 0xBF: { // RES 7, A
    A &= 0x7F;
    return 8;
  }
  case 0xC0: { // SET 0, B
    B |= 0x01;
    return 8;
  }
  case 0xC1: { // SET 0, C
    C |= 0x01;
    return 8;
  }
  case 0xC2: { // SET 0, D
    D |= 0x01;
    return 8;
  }
  case 0xC3: { // SET 0, E
    E |= 0x01;
    return 8;
  }
  case 0xC4: { // SET 0, H
    H |= 0x01;
    return 8;
  }
  case 0xC5: { // SET 0, L
    L |= 0x01;
    return 8;
  }
  case 0xC6: { // SET 0, [HL]
    uint16_t HL = GetHL();
    uint8_t byte = bus.Read(HL);
    byte |= 0x01;
    bus.Write(HL, byte);
    return 16;
  }
  case 0xC7: { // SET 0, A
    A |= 0x01;
    return 8;
  }
  case 0xC8: { // SET 1, B
    B |= 0x02;
    return 8;
  }
  case 0xC9: { // SET 1, C
    C |= 0x02;
    return 8;
  }
  case 0xCA: { // SET 1, D
    D |= 0x02;
    return 8;
  }
  case 0xCB: { // SET 1, E
    E |= 0x02;
    return 8;
  }
  case 0xCC: { // SET 1, H
    H |= 0x02;
    return 8;
  }
  case 0xCD: { // SET 1, L
    L |= 0x02;
    return 8;
  }
  case 0xCE: { // SET 1, [HL]
    uint16_t HL = GetHL();
    uint8_t byte = bus.Read(HL);
    byte |= 0x02;
    bus.Write(HL, byte);
    return 16;
  }
  case 0xCF: { // SET 1, A
    A |= 0x02;
    return 8;
  }
  case 0xD0: { // SET 2, B
    B |= 0x04;
    return 8;
  }
  case 0xD1: { // SET 2, C
    C |= 0x04;
    return 8;
  }
  case 0xD2: { // SET 2, D
    D |= 0x04;
    return 8;
  }
  case 0xD3: { // SET 2, E
    E |= 0x04;
    return 8;
  }
  case 0xD4: { // SET 2, H
    H |= 0x04;
    return 8;
  }
  case 0xD5: { // SET 2, L
    L |= 0x04;
    return 8;
  }
  case 0xD6: { // SET 2, [HL]
    uint16_t HL = GetHL();
    uint8_t byte = bus.Read(HL);
    byte |= 0x04;
    bus.Write(HL, byte);
    return 16;
  }
  case 0xD7: { // SET 2, A
    A |= 0x04;
    return 8;
  }
  case 0xD8: { // SET 3, B
    B |= 0x08;
    return 8;
  }
  case 0xD9: { // SET 3, C
    C |= 0x08;
    return 8;
  }
  case 0xDA: { // SET 3, D
    D |= 0x08;
    return 8;
  }
  case 0xDB: { // SET 3, E
    E |= 0x08;
    return 8;
  }
  case 0xDC: { // SET 3, H
    H |= 0x08;
    return 8;
  }
  case 0xDD: { // SET 3, L
    L |= 0x08;
    return 8;
  }
  case 0xDE: { // SET 3, [HL]
    uint16_t HL = GetHL();
    uint8_t byte = bus.Read(HL);
    byte |= 0x08;
    bus.Write(HL, byte);
    return 16;
  }
  case 0xDF: { // SET 3, A
    A |= 0x08;
    return 8;
  }
  case 0xE0: { // SET 4, B
    B |= 0x10;
    return 8;
  }
  case 0xE1: { // SET 4, C
    C |= 0x10;
    return 8;
  }
  case 0xE2: { // SET 4, D
    D |= 0x10;
    return 8;
  }
  case 0xE3: { // SET 4, E
    E |= 0x10;
    return 8;
  }
  case 0xE4: { // SET 4, H
    H |= 0x10;
    return 8;
  }
  case 0xE5: { // SET 4, L
    L |= 0x10;
    return 8;
  }
  case 0xE6: { // SET 4, [HL]
    uint16_t HL = GetHL();
    uint8_t byte = bus.Read(HL);
    byte |= 0x10;
    bus.Write(HL, byte);
    return 16;
  }
  case 0xE7: { // SET 4, A
    A |= 0x10;
    return 8;
  }
  case 0xE8: { // SET 5, B
    B |= 0x20;
    return 8;
  }
  case 0xE9: { // SET 5, C
    C |= 0x20;
    return 8;
  }
  case 0xEA: { // SET 5, D
    D |= 0x20;
    return 8;
  }
  case 0xEB: { // SET 5, E
    E |= 0x20;
    return 8;
  }
  case 0xEC: { // SET 5, H
    H |= 0x20;
    return 8;
  }
  case 0xED: { // SET 5, L
    L |= 0x20;
    return 8;
  }
  case 0xEE: { // SET 5, [HL]
    uint16_t HL = GetHL();
    uint8_t byte = bus.Read(HL);
    byte |= 0x20;
    bus.Write(HL, byte);
    return 16;
  }
  case 0xEF: { // SET 5, A
    A |= 0x20;
    return 8;
  }
  case 0xF0: { // SET 6, B
    B |= 0x40;
    return 8;
  }
  case 0xF1: { // SET 6, C
    C |= 0x40;
    return 8;
  }
  case 0xF2: { // SET 6, D
    D |= 0x40;
    return 8;
  }
  case 0xF3: { // SET 6, E
    E |= 0x40;
    return 8;
  }
  case 0xF4: { // SET 6, H
    H |= 0x40;
    return 8;
  }
  case 0xF5: { // SET 6, L
    L |= 0x40;
    return 8;
  }
  case 0xF6: { // SET 6, [HL]
    uint16_t HL = GetHL();
    uint8_t byte = bus.Read(HL);
    byte |= 0x40;
    bus.Write(HL, byte);
    return 16;
  }
  case 0xF7: { // SET 6, A
    A |= 0x40;
    return 8;
  }
  case 0xF8: { // SET 7, B
    B |= 0x80;
    return 8;
  }
  case 0xF9: { // SET 7, C
    C |= 0x80;
    return 8;
  }
  case 0xFA: { // SET 7, D
    D |= 0x80;
    return 8;
  }
  case 0xFB: { // SET 7, E
    E |= 0x80;
    return 8;
  }
  case 0xFC: { // SET 7, H
    H |= 0x80;
    return 8;
  }
  case 0xFD: { // SET 7, L
    L |= 0x80;
    return 8;
  }
  case 0xFE: { // SET 7, [HL]
    uint16_t HL = GetHL();
    uint8_t byte = bus.Read(HL);
    byte |= 0x80;
    bus.Write(HL, byte);
    return 16;
  }
  case 0xFF: { // SET 7, A
    A |= 0x80;
    return 8;
  }
  default:
    std::cout << std::format("Unimplemented CB 0x{:02X}", cb_opcode)
              << std::endl;
    exit(1);
  }
}
