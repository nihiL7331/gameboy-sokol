#include "cpu.hpp"
#include <format>
#include <iostream>

void CPU::Step() {
  uint8_t opcode = bus.Read(PC++);

  switch (opcode) {

  default:
    std::cout << std::format("Unimplemented opcode {:02x} at PC={:04x}\n",
                             static_cast<unsigned int>(opcode), PC);
    exit(1);
  }
}
