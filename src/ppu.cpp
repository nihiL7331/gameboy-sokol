#include "ppu.hpp"
#include <vector>

void PPU::DrawBackgroundLine() {
  uint16_t map_base =
      (LCDC & 0x08) ? 0x1C00 : 0x1800; // Offset relative to VRAM start
  uint16_t tile_base = (LCDC & 0x10) ? 0x0000 : 0x0800;
  uint8_t y_pos = LY + SCY;
  uint16_t tile_row = (y_pos / 8) * 32;

  for (int x = 0; x < 160; x++) {
    uint8_t x_pos = x + SCX;
    uint16_t tile_col = x_pos / 8;
    uint16_t tile_addr = map_base + tile_row + tile_col;
    uint8_t tile_num = VRAM[tile_addr];
    uint16_t tile_loc = tile_base;

    if (LCDC & 0x10) {
      tile_loc += (tile_num * 16);
    } else {
      tile_loc = 0x1000 + (static_cast<int8_t>(tile_num) * 16);
    }

    uint8_t line = (y_pos % 8) * 2; // 2 bytes per line
    uint8_t data1 = VRAM[tile_loc + line];
    uint8_t data2 = VRAM[tile_loc + line + 1];

    int color_bit = 7 - (x_pos % 8);
    uint8_t color_id =
        ((data2 >> color_bit) & 1) << 1 | ((data1 >> color_bit) & 1);

    bg_color_ids[x] = color_id;

    uint8_t pal_color = (BGP >> (color_id * 2)) & 0x03;

    const uint32_t colors[4] = {
        0xFFFFFFFF, // White
        0xFFAAAAAA, // Light Gray
        0xFF555555, // Dark Gray
        0xFF000000  // Black
    };

    // Write the final pixel to your flat framebuffer array!
    fb[LY * 160 + x] = colors[pal_color];
  }
}

void PPU::DrawSprites() {
  if ((LCDC & 0x02) == 0)
    return;

  bool use_8x16 = (LCDC & 0x04) != 0;
  int obj_hei = use_8x16 ? 16 : 8;

  struct Sprite {
    int index;
    int x;
  };
  std::vector<Sprite> active_sprites;

  for (int i = 0; i < 40; ++i) {
    int y_pos = OAM[i * 4] - 16;
    if (LY >= y_pos && LY < y_pos + obj_hei)
      active_sprites.push_back({i, OAM[i * 4 + 1] - 8});

    if (active_sprites.size() >= 10)
      break;
  }

  // 1. SORT FRONT-TO-BACK (Highest priority first!)
  // Smaller X is higher priority. If X is equal, smaller index is higher
  // priority.
  std::sort(active_sprites.begin(), active_sprites.end(),
            [](const Sprite &a, const Sprite &b) {
              return a.x != b.x ? a.x < b.x : a.index < b.index;
            });

  // 2. Track which pixels have already been claimed by a sprite
  bool sprite_drawn[160] = {false};

  for (auto &sprite : active_sprites) {
    uint8_t index = sprite.index * 4;
    int y_pos = OAM[index] - 16;
    int x_pos = OAM[index + 1] - 8;
    uint8_t tile_num = OAM[index + 2];
    uint8_t attributes = OAM[index + 3];

    bool y_flip = (attributes & 0x40) != 0;
    bool x_flip = (attributes & 0x20) != 0;

    if (LY >= y_pos && LY < (y_pos + obj_hei)) {
      int line = LY - y_pos;
      if (y_flip) {
        line = (obj_hei - 1) - line;
      }

      if (use_8x16) {
        tile_num &= 0xFE;
      }

      uint16_t tile_addr = (tile_num * 16) + (line * 2);
      uint8_t data1 = VRAM[tile_addr];
      uint8_t data2 = VRAM[tile_addr + 1];
      uint8_t palette = (attributes & 0x10) ? OBP1 : OBP0;

      for (int x = 0; x < 8; x++) {
        int pixel_x = x_pos + x;

        if (pixel_x < 0 || pixel_x >= 160)
          continue;

        // 3. If a higher priority sprite already claimed this pixel, skip!
        if (sprite_drawn[pixel_x])
          continue;

        int color_bit = x_flip ? x : (7 - x);
        uint8_t color_id =
            ((data2 >> color_bit) & 1) << 1 | ((data1 >> color_bit) & 1);

        if (color_id == 0)
          continue;

        // 5. Check if the winning sprite is hidden behind the background
        bool bg_prio = (attributes & 0x80) != 0x00;
        if (bg_prio && bg_color_ids[pixel_x] != 0x00)
          continue; // Sprite is hidden, but it still blocked lower-priority
                    // sprites!

        // 4. WE HAVE A WINNING SPRITE FOR THIS PIXEL! Claim it.
        sprite_drawn[pixel_x] = true;

        uint8_t palette_color = (palette >> (color_id * 2)) & 0x03;
        const uint32_t colors[4] = {
            0xFFFFFFFF, // White
            0xFFAAAAAA, // Light Gray
            0xFF555555, // Dark Gray
            0xFF000000  // Black
        };

        fb[LY * 160 + pixel_x] = colors[palette_color];
      }
    }
  }
}

void PPU::DrawWindowLine() {
  if ((LCDC & 0x20) == 0)
    return;
  if (!window_active)
    return;

  uint16_t window_map_base = (LCDC & 0x40) ? 0x1C00 : 0x1800;
  uint16_t tile_base = (LCDC & 0x10) ? 0x0000 : 0x0800;

  uint8_t window_y = window_line;
  uint16_t tile_row = (window_y / 8) * 32;
  int actual_wx = WX - 7;

  for (int x = 0; x < 160; x++) {
    if (x < actual_wx)
      continue;

    uint8_t window_x = x - (WX - 7);
    uint16_t tile_col = window_x / 8;
    uint16_t tile_address = window_map_base + tile_row + tile_col;
    uint8_t tile_num = VRAM[tile_address];
    uint16_t tile_loc = tile_base;

    if (LCDC & 0x10)
      tile_loc += (tile_num * 16);
    else
      tile_loc = 0x1000 + (static_cast<int8_t>(tile_num) * 16);

    uint8_t line = (window_y % 8) * 2;
    uint8_t data1 = VRAM[tile_loc + line];
    uint8_t data2 = VRAM[tile_loc + line + 1];
    int color_bit = 7 - (window_x % 8);
    uint8_t color_id =
        ((data2 >> color_bit) & 1) << 1 | ((data1 >> color_bit) & 1);
    bg_color_ids[x] = color_id;
    uint8_t palette_color = (BGP >> (color_id * 2)) & 0x03;
    uint32_t colors[4] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000};

    fb[LY * 160 + x] = colors[palette_color];
  }
}

uint8_t PPU::Update(uint8_t cycles) {
  if ((LCDC & 0x80) == 0) {
    dots = 0;
    LY = 0;
    STAT &= 0xF8;
    window_line = 0;
    window_active = false;
    return 0x00;
  }

  uint8_t interrupts = 0x00;
  uint8_t old_mode = STAT & 0x03;

  dots += cycles;

  if (dots >= 456) {
    dots -= 456;
    LY++;
    if (LY == 154) {
      LY = 0;
      window_line = 0;
      window_active = false;
    } else if (LY == 144) {
      interrupts |= 0x01;
      frame_rd = true;
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

  if (old_mode == 0x03 && mode == 0x00) {
    if (LY == WY)
      window_active = true;
    DrawBackgroundLine();
    DrawWindowLine();
    DrawSprites();
    if (window_active && (LCDC & 0x20) != 0 && WX <= 166)
      window_line++;
  }

  bool curr_line =
      ((STAT & 0x08) && (mode == 0x00)) || ((STAT & 0x10) && (mode == 0x01)) ||
      ((STAT & 0x20) && (mode == 0x02)) || ((STAT & 0x40) && (LY == LYC));

  bool trigger = !stat_line && curr_line;
  stat_line = curr_line;
  if (trigger)
    interrupts |= 0x02;

  return interrupts;
}
