#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#include "dma.h"
#include "instruction.h"

#ifndef GPU_H
#define GPU_H

typedef enum GpuTransparencyMode {
  GpuTransparencyMean,
  GpuTransparencySum,
  GpuTransparencyDiff,
  GpuTransparencySumQuarter
} GpuTransparencyMode;

typedef enum GpuTextureBlend {
  GpuNoTexture,
  GpuRawTexture,
  GpuBlendedTexture
} GpuTextureBlend;

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
  Gp0ImageLoadMode,
  Gp0ImageStoreMode
} GP0Mode;

typedef int32_t Vec2[2];
typedef int32_t Vec3[3];

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

static inline void tex_from_gp0(uint32_t val, Vec2 vec) {
  uint8_t x = val;
  uint8_t y = val >> 8;

  vec[0] = x;
  vec[1] = y;
}

typedef struct Gpu Gpu;

static inline uint16_t vec_to_555(Vec3 color) {
  uint16_t r = color[0];
  uint16_t g = color[1];
  uint16_t b = color[2];

  r = (r & 0xF8) >> 3;
  g = (g & 0xF8) >> 3;
  b = (b & 0xF8) >> 3;

  return (r << 10) | (g << 5) | b;
}

static inline uint32_t vec_to_888(Vec3 color) {
  uint32_t r = color[0];
  uint32_t g = color[1];
  uint32_t b = color[2];

  r = (r & 0xFF);
  g = (g & 0xFF);
  b = (b & 0xFF);

  return (r << 16) | (g << 8) | b;
}

static inline uint16_t max(uint16_t a, uint16_t b) {
  return (a > b) ? a : b;
}

static inline uint16_t min(uint16_t a, uint16_t b) {
  return (a > b) ? b : a;
}

static inline uint16_t multiply_888_555(uint32_t color888, uint16_t color555) {
  uint16_t b = min(31, ((color888 & 0xFF) * (color555 & 0x1F)) >> 7);
  uint16_t g = min(31, (((color888 >> 8) & 0xFF) * ((color555 >> 5) & 0x1F)) >> 7);
  uint16_t r = min(31, (((color888 >> 16) & 0xFF) * ((color555 >> 10) & 0x1F)) >> 7);

  return (r << 10) | (g << 5) | b;
}

typedef enum GpuRenderMode {
  GpuRenderTri,
  GpuRenderRect
} GpuRenderMode;

typedef struct GpuRenderer {
  SDL_Window *window;
  SDL_Surface *window_surface;
  SDL_Surface *vram_surface;
  GpuRenderMode render_mode;
  Vec2 tri_pos[3];
  Vec3 tri_color[3];
  Vec2 tri_tex[3];
  Vec2 rect_pos;
  Vec2 rect_size;
  Vec3 rect_color;
  Vec2 rect_tex;
} GpuRenderer;

GpuRenderer init_renderer();
void renderer_update_window(GpuRenderer *renderer);

static inline uint16_t *get_vram(GpuRenderer *renderer, uint16_t x, uint16_t y) {
  SDL_Surface *surface = renderer->vram_surface;
  uint16_t *target = surface->pixels;
  target += (y * surface->pitch + x * surface->format->BytesPerPixel) / 2;
  return target;
}

void destroy_renderer(GpuRenderer *renderer);

typedef struct GpuImageBuffer {
  uint16_t left;
  uint16_t top;
  uint16_t width;
  uint16_t height;
  uint16_t x;
  uint16_t y;
} GpuImageBuffer;

static inline void push_image_word(GpuImageBuffer *buffer, GpuRenderer *renderer, uint32_t image_word) {
  uint16_t *target = get_vram(renderer, buffer->x, buffer->y);
  *target = image_word;

  buffer->x++;
  if (buffer->x == buffer->left + buffer->width) {
    buffer->x = buffer->left;
    buffer->y++;
  }

  target = get_vram(renderer, buffer->x, buffer->y);
  *target = image_word >> 16;

  buffer->x++;
  if (buffer->x == buffer->left + buffer->width) {
    buffer->x = buffer->left;
    buffer->y++;
  }
}

static inline void pop_image_word(GpuImageBuffer *buffer, GpuRenderer *renderer, uint32_t *store) {
  uint16_t *target = get_vram(renderer, buffer->x, buffer->y);
  uint32_t w1 = *target;

  buffer->x++;
  if (buffer->x == buffer->left + buffer->width) {
    buffer->x = buffer->left;
    buffer->y++;
  }

  target = get_vram(renderer, buffer->x, buffer->y);
  uint32_t w2 = *target;

  buffer->x++;
  if (buffer->x == buffer->left + buffer->width) {
    buffer->x = buffer->left;
    buffer->y++;
  }

  *store = w1 | (w2 << 16);
}

static inline float edge_func(Vec2 a, Vec2 b, Vec2 c) {
  return (c[0] - a[0]) * (b[1] - a[1]) - (c[1] - a[1]) * (b[0] - a[0]);
}

typedef void (*GP0Method)(Gpu *gpu, uint32_t val);

typedef struct Gpu {
  uint16_t texture_page[2];
  uint16_t clut[2];
  bool semi_transparent;
  GpuTransparencyMode semi_transparency_mode;
  GpuTextureDepth texture_depth;
  GpuTextureBlend blend_mode;
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
  GpuImageBuffer image_buffer;
  GpuRenderer renderer;
  uint32_t read_word;
  size_t output_log_index;
} Gpu;

static inline void set_clut(Gpu *gpu, uint32_t val) {
  uint16_t clut = val >> 16;
  gpu->clut[0] = (clut & 0x3F) << 4;
  gpu->clut[1] = (clut >> 6) & 0x1FF;
}

static inline void set_texture_params(Gpu *gpu, uint32_t val) {
  uint16_t tex = val >> 16;
  gpu->texture_page[0] = (tex & 0xF) << 6;
  gpu->texture_page[1] = ((tex >> 4) & 1) << 8;
  gpu->semi_transparency_mode = (tex >> 5) & 3;
  gpu->texture_depth = (tex >> 7) & 3;
  if (gpu->texture_depth == 3) {
    log_error("Invalid Texture Depth detected!");
    gpu->texture_depth = GpuTexture15Bits;
  }
}

static inline uint16_t bgr_to_rgb(uint16_t bgr) {
  uint16_t red = bgr & 0x1F;
  uint16_t green = (bgr >> 5) & 0x1F;
  uint16_t blue = (bgr >> 10) & 0x1F;

  return blue | (green << 5) | (red << 10) | (bgr & (1 << 15));
}

static inline uint16_t get_texel(Gpu *gpu, uint16_t x, uint16_t y, GpuTextureDepth depth) {
  uint16_t add;
  switch (depth) {
    case GpuTexture4Bits:
      add = x/4;
      break;
    case GpuTexture8Bits:
      add = x/2;
      break;
    case GpuTexture15Bits:
      add = x;
      break;
  }
  uint16_t texel = *get_vram(&gpu->renderer, gpu->texture_page[0] + add, gpu->texture_page[1] + y);

  if (depth == GpuTexture15Bits)
    return bgr_to_rgb(texel);

  uint16_t index;
  switch (depth) {
    case GpuTexture4Bits:
      index = (texel >> (x % 4) * 4) & 0xF;
      break;
    case GpuTexture8Bits:
      index = (texel >> (x % 2) * 8) & 0xFF;
      break;
    default:
      break;
  }

  return bgr_to_rgb(*get_vram(&gpu->renderer, gpu->clut[0] + index, gpu->clut[1]));
}

Gpu init_gpu();
uint32_t gpu_status(Gpu *gpu);
uint32_t gpu_read(Gpu *gpu);
void gpu_gp0(Gpu *gpu, uint32_t val);
void gpu_gp1(Gpu *gpu, uint32_t val);
void gpu_draw(Gpu *gpu);
void destroy_gpu(Gpu *gpu);

#endif
