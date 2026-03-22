#ifndef PPU_HPP
#define PPU_HPP

#include <cstdint>

class PPU {
private:
  uint8_t LCDC; // LCD Control (0xFF40)
  uint8_t STAT; // LCD Status (0xFF41)
  uint8_t SCY;  // BG Viewport Y position (0xFF42)
  uint8_t SCX;  // BG Viewport X position (0xFF43)
  uint8_t LY;   // LCD Y coordinate (0xFF44)
  uint8_t LYC;  // LY compare (0xFF45)

  uint16_t dots;
  bool stat_line;

public:
  PPU()
      : LCDC(0), STAT(0), SCY(0), SCX(0), LY(0), LYC(0), dots(0),
        stat_line(false) {};
  ~PPU() = default;

  uint8_t GetLCDC() const { return LCDC; }
  uint8_t GetSTAT() const { return 0x80 | STAT; }
  uint8_t GetSCY() const { return SCY; }
  uint8_t GetSCX() const { return SCX; }
  uint8_t GetLY() const { return LY; }
  uint8_t GetLYC() const { return LYC; }

  void SetLCDC(uint8_t data) { LCDC = data; }
  void SetSTAT(uint8_t data) { STAT = 0x80 | (data & 0x78) | (STAT & 0x07); }
  void SetSCY(uint8_t data) { SCY = data; }
  void SetSCX(uint8_t data) { SCX = data; }
  void SetLY(uint8_t data) {} // No-op (LY is Read-only)
  void SetLYC(uint8_t data) { LYC = data; }

  uint8_t Update(uint8_t cycles);
};

#endif
