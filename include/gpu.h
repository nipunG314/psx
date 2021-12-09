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

typedef float Vec2[2];
typedef float Vec3[3];

static inline void pos_from_gp0(uint32_t val, Vec2 vec) {
  int16_t x = val;
  int16_t y = val >> 16;

  vec[0] = x;
  vec[1] = y;
}

static inline void color_from_gp0(uint32_t val, Vec3 vec) {
  uint8_t r = val;
  uint8_t g = val >> 8;
  uint8_t b = val >> 16;

  vec[0] = r;
  vec[1] = g;
  vec[2] = b;
}

typedef struct Gpu Gpu;

static inline uint16_t rgb_888_to_555(Gpu *gpu, Vec3 color) {
  uint16_t r = color[0];
  uint16_t g = color[1];
  uint16_t b = color[2];

  r = (r & 0xF8) >> 3;
  g = (g & 0xF8) >> 3;
  b = (b & 0xF8) >> 3;

  return (r << 10) | (g << 5) | b;
}

typedef struct GpuRenderer {
  SDL_Window *window;
  SDL_Surface *window_surface;
  SDL_Surface *vram_surface;
  Vec2 pos[3];
  Vec3 color[3];
} GpuRenderer;

GpuRenderer init_renderer();
void renderer_update_window(GpuRenderer *renderer);
void destroy_renderer(GpuRenderer *renderer);

typedef struct GpuImageLoadBuffer {
  uint16_t left;
  uint16_t top;
  uint16_t width;
  uint16_t height;
  uint16_t x;
  uint16_t y;
} GpuImageLoadBuffer;

static inline void push_image_word(GpuImageLoadBuffer *buffer, GpuRenderer *renderer, uint32_t image_word) {
  SDL_Surface *surface = renderer->vram_surface;
  uint16_t *_target = surface->pixels;
  uint16_t *target = _target + (buffer->y * surface->pitch + buffer->x * surface->format->BytesPerPixel) / 2;
  *target = image_word;

  buffer->x++;
  if (buffer->x == buffer->left + buffer->width) {
    buffer->x = buffer->left;
    buffer->y++;
  }

  target = _target + (buffer->y * surface->pitch + buffer->x * surface->format->BytesPerPixel) / 2;
  *target = image_word >> 16;

  buffer->x++;
  if (buffer->x == buffer->left + buffer->width) {
    buffer->x = buffer->left;
    buffer->y++;
  }
}

static inline float edge_func(Vec2 a, Vec2 b, Vec2 c) {
  return (c[0] - a[0]) * (b[1] - a[1]) - (c[1] - a[1]) * (b[0] - a[0]);
}

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
  GpuImageLoadBuffer image_load_buffer;
  GpuRenderer renderer;
  size_t output_log_index;
} Gpu;

Gpu init_gpu();
uint32_t gpu_status(Gpu *gpu);
uint32_t gpu_read(Gpu *gpu);
void gpu_gp0(Gpu *gpu, uint32_t val);
void gpu_gp1(Gpu *gpu, uint32_t val);
void gpu_draw(Gpu *gpu);
void destroy_gpu(Gpu *gpu);

#endif
