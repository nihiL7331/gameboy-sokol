#include "bus.hpp"
#include "clock.hpp"
#include "cpu.hpp"
#include "ppu.hpp"

#include "sokol/sokol_app.h"
#include "sokol/sokol_audio.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"

#include <fstream>
#include <iostream>

static Clock clk;
static PPU ppu;
static Bus bus(clk, ppu);
static CPU cpu(bus);

const uint8_t SCR_WID = 160;
const uint8_t SCR_HEI = 144;

static sg_pass_action pass_action;
static sg_image screen_tex;
static sg_sampler screen_smp;
static sg_pipeline pip;
static sg_bindings bind;

void init_cb(void) {
  sg_desc desc = {};
  desc.environment = sglue_environment();
  sg_setup(&desc);

  pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
  pass_action.colors[0].clear_value = {0.1f, 0.1, 0.2f, 1.0f};

  sg_image_desc img_desc = {};
  img_desc.width = SCR_WID;
  img_desc.height = SCR_HEI;
  img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
  img_desc.usage.immutable = false;
  img_desc.usage.stream_update = true;
  screen_tex = sg_make_image(&img_desc);

  sg_sampler_desc smp_desc = {};
  smp_desc.min_filter = SG_FILTER_NEAREST;
  smp_desc.mag_filter = SG_FILTER_NEAREST;
  smp_desc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
  smp_desc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
  screen_smp = sg_make_sampler(&smp_desc);

  float vertices[] = {
      -1.0f, 1.0f,  0.0f, 0.0f, // LT
      1.0f,  1.0f,  1.0f, 0.0f, // RT
      1.0f,  -1.0f, 1.0f, 1.0f, // RB
      -1.0f, -1.0f, 0.0f, 1.0f, // LB
  };
  uint16_t indices[] = {0, 1, 2, 0, 2, 3};

  sg_buffer_desc vbuf_desc = {};
  vbuf_desc.data = sg_range(vertices, sizeof(vertices));
  bind.vertex_buffers[0] = sg_make_buffer(&vbuf_desc);

  sg_buffer_desc ibuf_desc = {};
  ibuf_desc.usage.index_buffer = true;
  ibuf_desc.data = sg_range(indices, sizeof(indices));
  bind.index_buffer = sg_make_buffer(&ibuf_desc);

  sg_shader_desc shd_desc = {};
  shd_desc.vertex_func.source = "#version 330\n"
                                "layout(location=0) in vec2 position;\n"
                                "layout(location=1) in vec2 texcoord0;\n"
                                "out vec2 uv;\n"
                                "void main() {\n"
                                "  gl_Position = vec4(position, 0.0, 1.0);\n"
                                "  uv = texcoord0;\n"
                                "}\n";
  shd_desc.fragment_func.source = "#version 330\n"
                                  "uniform sampler2D tex;\n"
                                  "in vec2 uv;\n"
                                  "out vec4 frag_color;\n"
                                  "void main() {\n"
                                  "  frag_color = texture(tex, uv);\n"
                                  "}\n";
  shd_desc.views[0].texture.stage = SG_SHADERSTAGE_FRAGMENT;
  shd_desc.samplers[0].stage = SG_SHADERSTAGE_FRAGMENT;
  shd_desc.texture_sampler_pairs[0].stage = SG_SHADERSTAGE_FRAGMENT;
  shd_desc.texture_sampler_pairs[0].glsl_name = "tex";
  shd_desc.texture_sampler_pairs[0].view_slot = 0;
  shd_desc.texture_sampler_pairs[0].sampler_slot = 0;

  sg_pipeline_desc pip_desc = {};
  pip_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2; // xy
  pip_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2; // uv
  pip_desc.shader = sg_make_shader(&shd_desc);
  pip_desc.index_type = SG_INDEXTYPE_UINT16;
  pip = sg_make_pipeline(&pip_desc);

  sg_view_desc view_desc = {};
  view_desc.texture.image = screen_tex;
  bind.views[0] = sg_make_view(&view_desc);
  bind.samplers[0] = screen_smp;
}

void frame_cb(void) {
  while (!ppu.frame_rd) {
    cpu.HandleInterrupts();

    if (cpu.GetHALT())
      bus.Tick(4);
    else
      cpu.Step();
  }

  ppu.frame_rd = false;

  sg_image_data data = {};
  data.mip_levels[0].ptr = ppu.fb.data();
  data.mip_levels[0].size = ppu.fb.size() * sizeof(uint32_t);
  sg_update_image(screen_tex, &data);

  sg_pass pass = {};
  pass.action = pass_action;
  pass.swapchain = sglue_swapchain();

  sg_begin_pass(&pass);
  sg_apply_pipeline(pip);
  sg_apply_bindings(&bind);
  sg_draw(0, 6, 1);
  sg_end_pass();
  sg_commit();
}

void cleanup_cb(void) { sg_shutdown(); }

void event_cb(const sapp_event *ev) {}

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

sapp_desc sokol_main(int argc, char *argv[]) {
  (void)argc;

  InitializeSystem("../roms/dmg_boot.bin", "../roms/oam_bug/oam_bug.gb");

  sapp_desc app = {};
  app.init_cb = init_cb;
  app.frame_cb = frame_cb;
  app.cleanup_cb = cleanup_cb;
  app.event_cb = event_cb;

  app.width = SCR_WID * 2;
  app.height = SCR_HEI * 2;
  app.sample_count = 1;
  app.high_dpi = true;
  app.window_title = "GB";

  return app;
}
