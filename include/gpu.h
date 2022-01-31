#include <stdint.h>
#include <stdbool.h>

#include "types.h"
#include "fifo.h"
#include "renderer.h"

#ifndef GPU_H
#define GPU_H

#define GPU_CYCLES_FRACTIONAL_POWER 65536
#define GPU_CYCLES_NTSC 103896
#define GPU_CYCLES_PAL 102948

// GpuState defines the set GPU parameters
// sent by the PS1 on the the actual GPU.
// It doesn't encompass all state variables
// for GPU emulation
typedef struct GpuState GpuState;

typedef enum GpuTransparencyMode {
  GpuTransparencyMean,
  GpuTransparencySum,
  GpuTransparencyDiff,
  GpuTransparencySumQuarter
} GpuTransparencyMode;

typedef enum GpuTexDepth {
  GpuTex4Bit,
  GpuTex8Bit,
  GpuTex15Bit
} GpuTexDepth;

typedef struct GpuDrawMode {
  uint16_t tex_page_x;
  uint16_t tex_page_y;
  GpuTransparencyMode transparency_mode;
  GpuTexDepth tex_depth;
  bool dither;
  bool draw_to_display;
  bool tex_disabled;
  bool rect_tex_x_flip;
  bool rect_tex_y_flip;
} GpuDrawMode;

GpuDrawMode init_draw_mode();
void reset_draw_mode(GpuDrawMode *mode);

typedef enum GpuDmaDirection {
  GpuDmaDirOff,
  GpuDmaDirFifo,
  GpuDmaDirCpuToGp0,
  GpuDmaDirOffVRamToCpu
} GpuDmaDirection;

typedef struct GpuDrawArea {
  uint32_t left;
  uint32_t top;
  uint32_t right;
  uint32_t bottom;
  int32_t offset_x;
  int32_t offset_y;
} GpuDrawArea;

GpuDrawArea init_draw_area();
void reset_draw_area(GpuDrawArea *area);

typedef struct GpuTexWindow {
  uint8_t mask_x;
  uint8_t mask_y;
  uint8_t offset_x;
  uint8_t offset_y;
} GpuTexWindow;

GpuTexWindow init_tex_window();
void reset_tex_window(GpuTexWindow *window);

typedef struct GpuMaskSettings {
  bool force_mask_bit;
  bool preserve_masked_pixels;
} GpuMaskSettings;

typedef struct GpuDisplaySettings {
  uint16_t hres;
  bool fixed_hres;
  uint16_t vres;
  bool in_PAL_mode;
  bool in_24_bit;
  bool is_interlaced;
  uint16_t line_start;
  uint16_t line_end;
  uint16_t col_start;
  uint16_t col_end;
  bool active;
  uint16_t vram_start_x;
  uint16_t vram_start_y;
  bool disabled;
} GpuDisplaySettings;

GpuDisplaySettings init_display();
void reset_display(GpuDisplaySettings *display);

static inline uint16_t get_hres(GpuDisplaySettings *display) {
  if (display->fixed_hres)
    return 368;

  return display->hres;
}

static inline bool get_hres_from_field(GpuDisplaySettings *display, uint8_t *field) {
  if (display->fixed_hres)
    return false;

  if (display->hres == 256) {
    *field = 0;
    return true;
  }

  if (display->hres == 320) {
    *field = 1;
    return true;
  }

  if (display->hres == 512) {
    *field = 2;
    return true;
  }

  *field = 3;
  return true;
}

static inline void set_hres_from_field(GpuDisplaySettings *display, uint32_t field) {
  switch (field & 3) {
    case 0:
      display->hres = 256;
      break;
    case 1:
      display->hres = 320;
      break;
    case 2:
      display->hres = 512;
      break;
    case 3:
      display->hres = 640;
      break;
  }
}

static inline bool interlaced_display(GpuDisplaySettings *display) {
  return display->is_interlaced && (display->vres == 480);
}

typedef struct GpuState {
  GpuDrawMode draw_mode;
  GpuDmaDirection dma_dir;
  GpuDrawArea draw_area;
  GpuTexWindow tex_window;
  GpuMaskSettings mask_settings;
  GpuDisplaySettings display_settings;
  uint32_t read_word;
} GpuState;

GpuState init_gpu_state();

typedef struct Gpu Gpu;

typedef void (*Gp0Func)(Gpu *gpu);

typedef struct Gp0Command {
  Gp0Func handler;
  uint8_t len;
  uint8_t fifo_overhead;
  bool out_of_fifo;
} Gp0Command;

#define MAKE_Gp0Command(handler, len, fifo_overhead, out_of_fifo)\
{\
  handler,\
  len,\
  fifo_overhead,\
  out_of_fifo\
}

extern Gp0Command const gp0_commands[0x100];

static inline bool get_next_fifo_command(CommandFifo *fifo, Gp0Command *command) {
  if (command_fifo_empty(fifo))
      return false;

  uint32_t next_command = command_fifo_pop(fifo);
  *command = gp0_commands[next_command >> 24];
  return true;
}

typedef struct GpuTime {
  Cycles gpu_draw_cycles;
  uint16_t fractional_cycles;
  bool in_hsync;
  Cycles cycles_to_hsync;
  uint16_t cur_line;
  uint16_t cur_line_vram_offset; // relative to display.vram_start_y
  uint16_t cur_line_vram_y;
  bool line_phase;
  bool frame_finished;
} GpuTime;

GpuTime init_gpu_time();

static inline void gpu_add_time(GpuTime *time, Cycles cycles) {
  time->gpu_draw_cycles.data += cycles.data << 1;

  if (time->gpu_draw_cycles.data > 256)
    time->gpu_draw_cycles.data = 256;
}

static inline void gpu_consume_time(GpuTime *time, Cycles cycles) {
  time->gpu_draw_cycles.data -= cycles.data;
}

typedef struct Psx Psx;

typedef struct Gpu {
  GpuState state;
  CommandFifo fifo;
  Renderer renderer;
  GpuTime gpu_time;
} Gpu;

Gpu init_gpu();
void reset_gpu_state(Gpu *gpu);
void reset_command_buffer(Gpu *gpu);

void run_gpu(Psx *psx);

bool push_command_fifo(CommandFifo *fifo, uint32_t val);
void execute_command(Gpu *gpu);

void gp0(Gpu *gpu, uint32_t val);
void gp1(Gpu *gpu, uint32_t val);

uint32_t load_gpu(Gpu *gpu, Addr addr, AddrType type);
void store_gpu(Gpu *gpu, Addr addr, uint32_t val, AddrType type);

uint32_t gpu_status(GpuState *state);
uint32_t gpu_read(GpuState *state);

static inline Cycles gpu_tick(Gpu *gpu, Cycles cpu_cycles) {
  uint64_t gpu_cycles_ratio = gpu->state.display_settings.in_PAL_mode ? GPU_CYCLES_PAL : GPU_CYCLES_NTSC;
  uint64_t gpu_cycles = gpu->gpu_time.fractional_cycles + gpu_cycles_ratio * cpu_cycles.data;
  gpu->gpu_time.fractional_cycles = gpu_cycles % GPU_CYCLES_FRACTIONAL_POWER;
  return MAKE_Cycles(gpu_cycles / GPU_CYCLES_FRACTIONAL_POWER);
}

static inline uint16_t gpu_get_line_length(Gpu *gpu) {
  if (gpu->state.display_settings.in_PAL_mode)
    return 3405;
  return 3412 + gpu->gpu_time.line_phase;
}

static inline uint16_t gpu_get_lines_per_field(Gpu *gpu) {
   if (gpu->state.display_settings.in_PAL_mode)
    return 314;
  return 263;
}

void destroy_gpu(Gpu *gpu);

#endif

/*
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#ifndef GPU_H
#define GPU_H

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
*/
