#ifndef LGBTYCOON_STRUCTURE_H
#define LGBTYCOON_STRUCTURE_H

#include <stdint.h>

typedef enum {
    STRUCT_STAGE           = 0,
    STRUCT_BAR             = 1,
    STRUCT_CHAPEL_PROTEST  = 2,
    STRUCT_CORPORATE_BOOTH = 3,
    STRUCT_LINUS           = 4,
    STRUCT_TYPE_COUNT      = 5,
} StructureType;

#define MAX_STRUCTURES 64

typedef struct {
    int16_t       x[MAX_STRUCTURES];
    int16_t       y[MAX_STRUCTURES];
    uint8_t       type[MAX_STRUCTURES];
    uint8_t       radius[MAX_STRUCTURES];
    uint32_t      count;
} Structures;

// Walkers reuse the Structures pool layout. The semantic difference is that
// walker positions are updated each tick by world_step_walkers; otherwise
// they're queried identically (same affinity table, same env_delta_for).
typedef Structures Walkers;

int  structures_place(Structures *s, StructureType type,
                      int16_t x, int16_t y, uint8_t radius);

// Sum of per-tick comfort delta from every in-range structure for a guest
// with the given identity bitmask at (gx, gy). May be negative.
int  structures_env_delta_for(const Structures *s,
                              int16_t gx, int16_t gy, uint8_t identity);

const char *structure_type_name(StructureType t);

// Returns the 8-byte per-identity-bit delta row for a structure type, or
// NULL on out-of-range. Exposed so walker logic can score positions
// against guests using the same table without duplicating the data.
const int8_t *structure_affinity_row(StructureType t);

// Pure flavor: returns one of Linus's grievances, deterministically
// selected by `key` so a (walker, target, tick) triple maps stably to a
// quip while varying with any of those inputs.
const char *linus_pick_quip(uint32_t key);

#endif
