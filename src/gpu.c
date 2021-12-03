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

  return renderer;
}

void destroy_renderer(GpuRenderer *renderer) {
  SDL_DestroyWindow(renderer->window);
}

void gp0_nop(Gpu *gpu, uint32_t val);

Gpu init_gpu() {
  Gpu gpu;

  gpu.page_base_x = 0;
  gpu.page_base_y = 0;
  gpu.semi_transparency_blend = GpuTransparencyMean;
  gpu.texture_depth = GpuTexture4Bits;
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
  gpu.renderer = init_renderer();
  gpu.output_log_index = init_output_log();

  return gpu;
}

uint32_t gpu_status(Gpu *gpu) {
  uint32_t status = 0;

  status |= gpu->page_base_x << 0;
  status |= gpu->page_base_y << 4;
  status |= ((uint32_t)gpu->semi_transparency_blend) << 5;
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
  // TempFix: Return actual GPUREAD data once it
  // has been initialized
  return 0;
}

void gp0_nop(Gpu *gpu, uint32_t val) {
}

void gp0_clear_cache(Gpu *gpu, uint32_t val) {
}

void gp0_draw_mode(Gpu *gpu, uint32_t val) {
  gpu->page_base_x = (val & 0xF);
  gpu->page_base_y = ((val >> 4) & 1);
  switch ((val >> 5) & 3) {
    case 0:
      gpu->semi_transparency_blend = GpuTransparencyMean;
      break;
    case 1:
      gpu->semi_transparency_blend = GpuTransparencySum;
      break;
    case 2:
      gpu->semi_transparency_blend = GpuTransparencyDiff;
      break;
    case 3:
      gpu->semi_transparency_blend = GpuTransparencySumQuarter;
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
  log_trace("STUB: Draw Monochrome Quad!");
}

void gp0_texture_blend_quad(Gpu *gpu, uint32_t val) {
  log_trace("STUB: Draw Texture Solid Blended Quad!");
}

void gp0_shaded_tri(Gpu *gpu, uint32_t val) {
  log_trace("STUB: Draw Shaded Tri!");
}

void gp0_shaded_quad(Gpu *gpu, uint32_t val) {
  log_trace("STUB: Draw Shaded Quad!");
}

void gp0_monochrome_rect(Gpu *gpu, uint32_t val) {
  log_trace("STUB: Draw Monochrome Rect!");
}

void gp0_image_load(Gpu *gpu, uint32_t val) {
  uint32_t width = gpu->gp0_command_buffer.commands[2] & 0xFFFF;
  uint32_t height = gpu->gp0_command_buffer.commands[2] >> 16;

  uint32_t size = width * height;
  size = (size + 1) & ~1;

  gpu->gp0_words_remaining = size / 2;
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
        gpu->gp0_words_remaining = 4;
        break;
      case 0x2C:
        gpu->gp0_method = gp0_texture_blend_quad;
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
      // ToDo: Load image data into VRAM
      if (gpu->gp0_words_remaining == 0)
        gpu->gp0_mode = Gp0CommandMode;
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
  gpu->page_base_x = gpu->page_base_y = 0;
  gpu->semi_transparency_blend = GpuTransparencyMean;
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
    default:
      fatal("Unhandled GP1 Command: 0x%08X", val);
  }

  print_output_log(gpu->output_log_index);
}

void destroy_gpu(Gpu *gpu) {
  destroy_renderer(&gpu->renderer);
}
