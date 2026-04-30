#include "game.h"
#include <stdio.h>
#include <string.h>

#define WIN_DAY 30u
#define WIN_TRUST 100
#define MIN_AVG_COMFORT 145
#define LOSS_TRUST 0
#define LOSS_SAFETY 0
#define LOW_COMFORT_LIMIT 6

static uint8_t low_comfort_ticks[MAX_GUESTS];

static uint8_t default_radius(StructureType type) {
    switch (type) {
        case STRUCT_BAR: return 2;
        case STRUCT_LINUS: return 2;
        default: return 3;
    }
}

static uint8_t guest_identity_for(uint64_t key) {
    static const uint8_t ids[] = {
        ID_LESBIAN | ID_TRANS,
        ID_GAY,
        ID_BI | ID_NONBINARY,
        ID_ACE,
        ID_QUEER | ID_INTERSEX,
        ID_TRANS | ID_NONBINARY,
        ID_LESBIAN | ID_QUEER,
        ID_GAY | ID_ACE,
    };
    return ids[key % (sizeof ids / sizeof ids[0])];
}

static void set_message(GameState *g, const char *message) {
    snprintf(g->message, sizeof g->message, "%s", message);
}

static int tile_has_structure(const World *w, int16_t x, int16_t y) {
    for (uint32_t i = 0; i < w->structures.count; i++) {
        if (w->structures.x[i] == x && w->structures.y[i] == y) return 1;
    }
    for (uint32_t i = 0; i < w->walkers.count; i++) {
        if (w->walkers.x[i] == x && w->walkers.y[i] == y) return 1;
    }
    return 0;
}

static void update_metrics(GameState *g, const World *w) {
    int total_comfort = 0;
    int alive = 0;
    for (uint32_t i = 0; i < w->guest_count; i++) {
        if (!w->alive[i]) continue;
        total_comfort += w->comfort[i];
        alive++;
    }
    g->alive_guests = alive;
    g->average_comfort = alive > 0 ? total_comfort / alive : 0;
}

static void remove_guest(GameState *g, World *w, uint32_t i) {
    w->alive[i] = 0;
    w->stuck_ticks[i] = 0;
    low_comfort_ticks[i] = 0;
    g->guests_lost++;
}

static void apply_guest_lifecycle(GameState *g, World *w) {
    for (uint32_t i = 0; i < w->guest_count; i++) {
        if (!w->alive[i]) continue;
        if (w->comfort[i] < 45) {
            if (low_comfort_ticks[i] < 255) low_comfort_ticks[i]++;
            if (low_comfort_ticks[i] >= LOW_COMFORT_LIMIT) {
                remove_guest(g, w, i);
                set_message(g, "A guest left after too many miserable ticks.");
            }
        } else {
            low_comfort_ticks[i] = 0;
        }
    }
}

static void spawn_arrival(GameState *g, World *w) {
    static const int16_t entrances[][2] = {
        {0, 3}, {0, 12}, {15, 4}, {15, 12}, {4, 0}, {12, 15},
    };
    uint64_t key = w->tick + (uint64_t)g->community_trust + (uint64_t)g->alive_guests * 13u;
    uint32_t pick = (uint32_t)(key % (sizeof entrances / sizeof entrances[0]));
    world_spawn_guest(w, guest_identity_for(key), entrances[pick][0], entrances[pick][1]);
}

static void maybe_spawn_event(GameState *g, World *w) {
    if (g->event_timer > 0) {
        g->event_timer--;
        return;
    }

    if (w->structures.count < MAX_STRUCTURES) {
        int16_t x = (int16_t)(2 + (w->tick * 5u) % (WORLD_W - 4));
        int16_t y = (int16_t)(2 + (w->tick * 3u) % (WORLD_H - 4));
        if (!tile_has_structure(w, x, y)) {
            structures_place(&w->structures, STRUCT_CHAPEL_PROTEST, x, y, 3);
            g->safety -= 6;
            g->community_trust -= 3;
            set_message(g, "A protest group claimed space on the grounds.");
        }
    }
    g->event_timer = 10u + (uint32_t)(w->tick % 8u);
}

int game_structure_funding_cost(StructureType type) {
    switch (type) {
        case STRUCT_STAGE: return 35;
        case STRUCT_BAR: return 25;
        case STRUCT_CORPORATE_BOOTH: return -45;
        case STRUCT_LINUS: return 0;
        case STRUCT_CHAPEL_PROTEST: return 0;
        default: return 20;
    }
}

int game_structure_volunteer_cost(StructureType type) {
    switch (type) {
        case STRUCT_STAGE: return 3;
        case STRUCT_BAR: return 1;
        case STRUCT_CORPORATE_BOOTH: return 1;
        case STRUCT_LINUS: return 0;
        case STRUCT_CHAPEL_PROTEST: return 0;
        default: return 1;
    }
}

int game_structure_trust_delta(StructureType type) {
    switch (type) {
        case STRUCT_STAGE: return 4;
        case STRUCT_BAR: return 1;
        case STRUCT_CORPORATE_BOOTH: return -8;
        case STRUCT_LINUS: return -2;
        case STRUCT_CHAPEL_PROTEST: return -5;
        default: return 0;
    }
}

int game_structure_noise_delta(StructureType type) {
    switch (type) {
        case STRUCT_STAGE: return 4;
        case STRUCT_BAR: return 3;
        case STRUCT_CORPORATE_BOOTH: return 1;
        case STRUCT_LINUS: return 1;
        case STRUCT_CHAPEL_PROTEST: return 2;
        default: return 0;
    }
}

void game_init(GameState *g, World *w) {
    memset(g, 0, sizeof *g);
    memset(low_comfort_ticks, 0, sizeof low_comfort_ticks);
    g->funding = 100;
    g->volunteers = 8;
    g->community_trust = 55;
    g->safety = 80;
    g->noise_pressure = 0;
    g->selected_type = STRUCT_STAGE;
    g->status = GAME_RUNNING;
    g->arrival_timer = 3;
    g->event_timer = 8;
    set_message(g, "Build a thriving festival before trust or safety collapses.");
    update_metrics(g, w);
}

void game_select(GameState *g, StructureType type) {
    if ((unsigned)type >= STRUCT_TYPE_COUNT) return;
    g->selected_type = (uint8_t)type;
}

void game_toggle_pause(GameState *g) {
    g->paused = !g->paused;
}

void game_toggle_debug(GameState *g) {
    g->debug_free_place = !g->debug_free_place;
    set_message(g, g->debug_free_place ? "Debug placement enabled." : "Debug placement disabled.");
}

int game_can_place(const GameState *g, const World *w, StructureType type,
                   int16_t x, int16_t y, char *reason, size_t reason_size) {
    const char *why = NULL;
    if (g->status != GAME_RUNNING) why = "game over";
    else if (x < 0 || x >= WORLD_W || y < 0 || y >= WORLD_H) why = "outside map";
    else if (type == STRUCT_CHAPEL_PROTEST && !g->debug_free_place) why = "event only";
    else if (type == STRUCT_LINUS && !g->debug_free_place) why = "event only";
    else if (tile_has_structure(w, x, y)) why = "tile occupied";
    else if (g->placement_cooldown > 0 && !g->debug_free_place) why = "cooldown";
    else if (!g->debug_free_place && g->funding < game_structure_funding_cost(type)) why = "not enough funding";
    else if (!g->debug_free_place && g->volunteers < game_structure_volunteer_cost(type)) why = "not enough volunteers";

    if (why) {
        if (reason && reason_size > 0) snprintf(reason, reason_size, "%s", why);
        return 0;
    }
    if (reason && reason_size > 0) snprintf(reason, reason_size, "ok");
    return 1;
}

int game_try_place_structure(GameState *g, World *w, StructureType type,
                             int16_t x, int16_t y) {
    char reason[48];
    if (!game_can_place(g, w, type, x, y, reason, sizeof reason)) {
        char msg[96];
        snprintf(msg, sizeof msg, "Cannot place %s: %s.", structure_type_name(type), reason);
        set_message(g, msg);
        return 0;
    }

    int result;
    if (type == STRUCT_LINUS) {
        result = structures_place(&w->walkers, type, x, y, default_radius(type));
    } else {
        result = structures_place(&w->structures, type, x, y, default_radius(type));
    }
    if (result < 0) {
        set_message(g, "No structure slots left.");
        return 0;
    }

    if (!g->debug_free_place) {
        g->funding -= game_structure_funding_cost(type);
        g->volunteers -= game_structure_volunteer_cost(type);
        g->community_trust += game_structure_trust_delta(type);
        g->noise_pressure += game_structure_noise_delta(type);
        g->placement_cooldown = 2;
    }

    char msg[96];
    snprintf(msg, sizeof msg, "Placed %s.", structure_type_name(type));
    set_message(g, msg);
    return 1;
}

int game_try_spawn_guest(GameState *g, World *w, int16_t x, int16_t y) {
    if (!g->debug_free_place) {
        set_message(g, "Guest spawning is debug-only. Press D to toggle debug placement.");
        return 0;
    }
    if (x < 0 || x >= WORLD_W || y < 0 || y >= WORLD_H) return 0;
    world_spawn_guest(w, guest_identity_for(w->guest_count + w->tick), x, y);
    set_message(g, "Spawned debug guest.");
    return 1;
}

void game_tick(GameState *g, World *w) {
    if (g->status != GAME_RUNNING) return;

    world_tick(w);
    g->day = (uint32_t)w->tick;
    if (g->placement_cooldown > 0) g->placement_cooldown--;

    apply_guest_lifecycle(g, w);
    update_metrics(g, w);

    if (g->arrival_timer > 0) {
        g->arrival_timer--;
    } else {
        int target = 5 + g->community_trust / 12 + g->average_comfort / 45;
        if (g->alive_guests < target && w->guest_count < MAX_GUESTS) {
            spawn_arrival(g, w);
            set_message(g, "New guests arrived at the festival edge.");
        }
        g->arrival_timer = g->community_trust > 80 ? 2 : 4;
    }

    if (g->average_comfort >= 170 && g->alive_guests > 0) g->community_trust += 2;
    else if (g->average_comfort < 85) g->community_trust -= 3;

    if (g->noise_pressure > 35) g->safety -= 2;
    if (g->noise_pressure > 0 && (w->tick % 3u) == 0) g->noise_pressure--;
    if (g->volunteers < 12 && g->average_comfort > 155 && (w->tick % 5u) == 0) g->volunteers++;
    if ((w->tick % 4u) == 0) g->funding += 8 + g->alive_guests / 3;

    maybe_spawn_event(g, w);
    update_metrics(g, w);

    if (g->community_trust >= WIN_TRUST && g->day >= WIN_DAY && g->average_comfort >= MIN_AVG_COMFORT) {
        g->status = GAME_WON;
        set_message(g, "The festival has become a thriving sovereign queer community.");
    } else if (g->community_trust <= LOSS_TRUST) {
        g->status = GAME_LOST;
        set_message(g, "Community trust collapsed.");
    } else if (g->safety <= LOSS_SAFETY) {
        g->status = GAME_LOST;
        set_message(g, "Safety collapsed.");
    } else if (g->guests_lost >= 12) {
        g->status = GAME_LOST;
        set_message(g, "Too many guests left the grounds.");
    }
}

const char *game_status_text(GameStatus status) {
    switch (status) {
        case GAME_RUNNING: return "running";
        case GAME_WON: return "won";
        case GAME_LOST: return "lost";
        default: return "?";
    }
}
