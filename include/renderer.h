#include <SDL2/SDL.h>

#ifndef GPU_RENDERER_H
#define GPU_RENDERER_H

typedef struct Renderer {
  SDL_Window *window;
  SDL_Surface *window_surface;
  SDL_Surface *vram;
} Renderer;

Renderer init_renderer();

static inline uint16_t *get_vram(Renderer *renderer, uint16_t x, uint16_t y) {
  SDL_Surface *vram = renderer->vram;
  uint16_t *target = vram->pixels;
  target += (y * vram->pitch + x * vram->format->BytesPerPixel) / 2;
  return target;
}

void destroy_renderer(Renderer *renderer);

#endif
