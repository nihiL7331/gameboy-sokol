#ifndef CLOCK_HPP
#define CLOCK_HPP

#include <cstdint>

class Clock {
private:
  uint16_t SYSCLK; // Oldest 8 bits are DIV (0xFF04)
  uint8_t TIMA;    // Timer Counter (0xFF05)
  uint8_t TMA;     // Timer Modulo (0xFF06)
  uint8_t TAC;     // Timer Control (0xFF07)

  bool TIMA_OF_delay;

public:
  Clock() : SYSCLK(0), TIMA(0), TMA(0), TAC(0), TIMA_OF_delay(false) {};
  ~Clock() = default;

  bool Update(uint8_t cycles);

  uint8_t GetDIV() const { return SYSCLK >> 8; }
  uint8_t GetTIMA() const { return TIMA; }
  uint8_t GetTMA() const { return TMA; }
  uint8_t GetTAC() const { return 0xF8 | TAC; }

  void ResetSYSCLK();
  void SetTIMA(uint8_t data) { TIMA = data; }
  void SetTMA(uint8_t data) { TMA = data; }
  void SetTAC(uint8_t data) { TAC = data; }
};

#endif
