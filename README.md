<!--
# SPDX-FileCopyrightText: Copyright 2025 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

# SPDX-FileCopyrightText: 2018 yuzu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later
-->
<!-- lang: en-GB -->

<h1 align="center">
  <br>
  <a href="https://github.com/kyabetsu4/Eglantine"><img src="https://github.com/user-attachments/assets/35031845-4bfa-4916-8bf5-bc3bce67d5e2" alt="Eglantine" width="200"></a>
  <br>
  <b>Eglantine — Orcheglichiined</b>
  <br>
</h1>

<h4 align="center"><b>Orcheglichiined</b> is a high-performance synthesized build of <b>Eglantine</b>, combining the performance optimizations, stability patches, and compatibility fixes from <b>Orchid</b>, <b>Lichii</b>, and <b>Eglantine Upstream</b> into a unified client.
</h4>

<p align="center">
    <a href="https://discord.gg/XkS5NVPTUY">
        <img src="https://img.shields.io/discord/1367654015269339267?color=5865F2&label=Eglantine&logo=discord&logoColor=white"
            alt="Discord">
    </a>
</p>

<p align="center">
  <a href="#about">About</a> |
  <a href="#core-highlights">Highlights</a> |
  <a href="#compatibility">Compatibility</a> |
  <a href="#building">Building</a> |
  <a href="#releases">Releases</a> |
  <a href="#license">License</a>
</p>

---

## About

The Switch 2 emulation landscape features several active community efforts independently tackling performance, game logic, and graphics bugs across different titles. 

**Orcheglichiined** unites these efforts into a single, cohesive branch:
* 🌸 **Orchid** ([MythosOfs/Orchid](https://github.com/MythosOfs/Orchid)) — Vulkan pipeline synchronization and per-title engine overrides.
* 🍃 **Lichii** ([zortos293/Lichii](https://github.com/zortos293/Lichii)) — Resource efficiency, OS-level timer thread sleeping, and filesystem resilience.
* 🌺 **Eglantine Upstream** ([Eglantine-Community](https://github.com/Eglantine-Community/Eglantine)) — Core driver improvements, compute pipeline expansions, and cache handling.

---

## Core Highlights

* ⚡ **CPU Efficiency & Pacing:** Replaces busy-wait polling in host timing loops with OS-level waitable timer sleeps, drastically reducing host CPU overhead and power draw without degrading frame pacing.
* 🎮 **Graphics & Driver Compatibility:** Enhances Vulkan stage synchronization for smooth texture uploads, expands compute shader buffer capabilities, and integrates vendor fixes for Intel Arc and modern GPUs.
* 🛡️ **Memory & Filesystem Resilience:** Hardens virtual filesystem parsers against malformed packages and trims redundant zeroed guest memory pages to keep RAM usage lean.
* ⚙️ **Per-Title Engine Tuning:** Includes tailored engine profiles for demanding titles that resolve GPU readback stalls and optimize accuracy settings.

---

## Releases & Changelogs

Specific commit histories, benchmark metrics, and per-title patch notes are documented in GitHub Releases rather than the project README.

👉 **Check the [Releases](https://github.com/kyabetsu4/Eglantine/releases) page for downloadable builds and detailed version changelogs.**

---

## Compatibility

Capable of running titles such as:
* **Mario Kart World:** Full speed with broad GPU vendor compatibility (Intel Arc, AMD, NVIDIA).
* **Xenoblade Chronicles DE (Switch 2 Edition):** Fluid 60 fps gameplay via readback-optimized engine overrides.
* **Donkey Kong Bananza / Hyrule Warriors:** Tuned GPU accuracy and balanced fence profiles to eliminate hitching.
* **Splatoon Raiders:** Variable performance with active compute and shader pipeline optimizations.

---

## Building

### Windows (MSVC 2022 + Qt 6)

The repository includes a dedicated build and packaging pipeline:

```powershell
# 1. Compile release build using MSVC, Ninja, and Qt 6.8:
powershell -ExecutionPolicy Bypass -File pc-build/build-windows-qt.ps1

# 2. Package into standalone directory and run automated smoke tests:
powershell -ExecutionPolicy Bypass -File pc-build/package-and-validate.ps1
```

The resulting binaries will be assembled in `pc-build/dist/eglantine-windows-x86_64/`:
* `eglantine.exe` (Qt GUI frontend)
* `eden-cli.exe` (Headless CLI launcher & NUP manager)

---

## Upstream & Acknowledgements

Maintained as a personal synthesized branch on [`kyabetsu4/Eglantine`](https://github.com/kyabetsu4/Eglantine/tree/orcheglichiined). Special thanks to the developers and contributors of:
* **Eglantine:** [Djihads80](https://github.com/Djihads80/Eglantine) & [Eglantine-Community](https://github.com/Eglantine-Community/Eglantine)
* **Orchid:** [MythosOfs](https://github.com/MythosOfs/Orchid)
* **Lichii:** [zortos293](https://github.com/zortos293/Lichii)
* **Eden / Yuzu / Sudachi:** The foundational open-source emulator codebases.

---

## License

Eglantine and its derivative components are licensed under the **GNU General Public License v3.0 (GPL-3.0-or-later)**. Refer to [LICENSE.txt](./LICENSE.txt) for full terms.
