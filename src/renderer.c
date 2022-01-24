#include "renderer.h"
#include "log.h"

Renderer init_renderer() {
  Renderer renderer;

  renderer.window = SDL_CreateWindow("PSX", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 512, 0);

  renderer.window_surface = SDL_GetWindowSurface(renderer.window);
  if (renderer.window_surface == NULL)
    fatal("SDLError: Couldn't initialize window_surface: %s", SDL_GetError());

  renderer.vram = SDL_CreateRGBSurfaceWithFormat(0, 1024, 512, 16, SDL_PIXELFORMAT_RGB555);
  if (renderer.vram == NULL)
    fatal("SDLError: Couldn't initialize vram_surface: %s", SDL_GetError());

  return renderer;
}

void destroy_renderer(Renderer *renderer) {
  SDL_DestroyWindow(renderer->window);
}
