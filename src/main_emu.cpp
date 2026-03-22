#include "bus.hpp"
#include "clock.hpp"
#include "cpu.hpp"
#include <fstream>
#include <iostream>

static Clock clk;
static Bus bus(clk);
static CPU cpu(bus);

std::vector<uint8_t> ReadROMFile(const std::string &path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);

  if (!file.is_open()) {
    throw std::runtime_error("Failed to open ROM: " + path);
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<uint8_t> buffer(size);
  if (file.read(reinterpret_cast<char *>(buffer.data()), size)) {
    return buffer;
  }

  return {};
}

void InitializeSystem(std::string boot_path, std::string rom_path) {
  try {
    std::vector<uint8_t> boot = ReadROMFile(boot_path);
    bus.LoadBoot(boot);
    std::vector<uint8_t> rom = ReadROMFile(rom_path);
    bus.LoadROM(rom);
  } catch (const std::exception &e) {
    std::cerr << "Initialization error: " << e.what() << std::endl;
  }
}

int main(int argc, char *argv[]) {
  // if (argc == 1)
  //   return -1;
  //
  // InitializeSystem(argv[1]);

  InitializeSystem("../roms/dmg_boot.bin", "../roms/cpu_instrs/cpu_instrs.gb");

  while (true) {
    uint8_t cycles = cpu.HandleInterrupts();

    if (cycles == 0) {
      if (cpu.GetHALT())
        cycles = 4;
      else
        cycles = cpu.Step();
    }

    if (clk.Update(cycles))
      bus.Write(0xFF0F, bus.Read(0xFF0F) | 0x04);
  }
}
