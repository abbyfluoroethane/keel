# keel

Keel is the software for **Vela**, an open-source music player in the spirit of
the Zune HD. This repository holds the first pass of the user interface: a demo
that runs on a Linux laptop and will move to the device with a new backend and
no UI rewrite.

- C11 and [LVGL](https://lvgl.io) v9.6, software renderer only (the device has no GPU).
- `ui/` holds the screens, widgets, theme and navigation. It has no SDL code.
- `platform/sdl/` holds the laptop backend: window, input, main loop,
  screenshots and the bench. It does not use LVGL's SDL driver; see
  "Performance" below.
- `ui/theme.c` holds every design token in one place. `KEEL_SCALE` in
  `ui/theme.h` (default 1.5) scales the design system for the ~233 ppi panel.

Target hardware: Rockchip RK3506B (3x Cortex-A7, 1.2 GHz, 512 MB) on a Luckfox
Core3506, 4" 480x800 IPS over MIPI DSI, GT911 touch, Buildroot Linux.

## Development environment: Fedora Silverblue toolbox

The host OS is immutable. Every install and build happens in a toolbox
container named `keel`. Never `rpm-ostree install` anything for Keel, and never
install build packages on the host.

Set up the toolbox (safe to run more than once):

```sh
tools/toolbox-setup.sh
```

It does these steps:

1. Check whether the toolbox exists: `toolbox list`.
2. If not, create it: `toolbox create keel`.
3. Install the build tools:
   `toolbox run -c keel sudo dnf install -y gcc gcc-c++ cmake ninja-build git pkgconf-pkg-config SDL2-devel freetype-devel python3 python3-pillow`

   (`python3-pillow` is only for `tools/fetch_demo_library.py`, which resizes
   cover art. It is not linked into Keel.)

Build and run from the host:

```sh
toolbox run -c keel cmake -B build -G Ninja
toolbox run -c keel cmake --build build
toolbox run -c keel ./build/keel
```

The toolbox shares your home directory and the Wayland socket, so the window
opens on the host desktop. The first configure downloads LVGL v9.6.0 into
`build/_deps/`.

### Demo cover art

The demo library is real: ten albums by Ninajirachi, Sewerslvt, Cynthoni and
100 gecs. The tracklists in `assets/demo/library.json` come from
[MusicBrainz](https://musicbrainz.org) (CC0). Cover art belongs to its owners,
so it is **not** in git. Fetch it once:

```sh
toolbox run -c keel python3 tools/fetch_demo_library.py covers
```

This downloads each front cover from the Cover Art Archive (or the iTunes
Search API when the archive has none) and writes four sizes to
`assets/demo/covers/`, which is gitignored. Without it, albums show as plain
squares. `tools/fetch_demo_library.py library` rebuilds the JSON from
MusicBrainz.

## Flags

| Flag | What it does |
|---|---|
| `--scale N` | Zoom the window N times (e.g. `--scale 2` on a laptop). Rendering stays 480x800. |
| `--fps` | Show frame time, worst frame and frames per second in the bottom-right corner, in the `data` style. |
| `--reduced-motion` | Turn off parallax, list stagger and slide animations. |
| `--screenshot DIR` | Render each demo screen once, write one PNG per screen to DIR, exit. Runs headless. |
| `--bench` | Drive scripted swipes, scrolls and button presses through a virtual touch pointer and keypad, print frame statistics per scenario, exit non-zero if a scenario misses 60 fps or a check fails. |

`KEEL_ASSETS=/path` overrides where fonts and demo data are loaded from.

## Input

Vela has three hardware buttons below the screen. On the laptop they are
arrow keys, and they mean the same thing on every screen:

| Button | Laptop key | Action |
|---|---|---|
| back (left) | ← | Go back one screen. Escape and Backspace also go back. |
| home (center) | ↓ | Return to the home screen. |
| menu (right) | → | Open the current screen's menu. Undecided; see [issue 0001](docs/issues/0001-menu-button.md). |

Everything else is touch. Mouse drag is one-finger touch: drag lists to
scroll, swipe pivots sideways, swipe the art on now playing to skip, drag
the radio's tuning strip. There is no on-screen back control; going back is
the back button's job. The status bar shows Wi-Fi and Bluetooth on the left
(hidden while that radio is off), the clock in the middle and the battery on
the right.

Keyboard focus, for testing without a mouse: ↑ and Shift+Tab move focus
back, Tab moves it forward, Enter or Space activates.

## Performance

`--bench` measures each frame as LVGL render plus flush. The pass rule is
99% of frames inside one 60 Hz period (16.7 ms) and under 1% over it; the
max column shows stray long frames. On the development laptop (i7-1180G7,
balanced profile), a typical frame takes 1-2 ms, of which the flush is
under 1 ms.

LVGL's own SDL driver presents every frame by alpha-blending a full-screen
texture through SDL's renderer. That cost about 11 ms per frame, nearly all
of the frame, and has no equivalent on the device. `platform/sdl/sdl_port.c`
replaces it: LVGL renders into one full-frame buffer, and flush copies only
the changed rectangles into the window surface, the way a DRM dumb-buffer
flush will on the device. LVGL itself is built without SDL.

The bench scrolls the album grid twice. An earlier build showed one
~20 ms frame on the first pass only; it stopped reproducing after the
pivot header change and its cause was never pinned down, so the second
pass stays as a guard.

The laptop is roughly an order of magnitude faster per core than the
RK3506B's Cortex-A7, so these numbers bound the device, they do not
predict it. Profile on hardware before tuning.

## Licenses

Keel is MIT (see `LICENSE`). Every dependency is permissive:

| Component | License | Where |
|---|---|---|
| LVGL 9.6 | MIT | fetched at configure time |
| SDL2 (sdl2-compat) | zlib | toolbox package |
| FreeType | FreeType License (FTL) | toolbox package |
| IBM Plex Sans, Plex Mono, Plex Sans Thai, Plex Sans Arabic | SIL Open Font License 1.1 | `assets/fonts/`, license in `assets/fonts/IBM-Plex-OFL.txt` |
| Phosphor Icons 2.1.2 (regular) | MIT | `assets/icons/`, license in `assets/icons/Phosphor-LICENSE.txt` |
| Demo tracklists | CC0 (MusicBrainz) | `assets/demo/library.json` |
| Demo cover art | © the artists and labels; not distributed | fetched locally, gitignored |

The Plex fonts ship as the unmodified TTF files from IBM's official release
and are rendered at run time through FreeType. They are not subset or
converted to bitmap fonts: the OFL treats a subset as a modified version, and
"Plex" is a Reserved Font Name. Plex Sans Thai and Arabic are glyph fallbacks
for real track titles such as Ninajirachi's "ฅ^•ﻌ•^ฅ".

Icons: Keel uses words first ("play", "next", "previous") and Plex glyphs
second. Phosphor icons are third, used only where a word does not fit: the
status bar's Wi-Fi and Bluetooth indicators. The font is the unmodified
`Phosphor.ttf` from the `@phosphor-icons/web` 2.1.2 package.
