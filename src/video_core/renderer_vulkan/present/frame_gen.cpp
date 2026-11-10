// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <cmath>

#include "common/settings.h"
#include "video_core/renderer_vulkan/present/frame_gen.h"
#include "video_core/renderer_vulkan/vk_present_manager.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"

namespace Vulkan {

namespace {

constexpr u64 REQUIRED_FRAMES = 2;
constexpr u32 RECURRENCE_FRAMES = 2;

[[nodiscard]] f32 ManualFlowScale() {
    return static_cast<f32>(Settings::values.frame_gen_flow_scale.GetValue()) / 100.0f;
}

[[nodiscard]] f32 ConfiguredFlowScale(VkExtent2D guest_extent, VkExtent2D presented_extent) {
    if (!Settings::values.frame_gen_flow_scale_auto.GetValue()) {
        return ManualFlowScale();
    }
    if (guest_extent.width == 0 || presented_extent.width == 0) {
        return 1.0f;
    }

    const f32 rendered_width = static_cast<f32>(guest_extent.width) *
                               Settings::values.resolution_info.up_factor;
    const f32 ratio = rendered_width / static_cast<f32>(presented_extent.width);

    constexpr f32 FLOW_SCALE_STEPS = 20.0f;
    const f32 stepped = std::ceil(ratio * FLOW_SCALE_STEPS) / FLOW_SCALE_STEPS;
    return std::clamp(stepped, 0.25f, 1.0f);
}

} // Anonymous namespace

FrameGen::FrameGen(MemoryAllocator& memory_allocator_, Scheduler& scheduler_)
    : memory_allocator{memory_allocator_}, scheduler{scheduler_} {}

FrameGen::~FrameGen() = default;

void FrameGen::Process(const Device& device, Frame* frame, VkFormat format,
                       VkExtent2D guest_extent) {
    generated = false;

    if (!Settings::FrameGenEnabled()) {
        if (fsr3_chain) {
            scheduler.Finish();
            fsr3_chain.reset();
        }
        warm_streak = 0;
        return;
    }

    if (!frame->storage_view) {
        warm_streak = 0;
        return;
    }

    peak_guest_extent.width = std::max(peak_guest_extent.width, guest_extent.width);
    peak_guest_extent.height = std::max(peak_guest_extent.height, guest_extent.height);

    const VkExtent2D extent{.width = frame->width, .height = frame->height};
    const f32 flow_scale = ConfiguredFlowScale(peak_guest_extent, extent);
    if (!fsr3_chain || built_extent.width != extent.width ||
        built_extent.height != extent.height || built_format != format ||
        built_flow_scale != flow_scale) {
        Rebuild(device, extent, format, flow_scale);
    }

    const u64 count = frame_count++;
    last_generations = plan.generations;

    const bool warm = plan.warm && count + 1 >= REQUIRED_FRAMES;
    warm_streak = warm ? warm_streak + 1 : 0;
    generated = warm && warm_streak >= RECURRENCE_FRAMES && plan.generations > 0;

    if (fsr3_chain) {
        scheduler.RequestOutsideRenderPassOperationContext();
        fsr3_chain->Process(device, scheduler, *frame->image, *frame->storage_view, count);
    }
}

size_t FrameGen::WantedGenerations(size_t capacity) {
    plan = pacer.Plan(capacity);
    return plan.generations;
}

size_t FrameGen::GeneratedFrameCount() const {
    return generated ? last_generations : 0;
}

void FrameGen::GenerateInto(const Device& device, Frame* destination, size_t generation) {
    if (!fsr3_chain) {
        return;
    }
    const f32 blend = static_cast<f32>(generation + 1) / static_cast<f32>(last_generations + 1);
    const f32 flow_scale = ConfiguredFlowScale(peak_guest_extent, built_extent);

    scheduler.RequestOutsideRenderPassOperationContext();
    fsr3_chain->Generate(device, scheduler, *destination->storage_view, blend, flow_scale);
}

void FrameGen::Rebuild(const Device& device, VkExtent2D extent, VkFormat format, f32 flow_scale) {
    scheduler.Finish();
    fsr3_chain.reset();

    built_flow_scale = flow_scale;
    fsr3_chain.emplace(device, memory_allocator, extent, format);
    built_extent = extent;
    built_format = format;
    frame_count = 0;
    warm_streak = 0;
    generated = false;
}

} // namespace Vulkan
