# Steam Deck Pokopia Config & Scripts

This directory contains the Steam Deck Pokopia configuration profiles, launcher scripts, and setup guide branched from `orcheglichiined` (large binary payloads excluded).

## Files

- `config/deck-controls.ini` — Emulator configuration with direct Steam Deck SDL controller bindings (`guid:03000000de2800000512000000026800`) used when launched outside Steam (`SteamAppId` unset/`0`).
- `config/steam-controls.ini` — Emulator configuration with Steam Virtual Gamepad bindings (`guid:03000000de280000ff11000001000000`) used when launched via Steam Gaming Mode (`SteamAppId` set).
- `launch.sh` — Entry wrapper that executes `launch-pokopia.py` via `/usr/bin/python3`.
- `launch-pokopia.py` — Session launcher that resolves/prompts character name (`--name`, `--rename`, or `kdialog`), syncs `profiles.dat`, sets `EDEN_NX2_*` environment variables, chooses `steam-controls.ini` vs `deck-controls.ini`, launches `eden-cli-name-entry`, and captures `summary.log`.
- `install-steam.py` — Adds/updates the `Pokopia` Non-Steam shortcut in Steam's `shortcuts.vdf` (and copies grid artwork when present) and creates `Pokopia.desktop`.
- `enable_lsfg.sh` — Enables `frame_gen=true` (`frame_gen_type=0`) in `steam-controls.ini` and `deck-controls.ini` under `/home/deck/pokopia-upstream/test/`.
- `update_ini.sh` — Sets `async_presentation=false` and `anti_aliasing=0` in `steam-controls.ini` and `deck-controls.ini` under `/home/deck/pokopia-upstream/test/` and ensures `launch-pokopia.py` is executable.
- `guide.html` — Steam Deck installation quick-start guide.

## Key Configuration Overrides in `*.ini`

- `resolution_setup=2` (0.75x scale)
- `gpu_accuracy=0` (Normal)
- `use_asynchronous_shaders=true`
- `anti_aliasing=0`
- `frame_gen=true`, `frame_gen_type=0` (LSFG)
- `cpu_accuracy=2` (Unsafe)
