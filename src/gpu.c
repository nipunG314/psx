#include <stdlib.h>
#include <string.h>

#include "gpu.h"
#include "log.h"
#include "output_logger.h"

GpuCommandBuffer init_command_buffer() {
  GpuCommandBuffer buffer;

  memset(buffer.commands, 0, sizeof buffer.commands);
  buffer.command_count = 0;

  return buffer;
}

GpuRenderer init_renderer() {
  GpuRenderer renderer;

  renderer.window = SDL_CreateWindow("PSX", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 512, 0);
  renderer.window_surface = SDL_GetWindowSurface(renderer.window);
  if (renderer.window_surface == NULL)
    fatal("SDLError: Couldn't initialize window_surface: %s", SDL_GetError());
  renderer.vram_surface = SDL_CreateRGBSurfaceWithFormat(0, 1024, 512, 16, SDL_PIXELFORMAT_RGB555);
  if (renderer.vram_surface == NULL)
    fatal("SDLError: Couldn't initialize vram_surface: %s", SDL_GetError());

  renderer.render_mode = GpuRenderTri;

  return renderer;
}

void renderer_update_window(GpuRenderer *renderer) {
  SDL_BlitSurface(renderer->vram_surface, 0, renderer->window_surface, 0);
  SDL_UpdateWindowSurface(renderer->window);
}

void destroy_renderer(GpuRenderer *renderer) {
  SDL_DestroyWindow(renderer->window);
}

void gp0_nop(Gpu *gpu, uint32_t val);

Gpu init_gpu() {
  Gpu gpu;

  gpu.texture_page[0] = gpu.texture_page[1] = 0;
  gpu.clut[0] = gpu.clut[1] = 0;
  gpu.semi_transparent = false;
  gpu.semi_transparency_mode = GpuTransparencyMean;
  gpu.texture_depth = GpuTexture4Bits;
  gpu.blend_mode = GpuNoTexture;
  gpu.dithering = false;
  gpu.draw_to_display = false;
  gpu.force_set_mask_bit = false;
  gpu.preserve_masked_pixels = false;
  gpu.interlace_field = GpuInterlaceTop;
  gpu.texture_disabled = false;
  gpu.hres = MAKE_GpuHRes(0x0);
  gpu.vres = GpuVertical240Lines;
  gpu.video_mode = GpuNTSC;
  gpu.display_depth = GpuDisplayDepth15Bits;
  gpu.interlaced = false;
  gpu.display_disabled = true;
  gpu.interrupt_active = false;
  gpu.dma_direction = GpuDmaDirOff;
  gpu.rectangle_texture_x_flip = false;
  gpu.rectangle_texture_y_flip = false;
  gpu.texture_window_x_mask = 0;
  gpu.texture_window_y_mask = 0;
  gpu.texture_window_x_offset = 0;
  gpu.texture_window_y_offset = 0;
  gpu.drawing_area_left = 0;
  gpu.drawing_area_top = 0;
  gpu.drawing_area_right = 0;
  gpu.drawing_area_bottom = 0;
  gpu.drawing_x_offset = 0;
  gpu.drawing_y_offset = 0;
  gpu.display_vram_x_start = 0;
  gpu.display_vram_y_start = 0;
  gpu.display_hor_start = 0;
  gpu.display_hor_end = 0;
  gpu.display_line_start = 0;
  gpu.display_line_end = 0;
  gpu.gp0_command_buffer = init_command_buffer();
  gpu.gp0_words_remaining = 0;
  gpu.gp0_method = gp0_nop;
  gpu.gp0_mode = Gp0CommandMode;
  gpu.image_load_buffer.left = 0;
  gpu.image_load_buffer.top = 0;
  gpu.image_load_buffer.width = 0;
  gpu.image_load_buffer.height = 0;
  gpu.image_load_buffer.x = 0;
  gpu.image_load_buffer.y = 0;
  gpu.renderer = init_renderer();
  gpu.read_word = 0;
  gpu.output_log_index = init_output_log();

  return gpu;
}

uint32_t gpu_status(Gpu *gpu) {
  uint32_t status = 0;

  status |= gpu->texture_page[0] >> 6;
  status |= (gpu->texture_page[1] >> 8) << 4;
  status |= ((uint32_t)gpu->semi_transparency_mode) << 5;
  status |= ((uint32_t)gpu->texture_depth) << 7;
  status |= ((uint32_t)gpu->dithering) << 9;
  status |= ((uint32_t)gpu->draw_to_display) << 10;
  status |= ((uint32_t)gpu->force_set_mask_bit) << 11;
  status |= ((uint32_t)gpu->preserve_masked_pixels) << 12;
  status |= ((uint32_t)gpu->interlace_field) << 13;
  status |= ((uint32_t)gpu->texture_disabled) << 15;
  status |= hres_status(gpu->hres);
  // TempFix: Setting bit 19 deadlocks the BIOS
  // without proper GPU Timings. For now, don't set bit
  // 19.
  //status |= ((uint32_t)gpu->vres) << 19;
  status |= ((uint32_t)gpu->video_mode) << 20;
  status |= ((uint32_t)gpu->display_depth) << 21;
  status |= ((uint32_t)gpu->interlaced) << 22;
  status |= ((uint32_t)gpu->display_disabled) << 23;
  status |= ((uint32_t)gpu->interrupt_active) << 24;
  // TempFix: Unconditionally set the Ready bits
  status |= 1 << 26;
  status |= 1 << 27;
  status |= 1 << 28;
  status |= ((uint32_t)gpu->dma_direction) << 29;
  status |= 0 << 31;

  switch (gpu->dma_direction) {
    case GpuDmaDirOff:
      status |= 0 << 25;
      break;
    case GpuDmaDirFifo:
      // TempFix: Should set 0 << 25 if FIFO is full. Unconditionally set to 1 for now.
      status |= 1 << 25;
      break;
    case GpuDmaDirCpuToGp0:
      status |= ((status >> 28) & 1) << 25;
      break;
    case GpuDmaDirOffVRamToCpu:
      status |= ((status >> 27) & 1) << 25;
      break;
  }

  return status;
}

uint32_t gpu_read(Gpu *gpu) {
  return gpu->read_word;
}

void gp0_nop(Gpu *gpu, uint32_t val) {
}

void gp0_clear_cache(Gpu *gpu, uint32_t val) {
}

void gp0_draw_mode(Gpu *gpu, uint32_t val) {
  gpu->texture_page[0] = (val & 0xF) << 6;
  gpu->texture_page[1] = ((val >> 4) & 1) << 8;
  switch ((val >> 5) & 3) {
    case 0:
      gpu->semi_transparency_mode = GpuTransparencyMean;
      break;
    case 1:
      gpu->semi_transparency_mode = GpuTransparencySum;
      break;
    case 2:
      gpu->semi_transparency_mode = GpuTransparencyDiff;
      break;
    case 3:
      gpu->semi_transparency_mode = GpuTransparencySumQuarter;
      break;
  }
  switch ((val >> 7) & 3) {
    case 0:
      gpu->texture_depth = GpuTexture4Bits;
      break;
    case 1:
      gpu->texture_depth = GpuTexture8Bits;
      break;
    case 2:
      gpu->texture_depth = GpuTexture15Bits;
      break;
    default:
      fatal("Unhandled Texture Depth: %x", (val >> 7) & 3)
  }
  gpu->dithering = (val >> 9) & 1;
  gpu->draw_to_display = (val >> 10) & 1;
  gpu->texture_disabled = (val >> 11) & 1;
  gpu->rectangle_texture_x_flip = (val >> 12) & 1;
  gpu->rectangle_texture_y_flip = (val >> 13) & 1;
}

void gp0_set_drawing_top_left(Gpu *gpu, uint32_t val) {
  gpu->drawing_area_top = (val >> 10) & 0x3FF;
  gpu->drawing_area_left = val & 0x3FF;
}

void gp0_set_drawing_bottom_right(Gpu *gpu, uint32_t val) {
  gpu->drawing_area_bottom = (val >> 10) & 0x3FF;
  gpu->drawing_area_right = val & 0x3FF;
}

void gp0_set_drawing_offsets(Gpu *gpu, uint32_t val) {
  uint16_t x = val & 0x7FF;
  uint16_t y = (val >> 11) & 0x7FF;

  gpu->drawing_x_offset = x << 5;
  gpu->drawing_y_offset = y << 5;
  gpu->drawing_x_offset = gpu->drawing_x_offset >> 5;
  gpu->drawing_y_offset = gpu->drawing_y_offset >> 5;

  renderer_update_window(&gpu->renderer);
}

void gp0_texture_window(Gpu *gpu, uint32_t val) {
  gpu->texture_window_x_mask = val & 0x1F;
  gpu->texture_window_y_mask = (val >> 5) & 0x1F;
  gpu->texture_window_x_offset = (val >> 10) & 0x1F;
  gpu->texture_window_y_offset = (val >> 15) & 0x1F;
}

void gp0_mask_bit_setting(Gpu *gpu, uint32_t val) {
  gpu->force_set_mask_bit = val & 1;
  gpu->preserve_masked_pixels = val & 2;
}

void gp0_monochrome_quad(Gpu *gpu, uint32_t val) {
  GpuCommandBuffer *command_buffer = &gpu->gp0_command_buffer;
  GpuRenderer *renderer = &gpu->renderer;
  renderer->render_mode = GpuRenderTri;

  pos_from_gp0(command_buffer->commands[1], renderer->tri_pos[0]);
  pos_from_gp0(command_buffer->commands[2], renderer->tri_pos[1]);
  pos_from_gp0(command_buffer->commands[3], renderer->tri_pos[2]);

  color_from_gp0(command_buffer->commands[0], renderer->tri_color[0]);
  color_from_gp0(command_buffer->commands[0], renderer->tri_color[1]);
  color_from_gp0(command_buffer->commands[0], renderer->tri_color[2]);

  gpu_draw(gpu);

  pos_from_gp0(command_buffer->commands[2], renderer->tri_pos[0]);
  pos_from_gp0(command_buffer->commands[3], renderer->tri_pos[1]);
  pos_from_gp0(command_buffer->commands[4], renderer->tri_pos[2]);

  color_from_gp0(command_buffer->commands[0], renderer->tri_color[0]);
  color_from_gp0(command_buffer->commands[0], renderer->tri_color[1]);
  color_from_gp0(command_buffer->commands[0], renderer->tri_color[2]);

  gpu_draw(gpu);
}

void gp0_texture_quad(Gpu *gpu, uint32_t val) {
  GpuCommandBuffer *command_buffer = &gpu->gp0_command_buffer;
  GpuRenderer *renderer = &gpu->renderer;
  renderer->render_mode = GpuRenderTri;

  set_clut(gpu, command_buffer->commands[2] >> 16);
  set_texture_params(gpu, command_buffer->commands[4] >> 16);

  pos_from_gp0(command_buffer->commands[1], renderer->tri_pos[0]);
  pos_from_gp0(command_buffer->commands[3], renderer->tri_pos[1]);
  pos_from_gp0(command_buffer->commands[5], renderer->tri_pos[2]);

  color_from_gp0(command_buffer->commands[0], renderer->tri_color[0]);
  color_from_gp0(command_buffer->commands[0], renderer->tri_color[1]);
  color_from_gp0(command_buffer->commands[0], renderer->tri_color[2]);

  tex_from_gp0(command_buffer->commands[2], renderer->tri_tex[0]);
  tex_from_gp0(command_buffer->commands[4], renderer->tri_tex[1]);
  tex_from_gp0(command_buffer->commands[6], renderer->tri_tex[2]);

  gpu_draw(gpu);

  pos_from_gp0(command_buffer->commands[3], renderer->tri_pos[0]);
  pos_from_gp0(command_buffer->commands[5], renderer->tri_pos[1]);
  pos_from_gp0(command_buffer->commands[7], renderer->tri_pos[2]);

  tex_from_gp0(command_buffer->commands[4], renderer->tri_tex[0]);
  tex_from_gp0(command_buffer->commands[6], renderer->tri_tex[1]);
  tex_from_gp0(command_buffer->commands[8], renderer->tri_tex[2]);

  gpu_draw(gpu);
}

void gp0_shaded_tri(Gpu *gpu, uint32_t val) {
  GpuCommandBuffer *command_buffer = &gpu->gp0_command_buffer;
  GpuRenderer *renderer = &gpu->renderer;
  renderer->render_mode = GpuRenderTri;

  pos_from_gp0(command_buffer->commands[1], renderer->tri_pos[0]);
  pos_from_gp0(command_buffer->commands[3], renderer->tri_pos[1]);
  pos_from_gp0(command_buffer->commands[5], renderer->tri_pos[2]);

  color_from_gp0(command_buffer->commands[0], renderer->tri_color[0]);
  color_from_gp0(command_buffer->commands[2], renderer->tri_color[1]);
  color_from_gp0(command_buffer->commands[4], renderer->tri_color[2]);

  gpu_draw(gpu);
}

void gp0_shaded_quad(Gpu *gpu, uint32_t val) {
  GpuCommandBuffer *command_buffer = &gpu->gp0_command_buffer;
  GpuRenderer *renderer = &gpu->renderer;
  renderer->render_mode = GpuRenderTri;

  pos_from_gp0(command_buffer->commands[1], renderer->tri_pos[0]);
  pos_from_gp0(command_buffer->commands[3], renderer->tri_pos[1]);
  pos_from_gp0(command_buffer->commands[5], renderer->tri_pos[2]);

  color_from_gp0(command_buffer->commands[0], renderer->tri_color[0]);
  color_from_gp0(command_buffer->commands[2], renderer->tri_color[1]);
  color_from_gp0(command_buffer->commands[4], renderer->tri_color[2]);

  gpu_draw(gpu);

  pos_from_gp0(command_buffer->commands[3], renderer->tri_pos[0]);
  pos_from_gp0(command_buffer->commands[5], renderer->tri_pos[1]);
  pos_from_gp0(command_buffer->commands[7], renderer->tri_pos[2]);

  color_from_gp0(command_buffer->commands[2], renderer->tri_color[0]);
  color_from_gp0(command_buffer->commands[4], renderer->tri_color[1]);
  color_from_gp0(command_buffer->commands[6], renderer->tri_color[2]);

  gpu_draw(gpu);
}

void gp0_monochrome_rect(Gpu *gpu, uint32_t val) {
  log_trace("STUB: Draw Monochrome Rect!");
}

void gp0_monochrome_rect_1x1(Gpu *gpu, uint32_t val) {
  GpuCommandBuffer *command_buffer = &gpu->gp0_command_buffer;
  GpuRenderer *renderer = &gpu->renderer;
  renderer->render_mode = GpuRenderRect;

  pos_from_gp0(command_buffer->commands[1], renderer->tri_pos[0]);
  color_from_gp0(command_buffer->commands[0], renderer->tri_color[0]);

  uint16_t *target = get_vram(renderer, renderer->tri_pos[0][0], renderer->tri_pos[0][1]);
  *target = vec_to_555(renderer->tri_color[0]);
}

void gp0_image_load(Gpu *gpu, uint32_t val) {
  uint32_t width = gpu->gp0_command_buffer.commands[2] & 0xFFFF;
  uint32_t height = gpu->gp0_command_buffer.commands[2] >> 16;

  uint32_t size = width * height;
  size = (size + 1) & ~1;

  if (size > 0) {
    gpu->image_load_buffer.left = gpu->gp0_command_buffer.commands[1];
    gpu->image_load_buffer.top = gpu->gp0_command_buffer.commands[1] >> 16;
    gpu->image_load_buffer.width = width;
    gpu->image_load_buffer.height = height;
    gpu->gp0_words_remaining = size / 2;
    gpu->image_load_buffer.x = gpu->image_load_buffer.left;
    gpu->image_load_buffer.y = gpu->image_load_buffer.top;
  } else {
    log_error("GPU: 0-Sized Image Load");
  }

  gpu->gp0_mode = Gp0ImageLoadMode;
}

void gp0_image_store(Gpu *gpu, uint32_t val) {
  uint32_t width = gpu->gp0_command_buffer.commands[2] & 0xFFFF;
  uint32_t height = gpu->gp0_command_buffer.commands[2] >> 16;

  log_error("Unhandled GP0_IMAGE_STORE. Width: %x, Height: %x", width, height);
}

void gpu_gp0(Gpu *gpu, uint32_t val) {
  if (gpu->gp0_words_remaining == 0) {
    uint8_t opcode = (val >> 24) & 0xFF;

    LOG_OUTPUT(gpu->output_log_index, "GP0 Command: %08x", val);

    switch (opcode) {
      case 0x00:
        gpu->gp0_method = gp0_nop;
        gpu->gp0_words_remaining = 1;
        break;
      case 0x01:
        gpu->gp0_method = gp0_clear_cache;
        gpu->gp0_words_remaining = 1;
        break;
      case 0x28:
        gpu->gp0_method = gp0_monochrome_quad;
        gpu->gp0_words_remaining = 5;
        break;
      case 0x2C:
        gpu->gp0_method = gp0_texture_quad;
        gpu->gp0_words_remaining = 9;
        break;
      case 0x30:
        gpu->gp0_method = gp0_shaded_tri;
        gpu->gp0_words_remaining = 6;
        break;
      case 0x38:
        gpu->gp0_method = gp0_shaded_quad;
        gpu->gp0_words_remaining = 8;
        break;
      case 0x60:
        gpu->gp0_method = gp0_monochrome_rect;
        gpu->gp0_words_remaining = 3;
        break;
      case 0x68:
        gpu->gp0_method = gp0_monochrome_rect_1x1;
        gpu->gp0_words_remaining = 2;
      case 0xA0:
        gpu->gp0_method = gp0_image_load;
        gpu->gp0_words_remaining = 3;
        break;
      case 0xC0:
        gpu->gp0_method = gp0_image_store;
        gpu->gp0_words_remaining = 3;
      case 0xE1:
        gpu->gp0_method = gp0_draw_mode;
        gpu->gp0_words_remaining = 1;
        break;
      case 0xE2:
        gpu->gp0_method = gp0_texture_window;
        gpu->gp0_words_remaining = 1;
        break;
      case 0xE3:
        gpu->gp0_method = gp0_set_drawing_top_left;
        gpu->gp0_words_remaining = 1;
        break;
      case 0xE4:
        gpu->gp0_method = gp0_set_drawing_bottom_right;
        gpu->gp0_words_remaining = 1;
        break;
      case 0xE5:
        gpu->gp0_method = gp0_set_drawing_offsets;
        gpu->gp0_words_remaining = 1;
        break;
      case 0xE6:
        gpu->gp0_method = gp0_mask_bit_setting;
        gpu->gp0_words_remaining = 1;
        break;
      default:
        fatal("Unhandled GP0 Command: 0x%08X", val);
    }

    command_buffer_clear(&gpu->gp0_command_buffer);

    bool textured = opcode & 0x4;
    if (textured) {
      gpu->blend_mode = (opcode & 0x1) ? GpuRawTexture : GpuBlendedTexture;
    } else
      gpu->blend_mode = GpuNoTexture;

    gpu->semi_transparent = opcode & 0x2;

    print_output_log(gpu->output_log_index);
  }

  gpu->gp0_words_remaining -= 1;

  switch (gpu->gp0_mode) {
    case Gp0CommandMode:
      push_command(&gpu->gp0_command_buffer, val);
      if (gpu->gp0_words_remaining == 0)
        gpu->gp0_method(gpu, val);
      break;
    case Gp0ImageLoadMode:
      push_image_word(&gpu->image_load_buffer, &gpu->renderer, val);
      if (gpu->gp0_words_remaining == 0) {
        gpu->gp0_mode = Gp0CommandMode;
      }
      break;
  }
}

void gp1_reset_command_buffer(Gpu *gpu, uint32_t val) {
  command_buffer_clear(&gpu->gp0_command_buffer);
  gpu->gp0_words_remaining = 0;
  gpu->gp0_mode = Gp0CommandMode;
  // ToDo: Clear FIFO once implemented
}

void gp1_reset(Gpu *gpu, uint32_t val) {
  gpu->interrupt_active = false;
  gpu->texture_page[0] = gpu->texture_page[1] = 0;
  gpu->semi_transparency_mode = GpuTransparencyMean;
  gpu->texture_depth = GpuTexture4Bits;
  gpu->texture_window_x_mask = gpu->texture_window_y_mask = 0;
  gpu->texture_window_x_offset = gpu->texture_window_y_offset = 0;
  gpu->dithering = false;
  gpu->draw_to_display = false;
  gpu->texture_disabled = false;
  gpu->rectangle_texture_x_flip = gpu->rectangle_texture_y_flip = false;
  gpu->drawing_area_top = gpu->drawing_area_bottom = 0;
  gpu->drawing_area_left = gpu->drawing_area_right = 0;
  gpu->drawing_x_offset = gpu->drawing_y_offset = 0;
  gpu->force_set_mask_bit = false;
  gpu->preserve_masked_pixels = false;
  gpu->dma_direction = GpuDmaDirOff;
  gpu->display_disabled = true;
  gpu->display_vram_x_start = gpu->display_vram_y_start = 0;
  gpu->hres = MAKE_GpuHRes(0);
  gpu->vres = GpuVertical240Lines;
  gpu->video_mode = GpuNTSC;
  gpu->interlaced = true;
  gpu->display_hor_start = 0x200;
  gpu->display_hor_end = 0xc00;
  gpu->display_line_start = 0x10;
  gpu->display_line_end = 0x100;
  gpu->display_depth = GpuDisplayDepth15Bits;
  gp1_reset_command_buffer(gpu, val);

  // ToDo: GPU Cache
}

void gp1_dma_direction(Gpu *gpu, uint32_t val) {
  switch (val & 3) {
    case 0:
      gpu->dma_direction = GpuDmaDirOff;
      break;
    case 1:
      gpu->dma_direction = GpuDmaDirFifo;
      break;
    case 2:
      gpu->dma_direction = GpuDmaDirCpuToGp0;
      break;
    case 3:
      gpu->dma_direction = GpuDmaDirOffVRamToCpu;
      break;
    default:
      fatal("Unknown GPU DMA Direction: %x", val & 3);
  }
}

void gp1_display_mode(Gpu *gpu, uint32_t val) {
  uint8_t hr1 = val & 3;
  uint8_t hr2 = (val >> 6) & 1;

  gpu->hres = hres_from_fields(hr1, hr2);
  gpu->vres = (val & 0x4) ? GpuVertical480Lines : GpuVertical240Lines;
  gpu->video_mode = (val & 0x8) ? GpuPAL : GpuNTSC;
  gpu->display_depth = (val & 0x10) ? GpuDisplayDepth15Bits : GpuDisplayDepth24Bits;
  gpu->interlaced = val & 0x20;

  if (val & 0x80)
    fatal("Unsupported Display Mode: 0x%08X", val);
}

void gp1_display_vram_start(Gpu *gpu, uint32_t val) {
  gpu->display_vram_x_start = val & 0x3FE;
  gpu->display_vram_y_start = (val >> 10) & 0x1FF;
}

void gp1_display_horizontal_range(Gpu *gpu, uint32_t val) {
  gpu->display_hor_start = val & 0xFFF;
  gpu->display_hor_end = (val >> 12) & 0xFFF;
}

void gp1_display_vertical_range(Gpu *gpu, uint32_t val) {
  gpu->display_line_start = val & 0x3FF;
  gpu->display_line_end = (val >> 10) & 0x3FF;
}

void gp1_display_enable(Gpu *gpu, uint32_t val) {
  gpu->display_disabled = val & 1;
}

void gp1_acknowledge_irq(Gpu *gpu, uint32_t val) {
  gpu->interrupt_active = false;
}

void gp1_get_info(Gpu *gpu, uint32_t val) {
  switch (val & 0xF) {
    case 2: {
      uint32_t x_mask = gpu->texture_window_x_mask;
      uint32_t y_mask = gpu->texture_window_y_mask;
      uint32_t x_offset = gpu->texture_window_x_offset;
      uint32_t y_offset = gpu->texture_window_y_offset;
      gpu->read_word = x_mask | (y_mask << 8) | (x_offset << 16) | (y_offset << 24);
    } break;
    case 3: {
      uint32_t top = gpu->drawing_area_top;
      uint32_t left = gpu->drawing_area_left;
      gpu->read_word = left | (top << 10);
    } break;
    case 4: {
      uint32_t bottom = gpu->drawing_area_bottom;
      uint32_t right = gpu->drawing_area_right;
      gpu->read_word = right | (bottom << 10);
    } break;
    case 5: {
      uint32_t x = gpu->drawing_x_offset;
      uint32_t y = gpu->drawing_y_offset;
      x = x & 0x7FF;
      y = y & 0x7FF;
      gpu->read_word = x | (y << 11);
    } break;
    case 7:
      gpu->read_word = 2;
      break;
    case 8:
      gpu->read_word = 0;
      break;
  }
}

void gpu_gp1(Gpu *gpu, uint32_t val) {
  uint8_t opcode = (val >> 24) & 0xFF;

  LOG_OUTPUT(gpu->output_log_index, "GP1 Command: %08x", val);

  switch (opcode) {
    case 0x00:
      gp1_reset(gpu, val);
      break;
    case 0x01:
      gp1_reset_command_buffer(gpu, val);
      break;
    case 0x02:
      gp1_acknowledge_irq(gpu, val);
      break;
    case 0x03:
      gp1_display_enable(gpu, val);
      break;
    case 0x04:
      gp1_dma_direction(gpu, val);
      break;
    case 0x05:
      gp1_display_vram_start(gpu, val);
      break;
    case 0x06:
      gp1_display_horizontal_range(gpu, val);
      break;
    case 0x07:
      gp1_display_vertical_range(gpu, val);
      break;
    case 0x08:
      gp1_display_mode(gpu, val);
      break;
    case 0x10:
      gp1_get_info(gpu, val);
      break;
    default:
      fatal("Unhandled GP1 Command: 0x%08X", val);
  }

  print_output_log(gpu->output_log_index);
}

void gpu_draw_tri(Gpu *gpu) {
  GpuRenderer *renderer = &gpu->renderer;

  for (int i=0; i<3; i++) {
    renderer->tri_pos[i][0] += gpu->drawing_x_offset;
    renderer->tri_pos[i][1] += gpu->drawing_y_offset;
  }

  float area = edge_func(renderer->tri_pos[0], renderer->tri_pos[1], renderer->tri_pos[2]);
  if (area < 0) {
    Vec2 tmp = {renderer->tri_pos[1][0], renderer->tri_pos[1][1]};
    memcpy(renderer->tri_pos[1], renderer->tri_pos[2], sizeof renderer->tri_pos[1]);
    memcpy(renderer->tri_pos[2], tmp, sizeof renderer->tri_pos[2]);
    tmp[0] = renderer->tri_tex[1][0];
    tmp[1] = renderer->tri_tex[1][1];
    memcpy(renderer->tri_tex[1], renderer->tri_tex[2], sizeof renderer->tri_tex[1]);
    memcpy(renderer->tri_tex[2], tmp, sizeof renderer->tri_tex[2]);
    area *= -1;
  }

  for(uint16_t i = gpu->drawing_area_left; i <= gpu->drawing_area_right; i++) {
    for(uint16_t j = gpu->drawing_area_top; j <= gpu->drawing_area_bottom; j++) {
      Vec2 p = {i + 0.5f, j + 0.5f};
      float w0 = edge_func(renderer->tri_pos[1], renderer->tri_pos[2], p);
      float w1 = edge_func(renderer->tri_pos[2], renderer->tri_pos[0], p);
      float w2 = edge_func(renderer->tri_pos[0], renderer->tri_pos[1], p);
      if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
        w0 /= area;
        w1 /= area;
        w2 /= area;

        Vec3 shaded_color = {
          w0 * renderer->tri_color[0][0] + w1 * renderer->tri_color[1][0] + w2 * renderer->tri_color[2][0],
          w0 * renderer->tri_color[0][1] + w1 * renderer->tri_color[1][1] + w2 * renderer->tri_color[2][1],
          w0 * renderer->tri_color[0][2] + w1 * renderer->tri_color[1][2] + w2 * renderer->tri_color[2][2]
        };
        Vec2 interop_tex = {
          w0 * renderer->tri_tex[0][0] + w1 * renderer->tri_tex[1][0] + w2 * renderer->tri_tex[2][0],
          w0 * renderer->tri_tex[0][1] + w1 * renderer->tri_tex[1][1] + w2 * renderer->tri_tex[2][1],
        };

        uint16_t *target = get_vram(renderer, i, j);
        uint16_t new_color;
        switch (gpu->blend_mode) {
          case GpuNoTexture:
            *target = vec_to_555(shaded_color);
            break;
          case GpuBlendedTexture:
            switch (gpu->texture_depth) {
              case GpuTexture4Bits:
                new_color = multiply_888_555(vec_to_888(shaded_color), get_texel_4bit(gpu, interop_tex[0], interop_tex[1]));
                break;
              case GpuTexture8Bits:
                new_color = multiply_888_555(vec_to_888(shaded_color), get_texel_8bit(gpu, interop_tex[0], interop_tex[1]));
                break;
              case GpuTexture15Bits:
                new_color = multiply_888_555(vec_to_888(shaded_color), get_texel_15bit(gpu, interop_tex[0], interop_tex[1]));
                break;
            }
            if (new_color)
              *target = new_color;
            break;
          case GpuRawTexture:
            switch (gpu->texture_depth) {
              case GpuTexture4Bits:
                new_color = get_texel_4bit(gpu, interop_tex[0], interop_tex[1]);
                break;
              case GpuTexture8Bits:
                new_color = get_texel_8bit(gpu, interop_tex[0], interop_tex[1]);
                break;
              case GpuTexture15Bits:
                new_color = get_texel_15bit(gpu, interop_tex[0], interop_tex[1]);
                break;
            }
            if (new_color)
              *target = new_color;
            break;
        }
      }
    }
  }
}

void gpu_draw_rect(Gpu *gpu) {
}

void gpu_draw(Gpu *gpu) {
  switch (gpu->renderer.render_mode) {
    case GpuRenderTri:
      gpu_draw_tri(gpu);
      break;
    case GpuRenderRect:
      gpu_draw_rect(gpu);
      break;
  }
}

void destroy_gpu(Gpu *gpu) {
  destroy_renderer(&gpu->renderer);
}
