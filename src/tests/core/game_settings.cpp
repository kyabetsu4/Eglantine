// SPDX-FileCopyrightText: Copyright 2026 Orchid Project
// SPDX-License-Identifier: GPL-3.0-or-later

// Per-title overrides in Core::GameSettings::LoadOverrides. Only the renderer's vendor string
// is read there, so minimal fakes stand in for the window and renderer.

#include <memory>
#include <catch2/catch_test_macros.hpp>
#include "common/settings.h"
#include "core/frontend/emu_window.h"
#include "core/frontend/graphics_context.h"
#include "core/game_settings.h"
#include "video_core/renderer_base.h"

namespace {

class FakeWindow final : public Core::Frontend::EmuWindow {
public:
    std::unique_ptr<Core::Frontend::GraphicsContext> CreateSharedContext() const override {
        return nullptr;
    }
    bool IsShown() const override {
        return true;
    }
};

class FakeRenderer final : public VideoCore::RendererBase {
public:
    explicit FakeRenderer(Core::Frontend::EmuWindow& window) : RendererBase(window, nullptr) {}
    void Composite(std::span<const Tegra::FramebufferConfig>) override {}
    std::vector<u8> GetAppletCaptureBuffer() override {
        return {};
    }
    VideoCore::RasterizerInterface* ReadRasterizer() override {
        return nullptr;
    }
    std::string GetDeviceVendor() const override {
        return "AMD Radeon RX 7600 (RADV NAVI33)";
    }
};

bool PreemptiveReadsFor(u64 program_id) {
    FakeWindow window;
    FakeRenderer renderer{window};
    Core::GameSettings::LoadOverrides(program_id, renderer);
    return Settings::values.preemptive_cpu_reads;
}

} // namespace

TEST_CASE("GameSettings: Xenoblade DE Switch 2 Edition skips ordered CPU reads", "[game_settings]") {
    // Title ID as the game reports it (not the enum), so a wrong ID in the table fails here.
    // The game reads a GPU-written texture every frame; ordered reads drained the host GPU.
    REQUIRE(PreemptiveReadsFor(0x0400FF500E34A001ULL));
    // The Switch 1 release keeps ordered reads.
    REQUIRE_FALSE(PreemptiveReadsFor(0x0100FF500E34A000ULL));
}

TEST_CASE("GameSettings: Wonder Switch 2 Edition overrides GPU defaults when global",
          "[game_settings]") {
    auto& accuracy = Settings::values.gpu_accuracy;
    auto& fences = Settings::values.gpu_fence_behavior;
    auto& reactive = Settings::values.use_reactive_flushing;

    const bool old_acc_global = accuracy.UsingGlobal();
    const auto old_acc = accuracy.GetValue(true);
    const auto old_acc_local = accuracy.GetValue(false);
    const bool old_fences_global = fences.UsingGlobal();
    const auto old_fences = fences.GetValue(true);
    const auto old_fences_local = fences.GetValue(false);
    const bool old_reactive_global = reactive.UsingGlobal();
    const bool old_reactive = reactive.GetValue(true);
    const bool old_reactive_local = reactive.GetValue(false);

    struct Restore {
        decltype(accuracy)& acc;
        decltype(fences)& fnc;
        decltype(reactive)& rct;
        bool ag, fg, rg;
        Settings::GpuAccuracy av, al;
        Settings::GpuFenceBehavior fv, fl;
        bool rv, rl;
        ~Restore() {
            acc.SetValue(al);
            acc.SetGlobal(ag);
            if (ag) acc.SetValue(av);
            fnc.SetValue(fl);
            fnc.SetGlobal(fg);
            if (fg) fnc.SetValue(fv);
            rct.SetValue(rl);
            rct.SetGlobal(rg);
            if (rg) rct.SetValue(rv);
            Settings::UpdateGPUAccuracy();
        }
    } restore{accuracy, fences, reactive, old_acc_global, old_fences_global, old_reactive_global,
              old_acc, old_acc_local, old_fences, old_fences_local, old_reactive, old_reactive_local};

    accuracy.SetGlobal(true);
    accuracy.SetValue(Settings::GpuAccuracy::High);
    Settings::UpdateGPUAccuracy();
    fences.SetGlobal(true);
    fences.SetValue(Settings::GpuFenceBehavior::Default);
    reactive.SetGlobal(true);
    reactive.SetValue(true);
    FakeWindow window;
    FakeRenderer renderer{window};

    SECTION("Switch 2 Edition gets Low accuracy, Balanced fences, reactive flushing off") {
        Core::GameSettings::LoadOverrides(0x040015100B514001ULL, renderer);
        CHECK(accuracy.GetValue() == Settings::GpuAccuracy::Low);
        CHECK(Settings::values.current_gpu_accuracy == Settings::GpuAccuracy::Low);
        CHECK(fences.GetValue() == Settings::GpuFenceBehavior::Balanced);
        CHECK_FALSE(reactive.GetValue());
        CHECK(accuracy.GetValue(true) == Settings::GpuAccuracy::High);
    }
    SECTION("The Switch 1 release keeps the inherited settings") {
        Core::GameSettings::LoadOverrides(0x010015100B514000ULL, renderer);
        CHECK(accuracy.GetValue() == Settings::GpuAccuracy::High);
        CHECK(fences.GetValue() == Settings::GpuFenceBehavior::Default);
        CHECK(reactive.GetValue());
    }
}
