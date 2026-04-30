# LGBTycoon Implementation Plan

Manual checklist for turning the current sim/render prototype into a playable game.

## Phase 1: Core Game Loop

- [ ] Add `game.h` / `game.c` with a `GameState` struct.
- [ ] Move pause state, selected placement type, tick interval, and placement rules out of `main.c` into `GameState`.
- [ ] Add resources to `GameState`:
  - [ ] `community_trust`
  - [ ] `volunteers`
  - [ ] `funding`
  - [ ] `safety`
  - [ ] `noise_pressure`
- [ ] Add win/loss state enum:
  - [ ] `GAME_RUNNING`
  - [ ] `GAME_WON`
  - [ ] `GAME_LOST`
- [ ] Add `game_init(GameState *g, World *w)`.
- [ ] Add `game_tick(GameState *g, World *w)`.
- [ ] Add `game_try_place_structure(GameState *g, World *w, StructureType type, int16_t x, int16_t y)`.
- [ ] Add `game_try_spawn_guest(GameState *g, World *w, int16_t x, int16_t y)`.

## Phase 2: Placement Costs And Constraints

- [ ] Define per-structure cost data.
- [ ] Stage costs volunteers and funding.
- [ ] Bar costs funding and increases noise pressure.
- [ ] Corporate booth grants funding but reduces community trust.
- [ ] Chapel protest is not player-placeable by default.
- [ ] Linus is not free player-placeable by default, or move him to event spawning.
- [ ] Prevent placement when resources are insufficient.
- [ ] Prevent placing multiple structures on the same tile.
- [ ] Add a placement cooldown.
- [ ] Add max structure count pressure through maintenance costs or scarcity.

## Phase 3: Guest Lifecycle

- [ ] Add per-guest low-comfort tracking field to `World` as another SoA array.
- [ ] Guests leave after several ticks below a comfort threshold.
- [ ] Guests with high comfort contribute to community trust.
- [ ] Guest arrivals happen over time instead of only manual spawn.
- [ ] Arrival rate scales with community trust and average comfort.
- [ ] Guest identities are selected from weighted tables instead of fixed cycling.
- [ ] Add a small random spawn edge/entrance system.

## Phase 4: Objectives

- [ ] Define the main win condition.
- [ ] Suggested win: survive 30 days with high trust and average comfort.
- [ ] Add loss condition for community trust collapse.
- [ ] Add loss condition for safety collapse.
- [ ] Add soft failure when too many guests leave.
- [ ] Display current objective progress in the HUD.
- [ ] Stop automatic ticks when won/lost.
- [ ] Allow restart from won/lost state.

## Phase 5: HUD And Feedback

- [ ] Add renderer HUD drawing.
- [ ] Show day/tick.
- [ ] Show guest count.
- [ ] Show average comfort.
- [ ] Show community trust.
- [ ] Show volunteers.
- [ ] Show funding.
- [ ] Show safety.
- [ ] Show noise pressure.
- [ ] Show selected placement type.
- [ ] Show whether the game is paused.
- [ ] Show win/loss banner.
- [ ] Add tile hover readout:
  - [ ] tile coordinates
  - [ ] structure on tile
  - [ ] guest count on tile
  - [ ] projected placement cost
  - [ ] whether placement is allowed
- [ ] Use color feedback for allowed vs blocked placement.

## Phase 6: Input Cleanup

- [ ] Move input interpretation into a small input handler.
- [ ] Keep SDL polling in `main.c`, but dispatch to game/input functions.
- [ ] Add number-key structure selection.
- [ ] Add left-click placement.
- [ ] Add right-click guest inspection or guest spawn only in debug mode.
- [ ] Add pause/unpause.
- [ ] Add speed controls:
  - [ ] normal
  - [ ] fast
  - [ ] single-step while paused
- [ ] Add restart key.
- [ ] Add debug toggle for free placement and guest spawning.

## Phase 7: Events And Antagonists

- [ ] Add event scheduler to `GameState`.
- [ ] Spawn chapel protest events from map edges or target tiles.
- [ ] Spawn Linus through events rather than initial setup only.
- [ ] Add protest surge event.
- [ ] Add grant opportunity event.
- [ ] Add committee interference event.
- [ ] Add mutual aid boost event.
- [ ] Add weather/noise event.
- [ ] Give each event a clear gameplay tradeoff.
- [ ] Show active events in the HUD.
- [ ] Add event log messages.

## Phase 8: Structure Design

- [ ] Revisit affinity values once resources and objectives exist.
- [ ] Add at least two new supportive structures.
- [ ] Add at least one defensive/safety structure.
- [ ] Add at least one trust-building structure.
- [ ] Add at least one controversial funding structure.
- [ ] Ensure every structure has:
  - [ ] cost
  - [ ] radius
  - [ ] affinity row
  - [ ] resource effect
  - [ ] visual color
  - [ ] readable gameplay purpose

## Phase 9: Simulation Balance

- [ ] Tune stuck movement threshold.
- [ ] Tune comfort gain/loss rates.
- [ ] Tune guest departure threshold.
- [ ] Tune guest arrival rate.
- [ ] Tune resource income.
- [ ] Tune structure costs.
- [ ] Tune event frequency.
- [ ] Tune win/loss thresholds.
- [ ] Add debug print summary for balance testing.
- [ ] Add deterministic seed option for repeatable runs.

## Phase 10: Rendering Improvements

- [ ] Draw distinct silhouettes for each structure type.
- [ ] Draw distinct walker sprites.
- [ ] Add simple guest identity color accents.
- [ ] Add comfort state animation or particles.
- [ ] Add placement preview ghost.
- [ ] Add blocked placement visual.
- [ ] Add map edge/entrance markers.
- [ ] Add active event visual markers.
- [ ] Keep SDL2 as the thin renderer/input shim.
- [ ] Do not introduce OpenGL or a new engine.

## Phase 11: Tests And Smoke Checks

- [ ] Add unit-style tests for structure placement rules.
- [ ] Add tests for resource cost application.
- [ ] Add tests for win/loss conditions.
- [ ] Add tests for guest departure behavior.
- [ ] Add tests for event scheduling.
- [ ] Keep SIMD decay behavior unchanged.
- [ ] Verify CMake build.
- [ ] Verify Makefile build.
- [ ] Verify `SDL_VIDEODRIVER=dummy timeout 2s ./build/lgbtycoon`.

## Phase 12: First Playable Milestone

- [ ] Player starts with limited resources.
- [ ] Guests arrive automatically.
- [ ] Player places supportive structures.
- [ ] Hostile events appear over time.
- [ ] Guests move, gain comfort, lose comfort, and leave.
- [ ] Resources change because of player choices.
- [ ] HUD explains current state.
- [ ] Game can be won.
- [ ] Game can be lost.
- [ ] A 5-minute run produces meaningful decisions.
