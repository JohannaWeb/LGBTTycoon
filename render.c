#include "render.h"
#include <stdio.h>
#include <stdlib.h>

int render_init(RenderContext *ctx) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return -1;

    ctx->tile_size = 64;
    int window_w = (WORLD_W + WORLD_H) * (ctx->tile_size / 2) + 120;
    int window_h = (WORLD_W + WORLD_H) * (ctx->tile_size / 4) + 150;

    ctx->window = SDL_CreateWindow("LGBTycoon", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                    window_w, window_h, SDL_WINDOW_SHOWN);
    if (!ctx->window) return -1;

    ctx->renderer = SDL_CreateRenderer(ctx->window, -1,
                                       SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ctx->renderer) {
        ctx->renderer = SDL_CreateRenderer(ctx->window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!ctx->renderer) {
        SDL_DestroyWindow(ctx->window);
        SDL_Quit();
        return -1;
    }

    SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ctx->renderer, 16, 19, 26, 255);
    SDL_RenderClear(ctx->renderer);

    ctx->hover_x = -1;
    ctx->hover_y = -1;
    for (int i = 0; i < MAX_PARTICLES; i++) ctx->particles[i].life = 0;
    ctx->last_frame_time = SDL_GetTicks();

    return 0;
}

static void project(RenderContext *ctx, float x, float y, int *sx, int *sy) {
    int tw = ctx->tile_size;
    int th = tw / 2;
    int ox = (WORLD_H * tw) / 2 + 60;
    int oy = 60;
    *sx = ox + (int)((x - y) * (tw / 2.0f));
    *sy = oy + (int)((x + y) * (th / 2.0f));
}

static int floor_to_int(float v) {
    int i = (int)v;
    return (v < (float)i) ? i - 1 : i;
}

int render_screen_to_tile(const RenderContext *ctx, int sx, int sy,
                          int16_t *tile_x, int16_t *tile_y) {
    int tw = ctx->tile_size;
    int th = tw / 2;
    int ox = (WORLD_H * tw) / 2 + 60;
    int oy = 60;
    float a = (float)(sx - ox) / (tw / 2.0f);
    float b = (float)(sy - oy) / (th / 2.0f);
    int x = floor_to_int((a + b) * 0.5f);
    int y = floor_to_int((b - a) * 0.5f);
    if (x < 0 || x >= WORLD_W || y < 0 || y >= WORLD_H) return 0;
    *tile_x = (int16_t)x;
    *tile_y = (int16_t)y;
    return 1;
}

static SDL_Color shade(SDL_Color c, float mul) {
    int r = (int)(c.r * mul);
    int g = (int)(c.g * mul);
    int b = (int)(c.b * mul);
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    return (SDL_Color){(uint8_t)r, (uint8_t)g, (uint8_t)b, c.a};
}

static void fill_poly(RenderContext *ctx, const SDL_Point *p, int point_count,
                      SDL_Color color, const int *indices, int index_count) {
    SDL_Vertex vertices[8];
    for (int i = 0; i < point_count && i < 8; i++) {
        vertices[i].position = (SDL_FPoint){(float)p[i].x, (float)p[i].y};
        vertices[i].color = color;
        vertices[i].tex_coord = (SDL_FPoint){0.0f, 0.0f};
    }
    SDL_RenderGeometry(ctx->renderer, NULL, vertices, point_count, indices, index_count);
}

static void fill_quad(RenderContext *ctx, SDL_Point a, SDL_Point b, SDL_Point c, SDL_Point d,
                      SDL_Color color) {
    SDL_Point p[4] = {a, b, c, d};
    const int indices[6] = {0, 1, 2, 0, 2, 3};
    fill_poly(ctx, p, 4, color, indices, 6);
}

static void stroke_poly(RenderContext *ctx, const SDL_Point *points, int count,
                        SDL_Color color) {
    SDL_SetRenderDrawColor(ctx->renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLines(ctx->renderer, points, count);
}

static void draw_glow(RenderContext *ctx, int sx, int sy, int radius, SDL_Color col) {
    for (int r = radius; r > 0; r -= 6) {
        SDL_SetRenderDrawColor(ctx->renderer, col.r, col.g, col.b,
                               (uint8_t)((col.a * r) / radius / 5));
        SDL_Rect rect = {sx - r, sy - r / 2, r * 2, r};
        SDL_RenderFillRect(ctx->renderer, &rect);
    }
}

static void spawn_particle(RenderContext *ctx, float x, float y, SDL_Color col) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (ctx->particles[i].life <= 0) {
            ctx->particles[i].x = x;
            ctx->particles[i].y = y;
            ctx->particles[i].vx = ((rand() % 100) - 50) / 500.0f;
            ctx->particles[i].vy = ((rand() % 100) - 100) / 300.0f;
            ctx->particles[i].life = 1.0f;
            ctx->particles[i].color = col;
            break;
        }
    }
}

static void draw_iso_tile(RenderContext *ctx, int x, int y, SDL_Color color) {
    int sx, sy;
    project(ctx, (float)x, (float)y, &sx, &sy);
    int tw = ctx->tile_size;
    int th = tw / 2;

    SDL_Point points[5] = {
        {sx, sy}, {sx + tw / 2, sy + th / 2}, {sx, sy + th}, {sx - tw / 2, sy + th / 2}, {sx, sy}
    };

    fill_quad(ctx, points[0], points[1], points[2], points[3], color);
    stroke_poly(ctx, points, 5, (SDL_Color){17, 38, 31, 145});

    if ((x * 7 + y * 3) % 13 == 0) {
        SDL_SetRenderDrawColor(ctx->renderer, 92, 168, 91, 200);
        SDL_RenderDrawLine(ctx->renderer, sx - 4, sy + th / 2 + 1, sx - 4, sy + th / 2 - 5);
        SDL_RenderDrawLine(ctx->renderer, sx - 4, sy + th / 2 - 2, sx, sy + th / 2 - 5);
    }
}

static void draw_iso_block(RenderContext *ctx, int x, int y, int h, SDL_Color color) {
    int sx, sy;
    project(ctx, (float)x, (float)y, &sx, &sy);
    int tw = ctx->tile_size;
    int th = tw / 2;

    SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 60);
    SDL_Rect shadow = {sx - tw / 2 + 6, sy + th / 2 + 5, tw, th / 2};
    SDL_RenderFillRect(ctx->renderer, &shadow);

    SDL_Point top[4] = {
        {sx, sy - h},
        {sx + tw / 2, sy + th / 2 - h},
        {sx, sy + th - h},
        {sx - tw / 2, sy + th / 2 - h},
    };
    SDL_Point base[4] = {
        {sx, sy},
        {sx + tw / 2, sy + th / 2},
        {sx, sy + th},
        {sx - tw / 2, sy + th / 2},
    };

    fill_quad(ctx, top[1], base[1], base[2], top[2], shade(color, 0.55f));
    fill_quad(ctx, top[2], base[2], base[3], top[3], shade(color, 0.42f));
    fill_quad(ctx, top[0], top[1], top[2], top[3], shade(color, 1.15f));

    SDL_Point top_outline[5] = {top[0], top[1], top[2], top[3], top[0]};
    stroke_poly(ctx, top_outline, 5, (SDL_Color){18, 19, 24, 190});
    SDL_SetRenderDrawColor(ctx->renderer, 18, 19, 24, 130);
    SDL_RenderDrawLine(ctx->renderer, top[1].x, top[1].y, base[1].x, base[1].y);
    SDL_RenderDrawLine(ctx->renderer, top[2].x, top[2].y, base[2].x, base[2].y);
    SDL_RenderDrawLine(ctx->renderer, top[3].x, top[3].y, base[3].x, base[3].y);

    if (color.r > 150 && color.b > 150) {
        draw_glow(ctx, sx, sy - h + th / 2, 40, (SDL_Color){255, 100, 255, 100});
        SDL_SetRenderDrawColor(ctx->renderer, 255, 236, 94, 230);
        SDL_RenderDrawLine(ctx->renderer, sx - 16, sy - h + 12, sx + 16, sy - h + 12);
    }
}

static void draw_aura_tile(RenderContext *ctx, int x, int y, SDL_Color color) {
    int sx, sy;
    project(ctx, (float)x, (float)y, &sx, &sy);
    int tw = ctx->tile_size;
    int th = tw / 2;
    SDL_Point p[4] = {
        {sx, sy + 6},
        {sx + tw / 2 - 5, sy + th / 2 + 6},
        {sx, sy + th - 6},
        {sx - tw / 2 + 5, sy + th / 2 + 6},
    };
    fill_quad(ctx, p[0], p[1], p[2], p[3], color);
}

static void draw_tile_outline(RenderContext *ctx, int x, int y, SDL_Color color) {
    int sx, sy;
    project(ctx, (float)x, (float)y, &sx, &sy);
    int tw = ctx->tile_size;
    int th = tw / 2;
    SDL_Point points[5] = {
        {sx, sy}, {sx + tw / 2, sy + th / 2}, {sx, sy + th},
        {sx - tw / 2, sy + th / 2}, {sx, sy}
    };
    stroke_poly(ctx, points, 5, color);
    SDL_Point inner[5] = {
        {sx, sy + 4}, {sx + tw / 2 - 8, sy + th / 2}, {sx, sy + th - 4},
        {sx - tw / 2 + 8, sy + th / 2}, {sx, sy + 4}
    };
    stroke_poly(ctx, inner, 5, color);
}

static void draw_structure_aura(RenderContext *ctx, const Structures *s, uint32_t k) {
    SDL_Color col;
    StructureType type = (StructureType)s->type[k];
    switch (type) {
        case STRUCT_STAGE:           col = (SDL_Color){230, 68, 210, 22}; break;
        case STRUCT_BAR:             col = (SDL_Color){255, 214, 65, 22}; break;
        case STRUCT_CHAPEL_PROTEST:  col = (SDL_Color){32, 32, 38, 32}; break;
        case STRUCT_CORPORATE_BOOTH: col = (SDL_Color){77, 164, 240, 22}; break;
        case STRUCT_LINUS:           col = (SDL_Color){255, 48, 48, 28}; break;
        default:                     col = (SDL_Color){255, 255, 255, 16}; break;
    }

    int16_t cx = s->x[k];
    int16_t cy = s->y[k];
    int r = s->radius[k];
    for (int y = cy - r; y <= cy + r; y++) {
        if (y < 0 || y >= WORLD_H) continue;
        for (int x = cx - r; x <= cx + r; x++) {
            if (x < 0 || x >= WORLD_W) continue;
            draw_aura_tile(ctx, x, y, col);
        }
    }
}

static void draw_guest(RenderContext *ctx, const World *w, uint32_t i) {
    int sx, sy;
    project(ctx, (float)w->x[i], (float)w->y[i], &sx, &sy);

    SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 85);
    SDL_Rect shadow = {sx - 7, sy + 8, 14, 5};
    SDL_RenderFillRect(ctx->renderer, &shadow);

    uint8_t c = w->comfort[i];
    SDL_Color body = {
        (uint8_t)(232 - (c / 6)),
        (uint8_t)(72 + (c / 2)),
        (uint8_t)(122 + (c / 4)),
        255
    };

    SDL_SetRenderDrawColor(ctx->renderer, body.r, body.g, body.b, body.a);
    SDL_Rect torso = {sx - 4, sy - 14, 8, 14};
    SDL_RenderFillRect(ctx->renderer, &torso);
    SDL_SetRenderDrawColor(ctx->renderer, shade(body, 0.55f).r, shade(body, 0.55f).g, shade(body, 0.55f).b, 255);
    SDL_RenderDrawRect(ctx->renderer, &torso);

    SDL_SetRenderDrawColor(ctx->renderer, 255, 221, 180, 255);
    SDL_Rect head = {sx - 4, sy - 21, 8, 7};
    SDL_RenderFillRect(ctx->renderer, &head);
    SDL_SetRenderDrawColor(ctx->renderer, 66, 36, 34, 255);
    SDL_RenderDrawLine(ctx->renderer, sx - 4, sy - 21, sx + 3, sy - 21);
}

static void draw_walker(RenderContext *ctx, int x, int y) {
    int sx, sy;
    project(ctx, (float)x, (float)y, &sx, &sy);

    SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 90);
    SDL_Rect shadow = {sx - 9, sy + 8, 18, 6};
    SDL_RenderFillRect(ctx->renderer, &shadow);

    SDL_SetRenderDrawColor(ctx->renderer, 210, 34, 42, 255);
    SDL_Rect body = {sx - 6, sy - 19, 12, 18};
    SDL_RenderFillRect(ctx->renderer, &body);
    SDL_SetRenderDrawColor(ctx->renderer, 96, 12, 18, 255);
    SDL_RenderDrawRect(ctx->renderer, &body);
    SDL_SetRenderDrawColor(ctx->renderer, 242, 199, 160, 255);
    SDL_Rect head = {sx - 5, sy - 27, 10, 8};
    SDL_RenderFillRect(ctx->renderer, &head);
    SDL_SetRenderDrawColor(ctx->renderer, 35, 35, 38, 255);
    SDL_RenderDrawLine(ctx->renderer, sx - 6, sy - 27, sx + 5, sy - 27);
    SDL_RenderDrawLine(ctx->renderer, sx - 8, sy - 13, sx + 8, sy - 18);
}

static uint8_t font_row(char c, int row) {
    static const uint8_t digits[10][7] = {
        {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14},
        {14,17,1,2,4,8,31}, {30,1,1,14,1,1,30},
        {2,6,10,18,31,2,2}, {31,16,30,1,1,17,14},
        {6,8,16,30,17,17,14}, {31,1,2,4,8,8,8},
        {14,17,17,14,17,17,14}, {14,17,17,15,1,2,12},
    };
    static const uint8_t letters[26][7] = {
        {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
        {14,17,16,16,16,17,14}, {30,17,17,17,17,17,30},
        {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
        {14,17,16,23,17,17,14}, {17,17,17,31,17,17,17},
        {14,4,4,4,4,4,14}, {1,1,1,1,17,17,14},
        {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17},
        {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
        {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
        {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},
        {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
        {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31},
    };
    if (row < 0 || row >= 7) return 0;
    if (c >= '0' && c <= '9') return digits[c - '0'][row];
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    if (c >= 'A' && c <= 'Z') return letters[c - 'A'][row];
    if (c == ':') return row == 1 || row == 5 ? 4 : 0;
    if (c == '-') return row == 3 ? 14 : 0;
    if (c == '/') return (uint8_t)(1u << (6 - row > 4 ? 4 : 6 - row));
    if (c == '.') return row == 6 ? 4 : 0;
    return 0;
}

static void draw_text(RenderContext *ctx, int x, int y, const char *text,
                      SDL_Color color, int scale) {
    SDL_SetRenderDrawColor(ctx->renderer, color.r, color.g, color.b, color.a);
    int pen = x;
    for (const char *p = text; *p; p++) {
        if (*p == ' ') {
            pen += 4 * scale;
            continue;
        }
        for (int row = 0; row < 7; row++) {
            uint8_t bits = font_row(*p, row);
            for (int col = 0; col < 5; col++) {
                if (!(bits & (1u << (4 - col)))) continue;
                SDL_Rect px = {pen + col * scale, y + row * scale, scale, scale};
                SDL_RenderFillRect(ctx->renderer, &px);
            }
        }
        pen += 6 * scale;
    }
}

static void draw_hud_bar(RenderContext *ctx, int x, int y, int w, int h,
                         int value, int max_value, SDL_Color fill) {
    if (value < 0) value = 0;
    if (value > max_value) value = max_value;
    SDL_SetRenderDrawColor(ctx->renderer, 18, 20, 27, 230);
    SDL_Rect bg = {x, y, w, h};
    SDL_RenderFillRect(ctx->renderer, &bg);
    SDL_SetRenderDrawColor(ctx->renderer, fill.r, fill.g, fill.b, fill.a);
    SDL_Rect fg = {x + 2, y + 2, (w - 4) * value / max_value, h - 4};
    SDL_RenderFillRect(ctx->renderer, &fg);
    SDL_SetRenderDrawColor(ctx->renderer, 220, 226, 235, 120);
    SDL_RenderDrawRect(ctx->renderer, &bg);
}

static void draw_hud(RenderContext *ctx, const GameState *g) {
    SDL_SetRenderDrawColor(ctx->renderer, 11, 13, 19, 235);
    SDL_Rect panel = {12, 10, 450, 92};
    SDL_RenderFillRect(ctx->renderer, &panel);
    SDL_SetRenderDrawColor(ctx->renderer, 226, 232, 240, 105);
    SDL_RenderDrawRect(ctx->renderer, &panel);

    char line[128];
    snprintf(line, sizeof line, "DAY %u  GUESTS %d  AVG %d",
             g->day, g->alive_guests, g->average_comfort);
    draw_text(ctx, 24, 22, line, (SDL_Color){232, 238, 246, 255}, 2);

    snprintf(line, sizeof line, "TRUST %d", g->community_trust);
    draw_text(ctx, 24, 48, line, (SDL_Color){232, 238, 246, 255}, 1);
    draw_hud_bar(ctx, 98, 45, 96, 12, g->community_trust, 120, (SDL_Color){66, 196, 126, 255});

    snprintf(line, sizeof line, "FUND %d", g->funding);
    draw_text(ctx, 212, 48, line, (SDL_Color){232, 238, 246, 255}, 1);
    draw_hud_bar(ctx, 278, 45, 72, 12, g->funding, 160, (SDL_Color){240, 198, 70, 255});

    snprintf(line, sizeof line, "VOL %d", g->volunteers);
    draw_text(ctx, 362, 48, line, (SDL_Color){232, 238, 246, 255}, 1);

    snprintf(line, sizeof line, "SAFE %d  NOISE %d  %s",
             g->safety, g->noise_pressure,
             structure_type_name((StructureType)g->selected_type));
    draw_text(ctx, 24, 70, line, (SDL_Color){183, 208, 236, 255}, 1);

    if (g->paused || g->debug_free_place || g->status != GAME_RUNNING) {
        snprintf(line, sizeof line, "%s%s%s",
                 g->paused ? "PAUSED " : "",
                 g->debug_free_place ? "DEBUG " : "",
                 g->status == GAME_WON ? "WON" : (g->status == GAME_LOST ? "LOST" : ""));
        draw_text(ctx, 338, 70, line, (SDL_Color){255, 235, 132, 255}, 1);
    }

    if (g->message[0]) {
        SDL_SetRenderDrawColor(ctx->renderer, 11, 13, 19, 220);
        SDL_Rect msg = {12, 108, 560, 28};
        SDL_RenderFillRect(ctx->renderer, &msg);
        draw_text(ctx, 24, 118, g->message, (SDL_Color){236, 220, 186, 255}, 1);
    }
}

void render_present(RenderContext *ctx, const World *w, const GameState *g) {
    uint32_t now = SDL_GetTicks();
    float dt = (now - ctx->last_frame_time) / 1000.0f;
    ctx->last_frame_time = now;

    SDL_SetRenderDrawColor(ctx->renderer, 15, 18, 27, 255);
    SDL_RenderClear(ctx->renderer);

    SDL_SetRenderDrawColor(ctx->renderer, 10, 12, 18, 170);
    SDL_Rect backdrop = {35, 44, (WORLD_W + WORLD_H) * ctx->tile_size / 2 + 48,
                         (WORLD_W + WORLD_H) * ctx->tile_size / 4 + 76};
    SDL_RenderFillRect(ctx->renderer, &backdrop);

    for (int y = 0; y < WORLD_H; y++) {
        for (int x = 0; x < WORLD_W; x++) {
            SDL_Color grass = {
                (uint8_t)(34 + (x + y) % 8),
                (uint8_t)(95 + (x * 3 + y * 5) % 26),
                (uint8_t)(66 + (x * 5 + y * 2) % 16),
                255
            };
            draw_iso_tile(ctx, x, y, grass);
        }
    }

    for (uint32_t i = 0; i < w->structures.count; i++) {
        draw_structure_aura(ctx, &w->structures, i);
    }
    for (uint32_t i = 0; i < w->walkers.count; i++) {
        draw_structure_aura(ctx, &w->walkers, i);
    }
    if (ctx->hover_x >= 0 && ctx->hover_y >= 0) {
        draw_aura_tile(ctx, ctx->hover_x, ctx->hover_y,
                       (SDL_Color){255, 255, 255, 42});
    }

    for (int layer = 0; layer <= WORLD_W + WORLD_H - 2; layer++) {
        for (int y = 0; y < WORLD_H; y++) {
            int x = layer - y;
            if (x < 0 || x >= WORLD_W) continue;

            for (uint32_t i = 0; i < w->structures.count; i++) {
                if (w->structures.x[i] == x && w->structures.y[i] == y) {
                    SDL_Color col;
                    switch (w->structures.type[i]) {
                        case STRUCT_STAGE:           col = (SDL_Color){210, 67, 214, 255}; break;
                        case STRUCT_BAR:             col = (SDL_Color){229, 194, 54, 255}; break;
                        case STRUCT_CHAPEL_PROTEST:  col = (SDL_Color){75, 76, 84, 255};   break;
                        case STRUCT_CORPORATE_BOOTH: col = (SDL_Color){64, 154, 221, 255}; break;
                        default:                     col = (SDL_Color){130, 130, 138, 255}; break;
                    }
                    draw_iso_block(ctx, x, y, 38, col);
                }
            }

            for (uint32_t i = 0; i < w->walkers.count; i++) {
                if (w->walkers.x[i] == x && w->walkers.y[i] == y) {
                    draw_walker(ctx, x, y);
                }
            }

            for (uint32_t i = 0; i < MAX_GUESTS; i++) {
                if (w->alive[i] && w->x[i] == x && w->y[i] == y) {
                    draw_guest(ctx, w, i);

                    uint8_t c = w->comfort[i];
                    if (c > 200) {
                        if (rand() % 10 == 0) spawn_particle(ctx, (float)x + 0.5f, (float)y + 0.5f, (SDL_Color){255, 255, 100, 255});
                    } else if (c < 50) {
                        if (rand() % 10 == 0) spawn_particle(ctx, (float)x + 0.5f, (float)y + 0.5f, (SDL_Color){100, 100, 100, 255});
                    }
                }
            }
        }
    }

    if (ctx->hover_x >= 0 && ctx->hover_y >= 0) {
        char reason[48];
        int can_place = game_can_place(g, w, (StructureType)g->selected_type,
                                       (int16_t)ctx->hover_x, (int16_t)ctx->hover_y,
                                       reason, sizeof reason);
        SDL_Color hover_color = can_place
            ? (SDL_Color){245, 250, 255, 235}
            : (SDL_Color){255, 68, 78, 235};
        draw_tile_outline(ctx, ctx->hover_x, ctx->hover_y, hover_color);
    }

    // Particles
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (ctx->particles[i].life > 0) {
            ctx->particles[i].x += ctx->particles[i].vx;
            ctx->particles[i].y += ctx->particles[i].vy;
            ctx->particles[i].life -= dt;
            
            int sx, sy;
            project(ctx, ctx->particles[i].x, ctx->particles[i].y, &sx, &sy);
            SDL_SetRenderDrawColor(ctx->renderer, ctx->particles[i].color.r, ctx->particles[i].color.g, 
                                    ctx->particles[i].color.b, (uint8_t)(ctx->particles[i].life * 255));
            SDL_Rect p = {sx, sy, 2, 2};
            SDL_RenderFillRect(ctx->renderer, &p);
        }
    }

    draw_hud(ctx, g);
    SDL_RenderPresent(ctx->renderer);
}

void render_cleanup(RenderContext *ctx) {
    if (ctx->renderer) SDL_DestroyRenderer(ctx->renderer);
    if (ctx->window) SDL_DestroyWindow(ctx->window);
    SDL_Quit();
}
