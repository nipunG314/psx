#include <stdlib.h>

#include "gpu.h"
#include "log.h"

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
  status |= ((uint32_t)gpu->vres) << 19;
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

void gpu_gp0(Gpu *gpu, uint32_t val) {
  uint8_t opcode = (val >> 24) & 0xFF;

  switch (opcode) {
    case 0x00:
      break;
    case 0xe1:
      gp0_draw_mode(gpu, val);
      break;
    default:
      fatal("Unhandled GP0 Command: 0x%08X", val);
  }
}

