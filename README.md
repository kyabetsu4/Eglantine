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

<h4 align="center"><b>Orcheglichiined</b> is a high-performance synthesized fork of <b>Eglantine</b>, integrating the best performance optimizations, compatibility fixes, and stability patches from <b>Orchid</b>, <b>Lichii</b>, and <b>Eglantine Upstream</b> into a unified build.
</h4>

<p align="center">
    <a href="https://discord.gg/XkS5NVPTUY">
        <img src="https://img.shields.io/discord/1367654015269339267?color=5865F2&label=Eglantine&logo=discord&logoColor=white"
            alt="Discord">
    </a>
</p>

<p align="center">
  <a href="#about-orcheglichiined">About</a> |
  <a href="#integrated-improvements">Integrated Improvements</a> |
  <a href="#compatibility">Compatibility</a> |
  <a href="#building">Building</a> |
  <a href="#license">License</a>
</p>

---

## About Orcheglichiined

The Switch 2 emulation landscape features several active community efforts independently tackling performance and graphics bugs across different titles. **Orcheglichiined** fuses these branches together:

* 🌸 **Orchid** ([MythosOfs/Orchid](https://github.com/MythosOfs/Orchid)): Vulkan texture upload synchronization, per-title engine overrides, and CPU-GPU readback decoupling.
* 🍃 **Lichii** ([zortos293/Lichii](https://github.com/zortos293/Lichii)): OS-level `HostTiming` event sleeping, memory page zero-trimming, robust filesystem parsing, and shader recompiler concurrency hardening.
* 🌺 **Eglantine Upstream** ([Eglantine-Community](https://github.com/Eglantine-Community/Eglantine)): Intel Arc driver fixes, compute pipeline 64 SSBO expansions, and clean pipeline cache shutdown routines.

---

## Integrated Improvements

| Subsystem | Source | Key Enhancement | Real-World Impact |
| :--- | :--- | :--- | :--- |
| **Video / Vulkan** | Orchid | Synchronize texture uploads & copies across all active pipeline stages | Eliminates texture tearing and missing render passes during fast scene transitions. |
| **Core Settings** | Orchid | Per-title engine overrides & preemptive CPU reads | *Xenoblade Chronicles DE (Switch 2 Edition)* device waits drop from ~70/s to <1/s (opening intro lifts from 26–32 fps to full 60 fps). Per-game profiles for *Mario Tennis Fever*, *Hyrule Warriors*, and *Donkey Kong Bananza*. |
| **Core Timing** | Lichii | OS-level waitable timer event sleeping (`Event::SleepFor`) | Replaces busy-wait polling in `HostTiming`. Reduces idle CPU consumption from ~99.7% of an entire CPU core down to **1–4%**. |
| **Memory / Common** | Lichii | Unconditional all-zero guest memory page trimming | Lowers overall RAM footprint by stripping zeroed heap pages. |
| **Virtual FS** | Lichii | Hardened PFS0, RomFS, NCA, and XCI parsers | Prevents silent crashes and heap corruptions when loading malformed or truncated package files. |
| **Shader Recompiler** | Lichii | Eliminated static `clip_distance_written` race condition in SPIR-V emit | Fixes multithreaded shader compilation crashes on high core-count CPUs. |
| **Vulkan / Drivers** | Upstream | Intel Arc optimizations for *Mario Kart World* | Resolves broken shaders and missing geometry on Intel Arc / Xe graphics. |
| **Compute Shaders** | Upstream | Up to 64 storage buffer bindings (SSBOs) | Prevents compute pipeline crashes in modern Switch 2 titles requiring wide buffer arrays. |
| **Pipeline Cache** | Upstream | Wait for active shader builds prior to cache flush on exit | Prevents pipeline cache corruption when closing the emulator while shaders compile. |

---

## Compatibility

Capable of running titles such as:
* **Mario Kart World:** Full speed with Intel Arc + AMD/NVIDIA Vulkan fixes.
* **Xenoblade Chronicles DE (Switch 2 Edition):** Fluid 60 fps intro & gameplay via preemptive CPU reads.
* **Donkey Kong Bananza / Hyrule Warriors:** Low GPU accuracy + balanced fence profiles preventing frame stalls.
* **Splatoon Raiders:** Variable performance with active shader pipeline improvements.

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

This build is maintained as a personal synthesized branch on [`kyabetsu4/Eglantine`](https://github.com/kyabetsu4/Eglantine/tree/orcheglichiined). Special thanks to the developers and contributors of:
* **Eglantine:** [Djihads80](https://github.com/Djihads80/Eglantine) & [Eglantine-Community](https://github.com/Eglantine-Community/Eglantine)
* **Orchid:** [MythosOfs](https://github.com/MythosOfs/Orchid)
* **Lichii:** [zortos293](https://github.com/zortos293/Lichii)
* **Eden / Yuzu / Sudachi:** The foundational open-source emulator codebases.

---

## License

Eglantine and its derivative components are licensed under the **GNU General Public License v3.0 (GPL-3.0-or-later)**. Refer to [LICENSE.txt](./LICENSE.txt) for full terms.
