#include "entity.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

extern void tick_decay_simd(uint8_t *energy, uint8_t *hunger, size_t n);

static void identity_str(uint8_t id, char *out, size_t n);

// Total per-tick comfort delta a guest at (gx, gy) feels from BOTH static
// structures AND walker auras. Single source of truth used by guest seek
// and by the comfort apply pass.
static int world_env_delta(const World *w, int16_t gx, int16_t gy,
                           uint8_t identity) {
    return structures_env_delta_for(&w->structures, gx, gy, identity)
         + structures_env_delta_for(&w->walkers,    gx, gy, identity);
}

static uint32_t world_hash(uint64_t tick, uint32_t guest_id) {
    uint32_t x = (uint32_t)tick ^ (guest_id * 0x9e3779b9u);
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

static int valid_neighbor_count(int16_t cx, int16_t cy, int16_t out[8][2]) {
    int n = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int16_t nx = (int16_t)(cx + dx), ny = (int16_t)(cy + dy);
            if (nx < 0 || nx >= WORLD_W) continue;
            if (ny < 0 || ny >= WORLD_H) continue;
            out[n][0] = nx;
            out[n][1] = ny;
            n++;
        }
    }
    return n;
}

// One-tile greedy seek: each guest evaluates their current tile and the 8
// neighbors, picks the tile with the highest projected env delta, and
// steps there. If that produces several no-move ticks in a row, the guest
// takes one deterministic exploration step to escape flat bands/saddles.
static void world_seek(World *w) {
    for (uint32_t i = 0; i < w->guest_count; i++) {
        if (!w->alive[i]) continue;
        int16_t cx = w->x[i], cy = w->y[i];
        uint8_t id = w->identity[i];
        int best = world_env_delta(w, cx, cy, id);
        int16_t bx = cx, by = cy;

        int16_t neighbors[8][2];
        int neighbor_count = valid_neighbor_count(cx, cy, neighbors);
        for (int n = 0; n < neighbor_count; n++) {
            int16_t nx = neighbors[n][0], ny = neighbors[n][1];
            int v = world_env_delta(w, nx, ny, id);
            if (v > best) { best = v; bx = nx; by = ny; }
        }

        if (bx == cx && by == cy) {
            if (w->stuck_ticks[i] < 255) w->stuck_ticks[i]++;
            if (w->stuck_ticks[i] >= 3 && neighbor_count > 0) {
                uint32_t pick = world_hash(w->tick, w->id[i]) % (uint32_t)neighbor_count;
                bx = neighbors[pick][0];
                by = neighbors[pick][1];
                w->stuck_ticks[i] = 0;
            }
        } else {
            w->stuck_ticks[i] = 0;
        }

        w->x[i] = bx;
        w->y[i] = by;
    }
}

// Score a candidate walker tile by summing the walker's per-bit affinity
// over every alive guest within its Chebyshev radius. Lower (more
// negative) = more aggravation delivered = better for the antagonist.
static int walker_score_at(const World *w, int16_t wx, int16_t wy,
                           StructureType type, uint8_t radius) {
    const int8_t *row = structure_affinity_row(type);
    if (!row) return 0;
    int total = 0;
    for (uint32_t i = 0; i < w->guest_count; i++) {
        if (!w->alive[i]) continue;
        int dx = w->x[i] - wx; if (dx < 0) dx = -dx;
        int dy = w->y[i] - wy; if (dy < 0) dy = -dy;
        if (dx > radius || dy > radius) continue;
        uint8_t id = w->identity[i];
        for (int b = 0; b < 8; b++) {
            if (id & (1u << b)) total += row[b];
        }
    }
    return total;
}

// Linus has two modes: stalk (move to the tile that hurts the most guests
// most) and roam (no targets in range — walk one step toward the alive-
// guest centroid). Roam is what gets him out of empty-quadrant starts;
// without it the stalk gradient is flat and he never moves.
static void world_step_walkers(World *w) {
    for (uint32_t k = 0; k < w->walkers.count; k++) {
        StructureType t = (StructureType)w->walkers.type[k];
        int16_t cx = w->walkers.x[k], cy = w->walkers.y[k];
        uint8_t r  = w->walkers.radius[k];

        int best = walker_score_at(w, cx, cy, t, r);
        int16_t bx = cx, by = cy;
        for (int dy = -1; dy <= 1; dy++) {

            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                int16_t nx = (int16_t)(cx + dx), ny = (int16_t)(cy + dy);
                if (nx < 0 || nx >= WORLD_W) continue;
                if (ny < 0 || ny >= WORLD_H) continue;
                int v = walker_score_at(w, nx, ny, t, r);
                if (v < best) { best = v; bx = nx; by = ny; }
            }
        }

        if (bx == cx && by == cy && best == 0 && w->guest_count > 0) {
            int sx = 0, sy = 0, n = 0;
            for (uint32_t i = 0; i < w->guest_count; i++) {
                if (!w->alive[i]) continue;
                sx += w->x[i]; sy += w->y[i]; n++;
            }
            if (n > 0) {
                int tx = sx / n, ty = sy / n;
                int sxd = (tx > cx) - (tx < cx);
                int syd = (ty > cy) - (ty < cy);
                int16_t nx = (int16_t)(cx + sxd);
                int16_t ny = (int16_t)(cy + syd);
                if (nx >= 0 && nx < WORLD_W) bx = nx;
                if (ny >= 0 && ny < WORLD_H) by = ny;
            }
        }

        w->walkers.x[k] = bx;
        w->walkers.y[k] = by;

        // Flavor: find the in-range guest most aggrieved by this walker's
        // aura (most-negative per-bit affinity sum) and print a quip.
        // No state mutation — purely cosmetic. A walker with no aggrieved
        // guests in range stays silent; ideological allies (positive sum)
        // never trigger chatter even when in range.
        if (t == STRUCT_LINUS) {
            const int8_t *row = structure_affinity_row(t);
            int worst = 0;
            int32_t target = -1;
            for (uint32_t i = 0; i < w->guest_count; i++) {
                if (!w->alive[i]) continue;
                int dx = w->x[i] - bx; if (dx < 0) dx = -dx;
                int dy = w->y[i] - by; if (dy < 0) dy = -dy;
                if (dx > r || dy > r) continue;
                int s = 0;
                for (int b = 0; b < 8; b++) {
                    if (w->identity[i] & (1u << b)) s += row[b];
                }
                if (s < worst) { worst = s; target = (int32_t)i; }
            }
            if (target >= 0) {
                char idstr[32];
                identity_str(w->identity[target], idstr, sizeof(idstr));
                uint32_t key = (uint32_t)(k * 31u
                                          + (uint32_t)target * 17u
                                          + (uint32_t)w->tick);
                printf("  Linus -> #%u [%s]: \"%s\"\n",
                       w->id[target], idstr, linus_pick_quip(key));
            }
        }
    }
}

void world_init(World *w) {
    memset(w, 0, sizeof(*w));
}

void world_spawn_guest(World *w, uint8_t identity, int16_t x, int16_t y) {
    if (w->guest_count >= MAX_GUESTS) return;
    uint32_t i = w->guest_count;
    w->id[i]       = i;
    w->identity[i] = identity;
    w->energy[i]   = 200;
    w->hunger[i]   = 40;
    w->comfort[i]  = 180;
    w->x[i]        = x;
    w->y[i]        = y;
    w->alive[i]    = 1;
    w->stuck_ticks[i] = 0;
    w->guest_count++;
}

void world_tick(World *w) {
    // Runs over the full MAX_GUESTS, not guest_count: dead/unspawned slots
    // start at 0, psubusb pins them there, paddusb saturates at 255 — both
    // invisible because reads gate on alive[].
    tick_decay_simd(w->energy, w->hunger, MAX_GUESTS);

    // Order matters: walkers move first (deciding where to threaten),
    // then guests respond to the new walker positions, then comfort is
    // applied at the post-move positions of both.
    world_step_walkers(w);
    world_seek(w);

    for (uint32_t i = 0; i < w->guest_count; i++) {
        if (!w->alive[i]) continue;
        int delta = world_env_delta(w, w->x[i], w->y[i], w->identity[i]);
        int c = (int)w->comfort[i] + delta;
        if (c < 0)        c = 0;
        else if (c > 255) c = 255;
        w->comfort[i] = (uint8_t)c;
    }
    w->tick++;
}

static void identity_str(uint8_t id, char *out, size_t n) {
    static const char letters[8] = {'L','G','B','T','Q','A','I','N'};
    size_t pos = 0;
    for (int i = 0; i < 8; i++) {
        if (!(id & (1 << i))) continue;
        if (pos > 0 && pos + 1 < n) out[pos++] = '+';
        if (pos + 1 < n)            out[pos++] = letters[i];
    }
    if (pos == 0 && n > 1) out[pos++] = '?';
    if (pos < n) out[pos] = '\0';
    else if (n > 0) out[n - 1] = '\0';
}

void world_debug_print(const World *w) {
    printf("=== tick %" PRIu64 " | %u guests | %u structures | %u walkers ===\n",
           w->tick, w->guest_count, w->structures.count, w->walkers.count);
    for (uint32_t k = 0; k < w->walkers.count; k++) {
        printf("  [%s] @(%d,%d) r=%u\n",
               structure_type_name((StructureType)w->walkers.type[k]),
               w->walkers.x[k], w->walkers.y[k], w->walkers.radius[k]);
    }
    for (uint32_t i = 0; i < w->guest_count; i++) {
        if (!w->alive[i]) continue;
        char id[32];
        identity_str(w->identity[i], id, sizeof(id));
        printf("  #%u [%s] @(%d,%d) energy=%u hunger=%u comfort=%u\n",
               w->id[i], id, w->x[i], w->y[i],
               w->energy[i], w->hunger[i], w->comfort[i]);
    }
}
