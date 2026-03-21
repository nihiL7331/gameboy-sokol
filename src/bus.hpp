#ifndef BUS_HPP
#define BUS_HPP

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

class Bus {
private:
  std::vector<uint8_t> ROM;          // usually 32KB
  std::array<uint8_t, 0x2000> VRAM;  // 0x8000 - 0x9FFF
  std::array<uint8_t, 0x2000> EXRAM; // 0xA000 - 0xBFFF
  std::array<uint8_t, 0x2000> WRAM;  // 0xC000 - 0xDFFF
  // 0xE000-0xFDFF is mirror of 0xC000-0xDDzFF
  std::array<uint8_t, 0x00A0> OAM; // 0xFE00 - 0xFE9F
  // 0xFEA0-FEFF is not usable
  std::array<uint8_t, 0x0080> IO;   // 0xFF00 - 0xFF7F
  std::array<uint8_t, 0x007F> HRAM; // 0xFF80 - 0xFFFE
  uint8_t IE;                       // 0xFFFF - 0xFFFF

  std::array<uint8_t, 0x100> BOOT; // technically not separate

public:
  Bus() = default;

  bool is_boot = true;
  void LoadBoot(const std::vector<uint8_t> &boot_data) {
    std::copy(boot_data.begin(), boot_data.begin() + 0x100, BOOT.begin());
  }

  void LoadROM(std::vector<uint8_t> rom_data) { ROM = std::move(rom_data); }

  uint8_t Read(uint16_t addr) const {
    // HACK:
    if (addr == 0xFF44) {
      static uint8_t fake_ly = 0;
      fake_ly = (fake_ly + 1) % 154;
      return fake_ly;
    }
    if (addr == 0xFF04) {
      static uint8_t fake_div = 0;
      return fake_div++;
    }
    if (is_boot && addr < 0x0100)
      return BOOT[addr];
    switch (addr >> 12) {
    case 0x0:
    case 0x1:
    case 0x2:
    case 0x3:
    case 0x4:
    case 0x5:
    case 0x6:
    case 0x7: { // ROM
      return ROM[addr];
      break;
    }
    case 0x8:
    case 0x9: { // VRAM
      return VRAM[addr & 0x1FFF];
      break;
    }
    case 0xA:
    case 0xB: { // EXRAM
      return EXRAM[addr & 0x1FFF];
      break;
    }
    case 0xC:
    case 0xD: { // WRAM
      return WRAM[addr & 0x1FFF];
      break;
    }
    case 0xE:
    case 0xF: {
      if (addr >= 0xE000 && addr <= 0xFDFF) { // Mirror RAM
        return WRAM[addr & 0x1FFF];
      } else if (addr >= 0xFE00 && addr <= 0xFE9F) { // OAM
        return OAM[addr & 0x00FF];
      } else if (addr >= 0xFEA0 && addr <= 0xFEFF) { // Dead zone
        return 0xFF;
      } else if (addr >= 0xFF00 && addr <= 0xFF7F) { // IO
        return IO[addr & 0x00FF];
      } else if (addr >= 0xFF80 && addr <= 0xFFFE) { // HRAM
        return HRAM[addr & 0x007F];
      } else if (addr == 0xFFFF) { // IE
        return IE;
      }
      break;
    }
    }
    return 0xFF;
  }

  void Write(uint16_t addr, uint8_t data) {
    // HACK: intercept serial port output for tests
    if (addr == 0xFF02 && data == 0x81) {
      std::cout << (char)Read(0xFF01);
      std::cout.flush();
      return;
    }

    if (addr == 0xFF02) {
      IO[0x02] = 0;
      return;
    }

    switch (addr >> 12) {
    case 0x0:
    case 0x1:
    case 0x2:
    case 0x3:
    case 0x4:
    case 0x5:
    case 0x6:
    case 0x7: { // ROM
      break;
    }
    case 0x8:
    case 0x9: { // VRAM
      VRAM[addr & 0x1FFF] = data;
      break;
    }
    case 0xA:
    case 0xB: { // EXRAM
      EXRAM[addr & 0x1FFF] = data;
      break;
    }
    case 0xC:
    case 0xD: { // WRAM
      WRAM[addr & 0x1FFF] = data;
      break;
    }
    case 0xE:
    case 0xF: {
      if (addr >= 0xE000 && addr <= 0xFDFF) { // Mirror RAM
        WRAM[addr & 0x1FFF] = data;
      } else if (addr >= 0xFE00 && addr <= 0xFE9F) { // OAM
        OAM[addr & 0x00FF] = data;
      } else if (addr >= 0xFF00 && addr <= 0xFF7F) { // IO
        if (addr == 0xFF50 & data != 0)
          is_boot = false;
        IO[addr & 0x00FF] = data;
      } else if (addr >= 0xFF80 && addr <= 0xFFFE) { // HRAM
        HRAM[addr & 0x007F] = data;
      } else if (addr == 0xFFFF) { // IE
        IE = data;
      }
      break;
    }
    }
  }
};

#endif
