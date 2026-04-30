#ifndef LGBTYCOON_ENTITY_H
#define LGBTYCOON_ENTITY_H

#include <stdint.h>
#include <stddef.h>
#include "structure.h"

typedef enum {
    ID_LESBIAN   = 1 << 0,
    ID_GAY       = 1 << 1,
    ID_BI        = 1 << 2,
    ID_TRANS     = 1 << 3,
    ID_QUEER     = 1 << 4,
    ID_ACE       = 1 << 5,
    ID_INTERSEX  = 1 << 6,
    ID_NONBINARY = 1 << 7,
} IdentityFlag;

#define MAX_GUESTS 256
#define WORLD_W    16
#define WORLD_H    16

typedef struct {
    uint32_t id[MAX_GUESTS];
    uint8_t  identity[MAX_GUESTS];
    _Alignas(16) uint8_t energy[MAX_GUESTS];
    _Alignas(16) uint8_t hunger[MAX_GUESTS];
    uint8_t  comfort[MAX_GUESTS];
    int16_t  x[MAX_GUESTS];
    int16_t  y[MAX_GUESTS];
    uint8_t  alive[MAX_GUESTS];
    uint8_t  stuck_ticks[MAX_GUESTS];
    uint32_t guest_count;
    uint64_t tick;
    Structures structures;
    Walkers    walkers;
} World;

void world_init(World *w);
void world_spawn_guest(World *w, uint8_t identity, int16_t x, int16_t y);
void world_tick(World *w);
void world_debug_print(const World *w);

#endif
