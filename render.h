#ifndef LGBTYCOON_RENDER_H
#define LGBTYCOON_RENDER_H

#include "entity.h"
#include "game.h"
#include <SDL2/SDL.h>

#define MAX_PARTICLES 1024

typedef struct {
    float x, y;
    float vx, vy;
    float life;
    SDL_Color color;
} Particle;

typedef struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    int           tile_size;
    int           hover_x;
    int           hover_y;
    Particle      particles[MAX_PARTICLES];
    uint32_t      last_frame_time;
} RenderContext;

int  render_init(RenderContext *ctx);
int  render_screen_to_tile(const RenderContext *ctx, int sx, int sy,
                           int16_t *tile_x, int16_t *tile_y);
void render_present(RenderContext *ctx, const World *w, const GameState *g);
void render_cleanup(RenderContext *ctx);

#endif
