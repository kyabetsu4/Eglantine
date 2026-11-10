// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: 2018 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>

#include <QEventLoop>
#include <boost/algorithm/string/replace.hpp>
#include "common/httplib.h"

#include <discord_rpc.h>
#include <fmt/format.h>

#include "common/common_types.h"
#include "common/fs/fs_util.h"
#include "common/string_util.h"
#include "core/core.h"
#include "core/file_sys/control_metadata.h"
#include "core/file_sys/patch_manager.h"
#include "core/loader/loader.h"

#include "discord_impl.h"

#ifdef YUZU_BUNDLED_OPENSSL
#include <openssl/cert.h>
#endif

namespace DiscordRPC {

namespace {

struct TitleFallback {
    u64 program_id;
    std::string_view title;
};

constexpr std::array<TitleFallback, 3> KNOWN_TITLE_FALLBACKS{{
    {0x0400A3A01EE4C000ULL, "Pokémon Pokopia"},
    {0x0400486024DEC000ULL, "The Legend of Zelda: Ocarina of Time"},
    {0x0400C3F00006E000ULL, "Mario Kart World"},
}};

std::string ResolveGameTitle(Core::System& system) {
    std::string title;
    system.GetAppLoader().ReadTitle(title);
    if (!title.empty()) {
        return title;
    }

    u64 program_id = system.GetApplicationProcessProgramID();
    if (program_id == 0) {
        system.GetAppLoader().ReadProgramId(program_id);
    }

    if (program_id != 0) {
        if (const auto metadata =
                FileSys::PatchManager::GetMetadataFromBaseOrUpdate(system, program_id);
            metadata.first != nullptr) {
            title = metadata.first->GetApplicationName();
            if (!title.empty()) {
                return title;
            }
        }

        for (const auto& entry : KNOWN_TITLE_FALLBACKS) {
            if (entry.program_id == program_id) {
                return std::string{entry.title};
            }
        }
    }

    const std::string& app_path = system.GetCurrentApplicationFilePath();
    if (!app_path.empty()) {
        const std::filesystem::path fs_path{Common::FS::ToU8String(app_path)};
        const std::string stem = Common::FS::PathToUTF8String(fs_path.stem());
        if (!stem.empty() && stem != "main") {
            return stem;
        }
    }

    if (program_id != 0) {
        return fmt::format("Title {:016X}", program_id);
    }

    return "Nintendo Switch Game";
}

} // namespace

DiscordImpl::DiscordImpl(Core::System& system_) : system{system_} {
    DiscordEventHandlers handlers{};
    // The number is the client ID for Eden, it's used for images and the
    // application name
    const char* env_client_id = std::getenv("EDEN_DISCORD_CLIENT_ID");
    const char* client_id =
        (env_client_id != nullptr && env_client_id[0] != '\0') ? env_client_id
                                                               : "1397286652128264252";
    Discord_Initialize(client_id, &handlers, 1, nullptr);
}

DiscordImpl::~DiscordImpl() {
    Discord_ClearPresence();
    Discord_Shutdown();
}

void DiscordImpl::Pause() {
    Discord_ClearPresence();
}

std::string DiscordImpl::GetGameString(const std::string& title) {
    // Convert to lowercase
    std::string icon_name = Common::ToLower(title);

    // Replace spaces with dashes
    std::replace(icon_name.begin(), icon_name.end(), ' ', '-');
    boost::replace_all(icon_name, "é", "e");

    // Remove non-alphanumeric characters but keep dashes
    std::erase_if(icon_name, [](char c) {
        return !std::isalnum(static_cast<unsigned char>(c)) && c != '-';
    });

    // Remove dashes from the start and end of the string
    icon_name.erase(icon_name.begin(), std::find_if(icon_name.begin(), icon_name.end(),
                                                    [](int ch) { return ch != '-'; }));
    icon_name.erase(
        std::find_if(icon_name.rbegin(), icon_name.rend(), [](int ch) { return ch != '-'; }).base(),
        icon_name.end());

    // Remove double dashes
    icon_name.erase(std::unique(icon_name.begin(), icon_name.end(),
                                [](char a, char b) { return a == '-' && b == '-'; }),
                    icon_name.end());

    return icon_name;
}

static constexpr char DEFAULT_DISCORD_TEXT[] = "Eglantine is an emulator for the Nintendo Switch";
static constexpr char DEFAULT_DISCORD_IMAGE[] =
    "https://git.eden-emu.dev/eden-emu/eden/raw/branch/master/dist/qt_themes/default/icons/256x256/"
    "eden.png";

void DiscordImpl::UpdateGameStatus(std::string_view game_url, bool has_boxart) {
    const std::string url = std::string{has_boxart ? game_url : DEFAULT_DISCORD_IMAGE};
    s64 start_time = std::chrono::duration_cast<std::chrono::seconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
    DiscordRichPresence presence{};

    presence.largeImageKey = url.c_str();
    presence.largeImageText = game_title.c_str();
    presence.smallImageKey = DEFAULT_DISCORD_IMAGE;
    presence.smallImageText = DEFAULT_DISCORD_TEXT;
    presence.state = game_title.c_str();
    presence.status_display_type = DiscordStatusDisplayType_State;
    presence.details = "Currently in game";
    presence.startTimestamp = start_time;
    Discord_UpdatePresence(&presence);
}

void DiscordImpl::Update() {
    if (system.IsPoweredOn()) {
        game_title = ResolveGameTitle(system);

        // Used to format Icon URL for yuzu website game compatibility page
        std::string icon_name = GetGameString(game_title);
        const std::string boxart_path =
            fmt::format("/eden-emulator/boxart/refs/heads/master/img/{}.png", icon_name);
        const std::string game_url =
            fmt::format("https://raw.githubusercontent.com{}", boxart_path);

        bool has_boxart = false;
        if (!icon_name.empty()) {
            httplib::SSLClient client("raw.githubusercontent.com");
            client.set_connection_timeout(1);
            client.set_read_timeout(1);
            client.set_follow_location(true);

#ifdef YUZU_BUNDLED_OPENSSL
            client.load_ca_cert_store(kCert, sizeof(kCert));
#endif

            httplib::Request request{
                .method = "HEAD",
                .path = boxart_path,
            };

            auto res = client.send(request);
            has_boxart = res && res->status == 200;
        }
        UpdateGameStatus(game_url, has_boxart);

        return;
    }

    s64 start_time = std::chrono::duration_cast<std::chrono::seconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
    DiscordRichPresence presence{};
    presence.largeImageKey = DEFAULT_DISCORD_IMAGE;
    presence.largeImageText = DEFAULT_DISCORD_TEXT;
    presence.status_display_type = DiscordStatusDisplayType_Name;
    presence.details = "Currently not in game";
    presence.startTimestamp = start_time;
    Discord_UpdatePresence(&presence);
}
} // namespace DiscordRPC
