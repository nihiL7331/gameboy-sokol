#ifndef CPU_HPP
#define CPU_HPP

#include "bus.hpp"
#include <cstdint>

const uint8_t FLAG_Z = 0x80;
const uint8_t FLAG_N = 0x40;
const uint8_t FLAG_HCY = 0x20;
const uint8_t FLAG_CY = 0x10;

class CPU {
private:
  uint8_t A, B, C, D, E, H, L; // Registers
  uint16_t PC;                 // Program counter
  uint16_t SP;                 // Stack pointer
  uint8_t F;                   // Flags register
  // Flag layout
  // - Zero
  // - Subtraction
  // - Half carry
  // - Carry
  // - Rest is always 0
  bool IME;          // Interrupt Master Enable
  bool enabling_IME; // Helper flag to handle EI
  bool HALT;         // true if CPU is halted (0x76)
  bool HALT_BUG;     // Flag for handling the HALT bug

  Bus &bus;

  void ExecuteCB();

public:
  CPU(class Bus &b)
      : A(0x00), B(0x00), C(0x00), D(0x00), E(0x00), H(0x00), L(0x00),
        PC(0x0000), SP(0xFFFE), F(0x00), bus(b) {};
  ~CPU() = default;

  void HandleInterrupts();
  void Step();

  void SetFlag(const uint8_t mask, bool cond) {
    uint8_t bs = -static_cast<uint8_t>(cond);
    F = (F & ~mask) | (bs & mask);
  }
  bool GetFlag(const uint8_t mask) const { return (F & mask) != 0; }

  uint16_t GetAF() const { return (A << 8) | F; }
  uint16_t GetBC() const { return (B << 8) | C; }
  uint16_t GetDE() const { return (D << 8) | E; }
  uint16_t GetHL() const { return (H << 8) | L; }
  uint16_t GetSP() const { return SP; }
  uint16_t GetPC() const { return PC; }

  bool GetHALT() const { return HALT; }
  void SetHALT(bool set) { HALT = set; }

  void SetAF(uint16_t val) {
    A = (val >> 8) & 0xFF;
    F = val & 0xF0;
  }
  void SetBC(uint16_t val) {
    B = (val >> 8) & 0xFF;
    C = val & 0xFF;
  }
  void SetDE(uint16_t val) {
    D = (val >> 8) & 0xFF;
    E = val & 0xFF;
  }
  void SetHL(uint16_t val) {
    H = (val >> 8) & 0xFF;
    L = val & 0xFF;
  }
  void SetSP(uint16_t val) { SP = val; }
  void SetPC(uint16_t val) { PC = val; }
};

#endif
