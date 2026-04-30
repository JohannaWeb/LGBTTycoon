#ifndef LGBTYCOON_GAME_H
#define LGBTYCOON_GAME_H

#include "entity.h"

typedef enum {
    GAME_RUNNING = 0,
    GAME_WON,
    GAME_LOST,
} GameStatus;

typedef struct {
    int funding;
    int volunteers;
    int community_trust;
    int safety;
    int noise_pressure;
    int average_comfort;
    int alive_guests;
    int guests_lost;
    uint32_t day;
    uint32_t placement_cooldown;
    uint32_t arrival_timer;
    uint32_t event_timer;
    uint8_t selected_type;
    uint8_t paused;
    uint8_t debug_free_place;
    GameStatus status;
    char message[128];
} GameState;

void game_init(GameState *g, World *w);
void game_tick(GameState *g, World *w);

void game_select(GameState *g, StructureType type);
void game_toggle_pause(GameState *g);
void game_toggle_debug(GameState *g);

int  game_can_place(const GameState *g, const World *w, StructureType type,
                    int16_t x, int16_t y, char *reason, size_t reason_size);
int  game_try_place_structure(GameState *g, World *w, StructureType type,
                              int16_t x, int16_t y);
int  game_try_spawn_guest(GameState *g, World *w, int16_t x, int16_t y);

int  game_structure_funding_cost(StructureType type);
int  game_structure_volunteer_cost(StructureType type);
int  game_structure_trust_delta(StructureType type);
int  game_structure_noise_delta(StructureType type);
const char *game_status_text(GameStatus status);

#endif
