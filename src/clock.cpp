#include "clock.hpp"

bool Clock::Update(uint8_t cycles) {
  bool ret = false;

  while (cycles > 0) {
    if (TIMA_OF_delay) {
      TIMA_OF_delay = false;
      TIMA = TMA;
      ret = true;
    }

    uint8_t enable = (TAC >> 2) & 0x01;
    uint8_t shift;
    uint8_t lo = TAC & 0x03;
    if (lo == 0x00)
      shift = 9U;
    else if (lo == 0x01)
      shift = 3U;
    else if (lo == 0x02)
      shift = 5U;
    else if (lo == 0x03)
      shift = 7U;

    uint8_t guard = ((SYSCLK >> shift) & 0x01) & enable;

    SYSCLK += 4;
    cycles -= 4;

    uint8_t curr = ((SYSCLK >> shift) & 0x01) & enable;

    if (guard && !curr) {
      TIMA++;

      if (TIMA == 0U)
        TIMA_OF_delay = true;
    }
  }

  return ret;
}

void Clock::ResetSYSCLK() {
  uint8_t enable = (TAC >> 2) & 0x01;
  uint8_t shift;
  uint8_t lo = TAC & 0x03;
  if (lo == 0x00)
    shift = 9U;
  else if (lo == 0x01)
    shift = 3U;
  else if (lo == 0x02)
    shift = 5U;
  else if (lo == 0x03)
    shift = 7U;

  uint8_t bit_high = ((SYSCLK >> shift) & 0x01) & enable;

  SYSCLK = 0;

  if (bit_high) {
    TIMA++;
    if (TIMA == 0U) {
      TIMA_OF_delay = true;
    }
  }
}
