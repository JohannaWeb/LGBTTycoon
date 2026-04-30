#include "structure.h"
#include <stdlib.h>

// Rows: StructureType. Columns: identity bit position (L,G,B,T,Q,A,I,N).
// Per-tick comfort delta a guest in range gets, summed over each set
// identity bit. Multi-flag guests respond more strongly — narratively a
// guest with more salient queer identities has more skin in the game.
static const int8_t affinity[STRUCT_TYPE_COUNT][8] = {
    //                  L   G   B   T   Q   A   I   N
    [STRUCT_STAGE]           = { +1, +1, +1, +2, +2, +1, +1, +2 },
    [STRUCT_BAR]             = { +2, +2, +2, +1,  0, -1,  0, +1 },
    [STRUCT_CHAPEL_PROTEST]  = { -2, -2, -2, -3, -2, -1, -2, -3 },
    [STRUCT_CORPORATE_BOOTH] = { +1, +2,  0, -1, -2, +1,  0, -2 },
    // Linus is a leftist anti-pinkwashing protester — annoying to most
    // mainstream-coded folks, mildly *welcome* to queer/NB who agree with
    // his critique. Drives him to seek L/G/B/A clusters and avoid his
    // ideological allies.
    [STRUCT_LINUS]           = { -2, -2, -1, -1, +1, -1,  0, +1 },
};

static const char *type_names[STRUCT_TYPE_COUNT] = {
    [STRUCT_STAGE]           = "STAGE",
    [STRUCT_BAR]             = "BAR",
    [STRUCT_CHAPEL_PROTEST]  = "CHAPEL_PROTEST",
    [STRUCT_CORPORATE_BOOTH] = "CORPORATE_BOOTH",
    [STRUCT_LINUS]           = "LINUS",
};

const char *structure_type_name(StructureType t) {
    if ((unsigned)t >= STRUCT_TYPE_COUNT) return "?";
    return type_names[t];
}

const int8_t *structure_affinity_row(StructureType t) {
    if ((unsigned)t >= STRUCT_TYPE_COUNT) return NULL;
    return affinity[t];
}

static const char *linus_quips[] = {
    "your rainbow lanyard is corporate-sponsored slop. NAK.",
    "this Stonewall reenactment is ahistorical garbage.",
    "your pronoun pin has a merge conflict.",
    "the trans flag stripe order is wrong. Did you even READ the spec?",
    "this drag brunch is a regression. Reverting.",
    "your bunting choice is utter garbage.",
    "I refuse to participate in this assimilationist circus.",
    "send a v2 with actual queer politics next time.",
    "this whole festival is rebased on a sponsorship deal. Sad.",
    "I'm filing a bug against your concept of liberation.",
    "your celebration has no test coverage.",
    "this should have been an email.",
    "did capitalism write this? Reads like capitalism wrote this.",
    "have you considered: not.",
    "the spec for being queer is in /usr/share/doc/queer.7. READ IT.",
    "this is the worst patch I have seen all month.",
};
static const size_t LINUS_QUIPS_N =
    sizeof linus_quips / sizeof linus_quips[0];

const char *linus_pick_quip(uint32_t key) {
    return linus_quips[key % LINUS_QUIPS_N];
}

int structures_place(Structures *s, StructureType type,
                     int16_t x, int16_t y, uint8_t radius) {
    if (s->count >= MAX_STRUCTURES) return -1;
    if ((unsigned)type >= STRUCT_TYPE_COUNT) return -1;
    uint32_t i = s->count;
    s->x[i]      = x;
    s->y[i]      = y;
    s->type[i]   = (uint8_t)type;
    s->radius[i] = radius;
    s->count++;
    return (int)i;
}

static inline int abs_i16(int16_t v) { return v < 0 ? -v : v; }

int structures_env_delta_for(const Structures *s,
                             int16_t gx, int16_t gy, uint8_t identity) {
    int total = 0;
    for (uint32_t k = 0; k < s->count; k++) {
        int dx = abs_i16((int16_t)(gx - s->x[k]));
        int dy = abs_i16((int16_t)(gy - s->y[k]));
        int r  = s->radius[k];
        if (dx > r || dy > r) continue;  // Chebyshev / square footprint
        const int8_t *row = affinity[s->type[k]];
        for (int b = 0; b < 8; b++) {
            if (identity & (1u << b)) total += row[b];
        }
    }
    return total;
}
