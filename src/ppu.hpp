#ifndef PPU_HPP
#define PPU_HPP

#include <array>
#include <cstdint>

class PPU {
private:
  uint8_t LCDC; // LCD Control (0xFF40)
  uint8_t STAT; // LCD Status (0xFF41)
  uint8_t SCY;  // BG Viewport Y position (0xFF42)
  uint8_t SCX;  // BG Viewport X position (0xFF43)
  uint8_t LY;   // LCD Y coordinate (0xFF44)
  uint8_t LYC;  // LY compare (0xFF45)
  uint8_t BGP;  // BG palette data (0xFF47)
  uint8_t OBP0; // OBJ palette 0 data (0xFF48)
  uint8_t OBP1; // OBJ palette 1 data (0xFF49)
  uint8_t WY;   // Window Y position (0xFF4A)
  uint8_t WX;   // Window X position (0xFF4B)

  std::array<uint8_t, 0x2000> VRAM{}; // 0x8000 - 0x9FFF
  std::array<uint8_t, 0x00A0> OAM{};  // 0xFE00 - 0xFE9F

  uint16_t dots;
  bool stat_line;
  uint8_t bg_color_ids[160]{};
  uint8_t window_line;
  bool window_active;

public:
  PPU()
      : LCDC(0), STAT(0), SCY(0), SCX(0), LY(0), LYC(0), BGP(0),
        frame_rd(false), dots(0), stat_line(false), window_line(0),
        window_active(false) {};
  ~PPU() = default;

  std::array<uint32_t, 160 * 144> fb{};
  bool frame_rd;

  uint8_t ReadVRAM(uint16_t addr) { return VRAM[addr & 0x1FFF]; }
  uint8_t ReadOAM(uint16_t addr) { return OAM[addr & 0x00FF]; }

  void WriteVRAM(uint16_t addr, uint8_t data) { VRAM[addr & 0x1FFF] = data; }
  void WriteOAM(uint16_t addr, uint8_t data) { OAM[addr & 0x00FF] = data; }

  uint8_t GetLCDC() const { return LCDC; }
  uint8_t GetSTAT() const { return 0x80 | STAT; }
  uint8_t GetSCY() const { return SCY; }
  uint8_t GetSCX() const { return SCX; }
  uint8_t GetLY() const { return LY; }
  uint8_t GetLYC() const { return LYC; }
  uint8_t GetBGP() const { return BGP; }
  uint8_t GetOBP0() const { return OBP0; }
  uint8_t GetOBP1() const { return OBP1; }
  uint8_t GetWY() const { return WY; }
  uint8_t GetWX() const { return WX; }

  void SetLCDC(uint8_t data) { LCDC = data; }
  void SetSTAT(uint8_t data) { STAT = (data & 0x78) | (STAT & 0x07); }
  void SetSCY(uint8_t data) { SCY = data; }
  void SetSCX(uint8_t data) { SCX = data; }
  void SetLY(uint8_t data) {} // No-op (LY is Read-only)
  void SetLYC(uint8_t data) { LYC = data; }
  void SetBGP(uint8_t data) { BGP = data; }
  void SetOBP0(uint8_t data) { OBP0 = data; }
  void SetOBP1(uint8_t data) { OBP1 = data; }
  void SetWY(uint8_t data) { WY = data; }
  void SetWX(uint8_t data) { WX = data; }

  void DrawBackgroundLine();
  void DrawSprites();
  void DrawWindowLine();
  uint8_t Update(uint8_t cycles);
};

#endif
