#include "NativeOptions.h"
#include "soh/Enhancements/cosmetics/CosmeticsEditor.h"
#include "soh/Enhancements/cosmetics/cosmeticsTypes.h"
#include "soh/cvar_prefixes.h"

namespace NativeOptions {
namespace {
struct Placement {
    const char* label;
    const char* cvar;
    int minY, maxY, minX, maxX;
    bool edgeRange = false;
};

PagePtr PlacementPage(Placement item) {
    const std::string base = item.cvar;
    return MakePage(base, Text(item.label), [=] {
        std::vector<Row> rows;
        const bool hearts = base == CVAR_COSMETIC("HUD.HeartsCount");
        const bool magic = base == CVAR_COSMETIC("HUD.MagicBar");
        const bool enemy = base == CVAR_COSMETIC("HUD.EnemyHealthBar");
        if (!enemy)
            rows.push_back(CVarToggle(Text("use_margins"), (hearts ? std::string(CVAR_COSMETIC("HUD.Hearts")) : base) + ".UseMargins",
                                      false, Text("use_margins_help")));
        std::map<int, std::string> anchors;
        if (enemy) {
            anchors = {{ENEMYHEALTH_ANCHOR_ACTOR, Text("anchor_enemy")}, {ENEMYHEALTH_ANCHOR_TOP, Text("anchor_top")},
                       {ENEMYHEALTH_ANCHOR_BOTTOM, Text("anchor_bottom")}};
        } else {
            anchors = {{ORIGINAL_LOCATION, Text("original_position")}, {ANCHOR_LEFT, Text("anchor_left")},
                       {ANCHOR_RIGHT, Text("anchor_right")}, {ANCHOR_NONE, Text("anchor_none")}, {HIDDEN, Text("hidden")}};
            if (magic) anchors.emplace(ANCHOR_TO_LIFE_METER, Text("anchor_life"));
        }
        rows.push_back(CVarChoice(Text("position_mode"), base + ".PosType", 0, anchors));
        auto minX = item.minX;
        auto maxX = item.maxX;
        if (item.edgeRange) {
            const auto mode = CVarGetInteger((base + ".PosType").c_str(), 0);
            if (mode == ANCHOR_RIGHT)
                maxX = base == CVAR_COSMETIC("HUD.VisualSoA") || base == CVAR_COSMETIC("HUD.Dpad") ? 290 : 294;
            else if (mode == HIDDEN)
                minX = -item.maxX;
        }
        rows.push_back(CVarInteger(Text("position_y"), base + ".PosY", 0, item.minY, item.maxY, 1, Text("position_y_help")));
        rows.push_back(CVarInteger(Text("position_x"), base + ".PosX", 0, minX, maxX, 1, Text("position_x_help")));
        if (hearts)
            rows.push_back(CVarInteger(Text("heart_line"), CVAR_COSMETIC("HUD.Hearts.LineLength"), 0, 0, 20, 1, Text("heart_line_help")));
        if (enemy) {
            rows.push_back(CVarInteger(Text("health_bar_width"), CVAR_COSMETIC("HUD.EnemyHealthBar.Width.Value"), 64, 32, 128, 1,
                Text("health_bar_width_help"), [] { CVarSetInteger(CVAR_COSMETIC("HUD.EnemyHealthBar.Width.Changed"), 1); }));
            rows.push_back(Action("reset_width", Text("reset_width"), [] {
                CVarClear(CVAR_COSMETIC("HUD.EnemyHealthBar.Width.Value"));
                CVarClear(CVAR_COSMETIC("HUD.EnemyHealthBar.Width.Changed"));
                SaveSettings();
            }));
        }
        return rows;
    });
}
}

PagePtr CosmeticPositionsPage() {
    return MakePage("cosmetics/placement", Text("hud_placement"), [] {
        const auto window = Ship::Context::GetInstance()->GetWindow();
        const int w = window->GetWidth();
        const int h = window->GetHeight();
        std::vector<Row> rows;
        rows.push_back(Link("margins", Text("general_margins"), [=] {
            return MakePage("cosmetics/margins", Text("general_margins"), [=] {
                return std::vector<Row>{
                    CVarInteger(Text("top"), CVAR_COSMETIC("HUD.Margin.T"), 0, -h / 2, 25),
                    CVarInteger(Text("left"), CVAR_COSMETIC("HUD.Margin.L"), 0, -25, w),
                    CVarInteger(Text("right"), CVAR_COSMETIC("HUD.Margin.R"), 0, -w, 25),
                    CVarInteger(Text("bottom"), CVAR_COSMETIC("HUD.Margin.B"), 0, -h / 2, 25),
                    Action("all_on", Text("margins_on"), [] { SetCosmeticMargins(true); }, Text("margins_on_help")),
                    Action("all_off", Text("margins_off"), [] { SetCosmeticMargins(false); }, Text("margins_off_help")),
                    Action("reset", Text("reset_positions"), [] { ResetCosmeticPositions(); }, Text("reset_positions_help"))};
            });
        }));
        const Placement placements[] = {
            {"position_hearts", CVAR_COSMETIC("HUD.HeartsCount"), -22, h, -125, w},
            {"position_magic", CVAR_COSMETIC("HUD.MagicBar"), 0, h / 2, -5, w / 2},
            {"position_agony", CVAR_COSMETIC("HUD.VisualSoA"), 0, h / 2, 0, w / 2, true},
            {"position_b", CVAR_COSMETIC("HUD.BButton"), 0, h / 4 + 50, -1, w - 50},
            {"position_a", CVAR_COSMETIC("HUD.AButton"), -10, h / 4 + 50, -20, w - 50},
            {"position_start", CVAR_COSMETIC("HUD.StartButton"), 0, h / 2, 0, w / 2 + 70},
            {"position_c_up", CVAR_COSMETIC("HUD.CUpButton"), 0, h / 2, 0, w / 2, true},
            {"position_c_down", CVAR_COSMETIC("HUD.CDownButton"), 0, h / 2, 0, w / 2, true},
            {"position_c_left", CVAR_COSMETIC("HUD.CLeftButton"), 0, h / 2, 0, w / 2, true},
            {"position_c_right", CVAR_COSMETIC("HUD.CRightButton"), 0, h / 2, 0, w / 2, true},
            {"position_dpad", CVAR_COSMETIC("HUD.Dpad"), 0, h / 2, 0, w / 2, true},
            {"position_minimap", CVAR_COSMETIC("HUD.Minimap"), -h / 3, h / 3, -w, w / 2},
            {"position_keys", CVAR_COSMETIC("HUD.SmallKey"), 0, h / 3, -1, w / 2},
            {"position_rupees", CVAR_COSMETIC("HUD.Rupees"), -2, h / 3, -3, w / 2},
            {"position_carrots", CVAR_COSMETIC("HUD.Carrots"), 0, h / 2, -50, w / 2 + 25},
            {"position_timers", CVAR_COSMETIC("HUD.Timers"), 0, h / 2, -50, w / 2 - 50},
            {"position_archery", CVAR_COSMETIC("HUD.ArcheryScore"), 0, h / 2, -50, w / 2 - 50},
            {"position_map_title", CVAR_COSMETIC("HUD.TitleCard.Map"), 0, h / 2, -50, w / 2 + 10},
            {"position_boss_title", CVAR_COSMETIC("HUD.TitleCard.Boss"), 0, h / 2, -50, w / 2 + 10},
            {"position_igt", CVAR_COSMETIC("HUD.IGT"), 0, h / 2, -50, w / 2 + 10},
            {"position_enemy", CVAR_COSMETIC("HUD.EnemyHealthBar"), -240, 240, -w / 2, w / 2},
        };
        for (const auto& placement : placements) {
            if (std::string(placement.cvar) == CVAR_COSMETIC("HUD.VisualSoA") && !CVarGetInteger(CVAR_ENHANCEMENT("VisualAgony"), 0)) continue;
            if (std::string(placement.cvar) == CVAR_COSMETIC("HUD.Dpad") && !CVarGetInteger(CVAR_ENHANCEMENT("DpadEquips"), 0)) continue;
            rows.push_back(Link(placement.cvar, Text(placement.label), [=] { return PlacementPage(placement); }));
        }
        return rows;
    });
}
}
