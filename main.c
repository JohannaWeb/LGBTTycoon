#include "entity.h"
#include "game.h"
#include "render.h"
#include <stdio.h>
#include <SDL2/SDL.h>

static void update_title(RenderContext *ctx, const GameState *g) {
    char title[128];
    snprintf(title, sizeof title, "LGBTycoon | %s | %s%s%s",
             structure_type_name((StructureType)g->selected_type),
             game_status_text(g->status),
             g->paused ? " | paused" : "",
             g->debug_free_place ? " | debug" : "");
    SDL_SetWindowTitle(ctx->window, title);
}

int main(void) {
    World w;
    world_init(&w);

    RenderContext ctx;
    if (render_init(&ctx) < 0) {
        return 1;
    }

    // The grounds.
    structures_place(&w.structures, STRUCT_STAGE,           5, 5, 3);
    structures_place(&w.structures, STRUCT_BAR,             2, 2, 2);
    structures_place(&w.structures, STRUCT_CHAPEL_PROTEST,  8, 8, 3);
    structures_place(&w.structures, STRUCT_CORPORATE_BOOTH, 5, 2, 2);

    // Linus arrives from the east edge of the grounds.
    structures_place(&w.walkers, STRUCT_LINUS, 12, 5, 2);

    // Guests positioned to feel different things
    world_spawn_guest(&w, ID_LESBIAN | ID_TRANS,    5, 5);
    world_spawn_guest(&w, ID_GAY,                   2, 2);
    world_spawn_guest(&w, ID_BI | ID_NONBINARY,     8, 8);
    world_spawn_guest(&w, ID_ACE,                   5, 2);
    world_spawn_guest(&w, ID_QUEER | ID_INTERSEX,   4, 4);

    GameState game;
    game_init(&game, &w);

    int quit = 0;
    SDL_Event e;
    uint32_t last_tick_time = SDL_GetTicks();
    uint32_t tick_interval = 500; // ms
    update_title(&ctx, &game);

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = 1;
            } else if (e.type == SDL_MOUSEMOTION) {
                int16_t tx, ty;
                if (render_screen_to_tile(&ctx, e.motion.x, e.motion.y, &tx, &ty)) {
                    ctx.hover_x = tx;
                    ctx.hover_y = ty;
                } else {
                    ctx.hover_x = -1;
                    ctx.hover_y = -1;
                }
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                int16_t tx, ty;
                if (render_screen_to_tile(&ctx, e.button.x, e.button.y, &tx, &ty)) {
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        game_try_place_structure(&game, &w, (StructureType)game.selected_type, tx, ty);
                        update_title(&ctx, &game);
                    } else if (e.button.button == SDL_BUTTON_RIGHT) {
                        game_try_spawn_guest(&game, &w, tx, ty);
                    }
                }
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        quit = 1;
                        break;
                    case SDLK_SPACE:
                        game_toggle_pause(&game);
                        update_title(&ctx, &game);
                        break;
                    case SDLK_1:
                        game_select(&game, STRUCT_STAGE);
                        update_title(&ctx, &game);
                        break;
                    case SDLK_2:
                        game_select(&game, STRUCT_BAR);
                        update_title(&ctx, &game);
                        break;
                    case SDLK_3:
                        game_select(&game, STRUCT_CHAPEL_PROTEST);
                        update_title(&ctx, &game);
                        break;
                    case SDLK_4:
                        game_select(&game, STRUCT_CORPORATE_BOOTH);
                        update_title(&ctx, &game);
                        break;
                    case SDLK_5:
                        game_select(&game, STRUCT_LINUS);
                        update_title(&ctx, &game);
                        break;
                    case SDLK_d:
                        game_toggle_debug(&game);
                        update_title(&ctx, &game);
                        break;
                    case SDLK_g:
                        if (ctx.hover_x >= 0 && ctx.hover_y >= 0) {
                            game_try_spawn_guest(&game, &w, (int16_t)ctx.hover_x, (int16_t)ctx.hover_y);
                        }
                        break;
                }
            }
        }

        uint32_t current_time = SDL_GetTicks();
        if (!game.paused && current_time - last_tick_time >= tick_interval) {
            game_tick(&game, &w);
            last_tick_time = current_time;
            // world_debug_print(&w);
            update_title(&ctx, &game);
        } else if (game.paused) {
            last_tick_time = current_time;
        }

        render_present(&ctx, &w, &game);
        SDL_Delay(16); // ~60fps for rendering
    }

    render_cleanup(&ctx);
    return 0;
}
