// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "common/literals.h"
#include "core/hle/kernel/k_address_space_info.h"

using namespace Common::Literals;

TEST_CASE("Switch 2 processes get a 128 GiB alias region", "[kernel][heap]") {
    using Space = Kernel::KAddressSpaceInfo;
    using Type = Space::Type;

    // Xenoblade 2 reserves 60 GiB of alias space twice through its SDK VAMM allocator.
    REQUIRE(Space::GetAddressSpaceSize(39, Type::Alias, true) == 128_GiB);
    REQUIRE(Space::GetAddressSpaceSize(39, Type::Alias, true) > 2 * 60_GiB);
    REQUIRE(Space::GetAddressSpaceSize(39, Type::Alias, false) == 64_GiB);
    REQUIRE(Space::GetAddressSpaceSize(36, Type::Alias, true) == 6_GiB);
    // Alias, heap, stack and kernel map must still fit the 256 GiB NCE code space.
    REQUIRE(Space::GetAddressSpaceSize(39, Type::Alias, true) +
                Space::GetAddressSpaceSize(39, Type::Heap, true) +
                Space::GetAddressSpaceSize(39, Type::Stack, true) +
                Space::GetAddressSpaceSize(39, Type::MapSmall, true) <
            256_GiB - 128_MiB);
}
