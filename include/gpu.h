#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

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

typedef struct GpuCommandBuffer {
  uint32_t commands[12];
  uint8_t command_count;
} GpuCommandBuffer;

GpuCommandBuffer init_command_buffer();
static inline void command_buffer_clear(GpuCommandBuffer *command_buffer) {
  command_buffer->command_count = 0;
}
static inline void push_command(GpuCommandBuffer *command_buffer, uint32_t command) {
  command_buffer->commands[command_buffer->command_count] = command;
  command_buffer->command_count++;
}

typedef enum GP0Mode {
  Gp0CommandMode,
  Gp0ImageLoadMode
} GP0Mode;

typedef struct GpuRenderer {
  SDL_Window *window;
} GpuRenderer;

GpuRenderer init_renderer();
void destroy_renderer(GpuRenderer *renderer);

typedef struct Gpu Gpu;

typedef void (*GP0Method)(Gpu *gpu, uint32_t val);

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
  GpuCommandBuffer gp0_command_buffer;
  uint32_t gp0_words_remaining;
  GP0Method gp0_method;
  GP0Mode gp0_mode;
  GpuRenderer renderer;
  size_t output_log_index;
} Gpu;

Gpu init_gpu();
uint32_t gpu_status(Gpu *gpu);
uint32_t gpu_read(Gpu *gpu);
void gpu_gp0(Gpu *gpu, uint32_t val);
void gpu_gp1(Gpu *gpu, uint32_t val);
void destroy_gpu(Gpu *gpu);

#endif
