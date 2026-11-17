// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <atomic>
#include <optional>

#include "common/logging.h"
#include "common/memory_detect.h"
#include "video_core/renderer_vulkan/pipeline_build_failure.h"

namespace Vulkan {

void ReportPipelineBuildFailure(VkResult result) {
    static std::atomic_flag reported;
    if (result != VK_ERROR_OUT_OF_HOST_MEMORY || reported.test_and_set()) {
        return;
    }
#ifdef _WIN32
    if (const std::optional<u64> commit_room = Common::GetSystemCommitRoom()) {
        constexpr double GiB = 1024.0 * 1024.0 * 1024.0;
        const Common::MemoryInfo mem_info = Common::GetMemInfo();
        LOG_CRITICAL(Render_Vulkan,
                     "The GPU driver ran out of host memory building a pipeline: Windows has "
                     "{:.1f} GiB of commit headroom left (RAM {:.1f} GiB, page file {:.1f} GiB). "
                     "The commit limit is RAM plus the page file, and a Switch 2 game commits 14 GiB "
                     "of guest memory when it starts. If headroom is low, enlarge the page file. "
                     "Pipelines that fail draw nothing.",
                     static_cast<double>(*commit_room) / GiB,
                     static_cast<double>(mem_info.TotalPhysicalMemory) / GiB,
                     static_cast<double>(mem_info.TotalSwapMemory) / GiB);
        return;
    }
#endif
    LOG_CRITICAL(Render_Vulkan,
                 "The GPU driver ran out of host memory building a pipeline. Close other programs "
                 "or add swap; until then, pipelines that fail draw nothing.");
}

} // namespace Vulkan
