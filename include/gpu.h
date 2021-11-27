#include <stdint.h>
#include <stdbool.h>

#include "dma.h"
#include "instruction.h"

#ifndef GPU_H
#define GPU_H

typedef enum GpuTransparencyBlend {
  GpuTransparencyMean,
  GpuTransparencySum,
  GpuTransparencyDiff,
  GpuTransparencySumQuarter
} GpuTransparencyBlend;

typedef enum GpuTextureDepth {
  GpuTexture4Bits,
  GpuTexture8Bits,
  GpuTexture15Bits
} GpuTextureDepth;

typedef enum GpuInterlaceField {
  GpuInterlaceBottom,
  GpuInterlaceTop
} GpuInterlaceField;

DECLARE_TYPE(uint8_t, GpuHRes)

static inline GpuHRes hres_from_fields(uint8_t h1, uint8_t h2) {
  uint8_t hr = (h2 & 1) | ((h1 & 3) << 1);

  return MAKE_GpuHRes(hr);
}

static inline uint32_t hres_status(GpuHRes hres) {
  uint32_t hr = hres.data;

  return (hr << 16);
}

typedef enum GpuVerticalRes {
  GpuVertical240Lines,
  GpuVertical480Lines
} GpuVerticalRes;

typedef enum GpuVideoMode {
  GpuNTSC,
  GpuPAL
} GpuVideoMode;

typedef enum GpuDisplayDepth {
  GpuDisplayDepth15Bits,
  GpuDisplayDepth24Bits
} GpuDisplayDepth;

typedef enum GpuDmaDirection {
  GpuDmaDirOff,
  GpuDmaDirFifo,
  GpuDmaDirCpuToGp0,
  GpuDmaDirOffVRamToCpu
} GpuDmaDirection;

typedef struct Gpu {
  uint8_t page_base_x;
  uint8_t page_base_y;
  GpuTransparencyBlend semi_transparency_blend;
  GpuTextureDepth texture_depth;
  bool dithering;
  bool draw_to_display;
  bool force_set_mask_bit;
  bool preserve_masked_pixels; 
  GpuInterlaceField interlace_field;
  bool texture_disabled;
  GpuHRes hres;
  GpuVerticalRes vres;
  GpuVideoMode video_mode;
  GpuDisplayDepth display_depth;
  bool interlaced;
  bool display_disabled;
  bool interrupt_active;
  GpuDmaDirection dma_direction;
  bool rectangle_texture_x_flip;
  bool rectangle_texture_y_flip;
  uint8_t texture_window_x_mask;
  uint8_t texture_window_y_mask;
  uint8_t texture_window_x_offset;
  uint8_t texture_window_y_offset;
  uint16_t drawing_area_left;
  uint16_t drawing_area_top;
  uint16_t drawing_area_right;
  uint16_t drawing_area_bottom;
  int16_t drawing_x_offset;
  int16_t drawing_y_offset;
  uint16_t display_vram_x_start;
  uint16_t display_vram_y_start;
  uint16_t display_hor_start;
  uint16_t display_hor_end;
  uint16_t display_line_start;
  uint16_t display_line_end;
} Gpu;

Gpu init_gpu();
uint32_t gpu_status(Gpu *gpu);
uint32_t gpu_read(Gpu *gpu);
void gpu_gp0(Gpu *gpu, uint32_t val);
void gpu_gp1(Gpu *gpu, uint32_t val);

#endif
