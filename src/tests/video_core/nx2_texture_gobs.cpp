// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <array>
#include <span>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "core/core.h"
#include "video_core/host1x/gpu_device_memory_manager.h"
#include "video_core/memory_manager.h"
#include "video_core/texture_cache/image_info.h"
#include "video_core/texture_cache/util.h"
#include "video_core/textures/decoders.h"
#include "video_core/textures/texture.h"

TEST_CASE("Unswizzling an image with no guest bytes yields zeros",
          "[video_core][texture_cache]") {
    Core::System system;
    system.Initialize();
    Tegra::MaxwellDeviceMemoryManager device_memory(system.DeviceMemory());
    Tegra::MemoryManager gpu_memory(system, device_memory);

    VideoCommon::ImageInfo info;
    info.type = VideoCommon::ImageType::e2D;
    info.format = VideoCore::Surface::PixelFormat::R8_UNORM;
    info.size = {64, 64, 1};
    info.resources = {2, 2};
    info.layer_stride = VideoCommon::CalculateLayerStride(info);
    std::vector<u8> output(64 * 64 * 2 * 2, 0xee);
    const auto copies =
        VideoCommon::UnswizzleImage(gpu_memory, 0, info, std::span<const u8>{}, output);
    REQUIRE(copies.size() == 2);
    CHECK(std::ranges::all_of(std::span{output}.first(64 * 64 * 2), [](u8 x) { return x == 0; }));
}

TEST_CASE("Switch 2 headers asking for Maxwell GOBs decode without the bit 5/6 swap",
          "[video_core][texture_cache]") {
    Tegra::Texture::TICEntry tic{};
    tic.format.Assign(Tegra::Texture::TextureFormat::R8);
    tic.r_type.Assign(Tegra::Texture::ComponentType::UNORM);
    tic.g_type.Assign(Tegra::Texture::ComponentType::UNORM);
    tic.b_type.Assign(Tegra::Texture::ComponentType::UNORM);
    tic.a_type.Assign(Tegra::Texture::ComponentType::UNORM);
    tic.header_version.Assign(Tegra::Texture::TICHeaderVersion::BlockLinear);
    tic.texture_type.Assign(Tegra::Texture::TextureType::Texture2D);
    tic.block_height.Assign(1);
    tic.width_minus_one.Assign(63);
    tic.height_minus_1.Assign(15);
    const bool maxwell = GENERATE(false, true);
    tic.load_store_hint.Assign(maxwell ? 1 : 0);
    const VideoCommon::ImageInfo info{tic};
    REQUIRE(info.maxwell_gobs == maxwell);
    REQUIRE(VideoCommon::UsesNativeGobs(true, info) == !maxwell);
    REQUIRE_FALSE(VideoCommon::UsesNativeGobs(false, info));
    // Render targets and other images built without a texture header keep the Switch 2 layout.
    REQUIRE(VideoCommon::UsesNativeGobs(true, VideoCommon::ImageInfo{}));

    Core::System system;
    system.Initialize();
    Tegra::MaxwellDeviceMemoryManager device_memory(system.DeviceMemory());
    Tegra::MemoryManager gpu_memory(system, device_memory);
    gpu_memory.EnableNativeTiling();

    std::vector<u8> linear(64 * 16);
    for (size_t i = 0; i < linear.size(); ++i) {
        linear[i] = static_cast<u8>(i * 7 + i / 64);
    }
    std::vector<u8> tiled(2 * 512);
    Tegra::Texture::SwizzleTexture(tiled, linear, 1, 64, 16, 1, 1, 0, 1, !maxwell);
    std::vector<u8> decoded(linear.size());
    (void)VideoCommon::UnswizzleImage(gpu_memory, 0, info, tiled, decoded);
    REQUIRE(decoded == linear);

    const std::array<std::pair<u64, u64>, 1> ranges{{{0, tiled.size()}}};
    const auto plan = VideoCommon::PlanUnswizzleImageRows(0, info, ranges);
    CHECK(plan.maxwell_gobs == maxwell);
}
