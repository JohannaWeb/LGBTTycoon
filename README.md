# LGBTycoon

LGBTycoon is a small Pride-festival simulation game prototype in the RollerCoaster Tycoon mold. The simulation is written in C with a Structure-of-Arrays `World`, a thin SDL2 window/input/rendering layer, and a small x86_64 NASM SIMD pass for per-tick guest need decay.

The current prototype has an isometric SDL renderer, basic resource-gated placement, guest movement, guest arrivals/departures, hostile events, and a HUD. The long-term goal is a narrative win condition: build a thriving sovereign queer community, not maximize profit.

## Requirements

- GCC or a compatible C17 compiler
- CMake 3.20+
- NASM 2.16+
- SDL2 development headers

On Debian/Ubuntu:

```bash
sudo apt install build-essential cmake nasm libsdl2-dev
```

## Build And Run

Using CMake:

```bash
cmake -S . -B build
cmake --build build
./build/lgbtycoon
```

Using the Makefile:

```bash
make
./lgbtycoon
```

## Controls

- `1`: select stage
- `2`: select bar
- `3`: select chapel protest
- `4`: select corporate booth
- `5`: select Linus
- Left click: place the selected structure if placement rules allow it
- Right click: spawn a guest in debug mode
- `G`: spawn a guest on the hovered tile in debug mode
- `D`: toggle debug placement
- `Space`: pause or unpause
- `Esc`: quit

## Architecture Notes

- Guest state is stored as parallel arrays in `World`; there is no `Guest` struct.
- `Walkers` is a typedef of `Structures`, so mobile antagonists reuse the same pool layout and affinity table.
- `world_env_delta` is the single source of truth for tile comfort effects.
- The fixed tick order is SIMD need decay, walker movement, guest seek, then comfort application.
- SIMD passes run over `MAX_GUESTS`, not `guest_count`, and rely on saturating arithmetic.
- SDL2 is intended to stay a thin window/input/rendering shim.

## License

This project is licensed under the Mozilla Public License Version 2.0. See [LICENSE.md](LICENSE.md).
