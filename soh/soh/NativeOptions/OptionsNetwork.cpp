#include "NativeOptions.h"
#include "soh/Network/Anchor/Anchor.h"
#include "soh/Network/Sail/Sail.h"
#include "soh/Network/CrowdControl/CrowdControl.h"
#include "soh/SohGui/MenuTypes.h"
#include "soh/OTRGlobals.h"
#include "soh/util.h"

#include <set>

extern "C" PlayState* gPlayState;

namespace NativeOptions {
namespace {

PagePtr HostPort(const std::string& prefix, std::function<bool()> connected) {
    return MakePage(prefix, Text("host_port"), [=] {
        std::vector<Row> rows = {
            CVarString(Text("host"), prefix + "Host", "127.0.0.1"),
            CVarInteger(Text("port"), prefix + "Port", 43384, 1025, 65534),
        };
        if (connected())
            Disable(rows, Text("disconnect_to_edit"));
        else if (CVarGetInteger(CVAR_SETTING("DisableChanges"), 0))
            Disable(rows, Text("race_lockout"));
        return rows;
    });
}

bool AnchorAdmin() {
    const auto anchor = Anchor::Instance;
    return anchor->isEnabled && anchor->isConnected && anchor->roomState.ownerClientId == anchor->ownClientId &&
           std::string(CVarGetString(CVAR_REMOTE_ANCHOR("RoomId"), "")) != "soh-global";
}

PagePtr AnchorRoom() {
    return MakePage("anchor/room", Text("anchor_room"), [] {
        auto anchor = Anchor::Instance;
        std::vector<Row> rows;
        if (!anchor->isConnected) return rows;
        if (std::string(CVarGetString(CVAR_REMOTE_ANCHOR("RoomId"), "")) == "soh-global") {
            size_t online = 0;
            for (const auto& [id, client] : anchor->clients) if (client.online) ++online;
            auto row = Action("online", Text("players_online"), [] { ReadCurrentDescription(); });
            row.value = std::to_string(online);
            rows.push_back(std::move(row));
            return rows;
        }
        for (const auto& [clientId, client] : anchor->clients) {
            const auto name = client.self ? CVarGetString(CVAR_REMOTE_ANCHOR("Name"), "") : client.name;
            auto row = Action(std::to_string(clientId), name, [clientId] {
                if (Anchor::Instance->CanTeleportTo(clientId))
                    Anchor::Instance->SendPacket_RequestTeleport(clientId);
            });
            row.value = client.teamId;
            if (clientId == anchor->roomState.ownerClientId)
                row.description = Text("room_owner");
            if (!client.online) {
                row.description += " " + Text("offline");
            } else {
                const bool ownTeam = client.teamId == CVarGetString(CVAR_REMOTE_ANCHOR("TeamId"), "default");
                if ((anchor->roomState.showLocationsMode == 2 ||
                     (anchor->roomState.showLocationsMode == 1 && ownTeam)) &&
                    (client.self ? anchor->IsSaveLoaded() : client.isSaveLoaded)) {
                    row.description += " " + SohUtils::GetSceneName(client.self ? gPlayState->sceneNum : client.sceneNum);
                }
                if (anchor->CanTeleportTo(clientId))
                    row.description += " " + Text("teleport_hint");
                if (client.clientVersion != Anchor::clientVersion)
                    row.description += " " + Text("version_mismatch") + " " + Anchor::clientVersion + ", " + client.clientVersion;
                const uint32_t seed = IS_RANDO ? Rando::Context::GetInstance()->GetSeed() : 0;
                if (client.isSaveLoaded && anchor->IsSaveLoaded() && client.seed != seed && !client.self)
                    row.description += " " + Text("seed_mismatch") + " " + std::to_string(seed) + ", " + std::to_string(client.seed);
            }
            rows.push_back(std::move(row));
        }
        return rows;
    });
}

PagePtr AnchorConnection() {
    return MakePage("anchor/connection", Text("connection_settings"), [] {
        auto anchor = Anchor::Instance;
        std::vector<Row> rows = {
            CVarString(Text("host"), CVAR_REMOTE_ANCHOR("Host"), "anchor.hm64.org"),
            CVarInteger(Text("port"), CVAR_REMOTE_ANCHOR("Port"), 43383, 1025, 65534),
            CVarString(Text("name"), CVAR_REMOTE_ANCHOR("Name")),
            CVarString(Text("room_id"), CVAR_REMOTE_ANCHOR("RoomId")),
            CVarString(Text("team_id"), CVAR_REMOTE_ANCHOR("TeamId"), "default"),
        };
        rows.push_back(CVarColor(Text("color"), CVAR_REMOTE_ANCHOR("Color"), {100, 255, 100, 255}));
        rows.push_back(Action("defaults", Text("restore_defaults"), [] {
            CVarSetString(CVAR_REMOTE_ANCHOR("Host"), "anchor.hm64.org");
            CVarSetInteger(CVAR_REMOTE_ANCHOR("Port"), 43383);
            CVarSetString(CVAR_REMOTE_ANCHOR("TeamId"), "default");
            CVarSetString(CVAR_REMOTE_ANCHOR("RoomId"), "");
            CVarSetString(CVAR_REMOTE_ANCHOR("Name"), "");
            SaveSettings();
        }));
        rows.push_back(Action("global", Text("global_room"), [] {
            CVarSetString(CVAR_REMOTE_ANCHOR("Host"), "anchor.hm64.org");
            CVarSetInteger(CVAR_REMOTE_ANCHOR("Port"), 43383);
            CVarSetString(CVAR_REMOTE_ANCHOR("TeamId"), "default");
            CVarSetString(CVAR_REMOTE_ANCHOR("RoomId"), "soh-global");
            SaveSettings();
        }, Text("global_room_description")));
        if (anchor->isEnabled)
            Disable(rows, Text("disconnect_to_edit"));
        const auto host = CVarGetString(CVAR_REMOTE_ANCHOR("Host"), "anchor.hm64.org");
        const auto room = CVarGetString(CVAR_REMOTE_ANCHOR("RoomId"), "");
        const auto name = CVarGetString(CVAR_REMOTE_ANCHOR("Name"), "");
        const auto port = CVarGetInteger(CVAR_REMOTE_ANCHOR("Port"), 43383);
        auto connect = Action("connect", Text(anchor->isEnabled ? "disable" : "enable"), [anchor] {
            if (anchor->isEnabled) {
                CVarClear(CVAR_REMOTE_ANCHOR("Enabled"));
                anchor->Disable();
            } else {
                CVarSetInteger(CVAR_REMOTE_ANCHOR("Enabled"), 1);
                anchor->Enable();
            }
            SaveSettings();
        });
        connect.enabled = anchor->isEnabled || (!SohUtils::IsStringEmpty(host) && !SohUtils::IsStringEmpty(room) &&
                           !SohUtils::IsStringEmpty(name) && port > 1024 && port < 65535);
        if (!connect.enabled)
            connect.disabledReason = Text("connection_required");
        rows.push_back(std::move(connect));
        if (anchor->isEnabled) {
            rows.push_back(Action("status", Text(anchor->isConnected ? "connected" : "connecting"), {}));
            if (anchor->isConnected) {
                rows.push_back(Action("request", Text("request_team_state"), [anchor] {
                    anchor->SendPacket_RequestTeamState();
                }, Text("request_team_state_description")));
                rows.push_back(Link("room", Text("anchor_room"), AnchorRoom));
                rows.push_back(CVarToggle(Text("room_overlay"), CVAR_WINDOW("AnchorRoom")));
                rows.push_back(OverlayLayout("Anchor Room"));
            }
        }
        return rows;
    });
}

PagePtr AnchorSettings() {
    return MakePage("anchor/admin", Text("room_settings"), [] {
        auto update = [] { if (AnchorAdmin()) Anchor::Instance->SendPacket_UpdateRoomState(); };
        std::vector<Row> rows = {
            Action("clear", Text("clear_team_state"), [] {
                if (!AnchorAdmin()) return;
                std::set<std::string> teams;
                for (const auto& [id, client] : Anchor::Instance->clients)
                    teams.insert(client.teamId);
                for (const auto& team : teams)
                    Anchor::Instance->SendPacket_ClearTeamState(team);
            }),
            CVarChoice(Text("pvp_mode"), CVAR_REMOTE_ANCHOR("RoomSettings.PvpMode"), 1,
                       {{0, Text("off")}, {1, Text("on")}, {2, Text("friendly_fire")}}, "", update),
            CVarChoice(Text("show_locations"), CVAR_REMOTE_ANCHOR("RoomSettings.ShowLocationsMode"), 1,
                       {{0, Text("none")}, {1, Text("team_only")}, {2, Text("all")}}, "", update),
            CVarChoice(Text("teleport_to"), CVAR_REMOTE_ANCHOR("RoomSettings.TeleportMode"), 1,
                       {{0, Text("none")}, {1, Text("team_only")}, {2, Text("all")}}, "", update),
            CVarToggle(Text("sync_items_flags"), CVAR_REMOTE_ANCHOR("RoomSettings.SyncItemsAndFlags"), true, "", update),
        };
        if (!AnchorAdmin()) Disable(rows, Text("admin_required"));
        return rows;
    });
}
} // namespace

void UpdateNetworkWhilePaused() {
#ifdef ENABLE_REMOTE_CONTROL
    // Anchor normally drains on OnGameFrameUpdate, including the original pause menu.
    // Keep that service running when native Options replaces the game-state frame.
    if (!GfxDebuggerIsDebugging() && Anchor::Instance && Anchor::Instance->isConnected)
        Anchor::Instance->ProcessIncomingPacketQueue();
#endif
}

void RegisterNetworkPages() {
#ifdef ENABLE_REMOTE_CONTROL
    RegisterPage("Network/Sail/Host & Port", [] {
        return HostPort(CVAR_REMOTE_SAIL(""), [] { return Sail::Instance->isEnabled; });
    });
    RegisterPage("Network/Crowd Control/Host & Port", [] {
        return HostPort(CVAR_REMOTE_CROWD_CONTROL(""), [] { return CrowdControl::Instance->isEnabled; });
    });
    RegisterPage("Network/Anchor/AnchorMainMenu", AnchorConnection, Text("connection_settings"));
    RegisterPage("Network/Anchor/AnchorAdminMenu", AnchorSettings, Text("room_settings"));
    RegisterPage("Network/Anchor/AnchorInstructionsMenu", [] {
        auto page = MakePage("anchor/instructions", Text("usage_instructions"), [] {
            return std::vector<Row>{Action("close", Text("close"), [] { GetModel().Back(); })};
        }, Text("anchor_instructions"));
        page->popup = true;
        return page;
    }, Text("usage_instructions"));
    RegisterPage("Anchor Room", AnchorRoom);
#endif
}

} // namespace NativeOptions
