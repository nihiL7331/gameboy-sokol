#include "ppu.hpp"

uint8_t PPU::Update(uint8_t cycles) {
  if ((LCDC & 0x80) == 0) {
    dots = 0;
    LY = 0;
    STAT &= 0xF8;
    return 0x00;
  }

  uint8_t interrupts = 0x00;
  dots += cycles;

  if (dots >= 456) {
    dots -= 456;
    LY++;
    if (LY == 154)
      LY = 0;
    else if (LY == 144) {
      interrupts |= 0x01;
    }
  }

  STAT &= 0xF8;
  if (LY >= 144) {
    STAT |= 0x01;
  } else {
    if (dots < 80)
      STAT |= 0x02;
    else if (dots < 252)
      STAT |= 0x03;
  }

  if (LY == LYC)
    STAT |= 0x04;

  uint8_t mode = STAT & 0x03;
  bool curr_line =
      ((STAT & 0x08) && (mode == 0x00)) || ((STAT & 0x10) && (mode == 0x01)) ||
      ((STAT & 0x20) && (mode == 0x02)) || ((STAT & 0x40) && (LY == LYC));

  bool trigger = !stat_line && curr_line;
  stat_line = curr_line;
  if (trigger)
    interrupts |= 0x02;

  return interrupts;
}
