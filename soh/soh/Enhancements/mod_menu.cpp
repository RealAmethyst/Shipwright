#include <map>
#include <vector>

#include <libultraship/classes.h>
#include <ship/utils/StringHelper.h>

#include "mod_menu.h"
#include "soh/NativeOptions/NativeOptions.h"
#include "soh/OTRGlobals.h"
#include "soh/resource/type/Skeleton.h"
#include "soh/SohGui/MenuTypes.h"
#include "soh/SohGui/SohMenu.h"
#include "soh/SohGui/SohGui.hpp"

std::vector<std::string> enabledModFiles;
std::vector<std::string> disabledModFiles;
std::vector<std::string> unsupportedFiles;
std::map<std::string, std::filesystem::path> filePaths;

namespace SohGui {
extern std::shared_ptr<SohMenu> mSohMenu;
}

static WidgetInfo enableModsWidget;
static WidgetInfo tabHotkeyWidget;

#define CVAR_ENABLED_MODS_NAME CVAR_SETTING("EnabledMods")
#define CVAR_ENABLED_MODS_DEFAULT ""
#define CVAR_ENABLED_MODS_VALUE CVarGetString(CVAR_ENABLED_MODS_NAME, CVAR_ENABLED_MODS_DEFAULT)

// "|" was chosen as the separator due to
// it being an invalid character in NTFS
// and being rarely used in ext4
// it is also an ASCII character
// improving portability

// if being an ASCII character is not a requirement,
// other possible candidates include:
// - U+FFFF: non-character
// - any private use character
#define SEPARATOR "|"

void SetEnabledModsCVarValue() {
    std::string s = "";

    for (auto& modPath : enabledModFiles) {
        s += modPath + SEPARATOR;
    }

    // remove trailing separator if present
    if (s.length() != 0) {
        s.pop_back();
    }

    CVarSetString(CVAR_ENABLED_MODS_NAME, s.c_str());
    Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
}

std::vector<std::string> GetEnabledModsFromCVar() {
    std::string enabledModsCVarValue = CVAR_ENABLED_MODS_VALUE;
    if (enabledModsCVarValue.empty())
        return {};
    return StringHelper::Split(enabledModsCVarValue, SEPARATOR);
}

std::shared_ptr<Ship::ArchiveManager> GetArchiveManager() {
    return Ship::Context::GetInstance()->GetResourceManager()->GetArchiveManager();
}

bool IsValidExtension(std::string extension) {
    if (
#ifdef INCLUDE_MPQ_SUPPORT
        // .mpq doesn't make sense to support because all tools to make such mods output OTR
        StringHelper::IEquals(extension, ".otr") /*|| StringHelper::IEquals(extension, ".mpq")*/ ||
#endif
        // .zip needs to be excluded because mods are most often distributed in zip archives
        // and thus could contain .otr/o2r files
        StringHelper::IEquals(extension, ".o2r") /*|| StringHelper::IEquals(extension, ".zip")*/) {
        return true;
    }
    return false;
}

void UpdateModFiles(bool init = false, bool reset = false) {
    if (init || reset) {
        enabledModFiles.clear();
        enabledModFiles = GetEnabledModsFromCVar();
    }
    disabledModFiles.clear();
    unsupportedFiles.clear();
    filePaths.clear();
    bool changed = false;
    std::string modsPath = Ship::Context::LocateFileAcrossAppDirs("mods", appShortName);
    std::map<std::string, std::string> tempMods;
    if (modsPath.length() > 0 && std::filesystem::exists(modsPath)) {
        std::vector<std::filesystem::path> enabledFiles;
        if (std::filesystem::is_directory(modsPath)) {
            for (const std::filesystem::directory_entry& p : std::filesystem::recursive_directory_iterator(
                     modsPath, std::filesystem::directory_options::follow_directory_symlink)) {
                if (p.is_directory()) {
                    continue;
                }
                std::string filename =
                    p.path().filename().generic_string().substr(0, p.path().filename().generic_string().rfind("."));
                std::string extension = p.path().extension().generic_string();
                if (!IsValidExtension(extension)) {
                    continue;
                }
                bool enabled =
                    std::find(enabledModFiles.begin(), enabledModFiles.end(), filename) != enabledModFiles.end();
                if (!enabled) {
                    tempMods.emplace(p.path().lexically_normal().generic_string(), filename);
                }
                filePaths.emplace(filename, p.path());
            }
            if (tempMods.size() > 0) {
                changed = true;
                for (auto [path, name] : tempMods) {
                    enabledModFiles.push_back(name);
                }
                tempMods.clear();
            }
            if (init) {
                std::vector<std::string> enabledTemp(enabledModFiles);
                for (std::string mod : enabledTemp) {
                    if (filePaths.contains(mod)) {
                        GetArchiveManager()->AddArchive(filePaths.at(mod).generic_string());
                    } else {
                        enabledModFiles.erase(std::find(enabledModFiles.begin(), enabledModFiles.end(), mod));
                        changed = true;
                    }
                }
            }
        }
        if (changed) {
            SetEnabledModsCVarValue();
        }
    }
}


namespace {
using namespace NativeOptions;
bool editing = false;

PagePtr EditMods() {
    auto order = std::make_shared<std::vector<std::string>>(enabledModFiles);
    editing = true;
    auto page = MakePage("mods/order", NativeOptions::Text("enabled_mods"), [order] {
        std::vector<Row> rows;
        for (size_t reverse = order->size(); reverse > 0; --reverse) {
            const auto index = reverse - 1;
            const auto file = order->at(index);
            const auto path = filePaths.find(file);
            const auto name = path == filePaths.end() ? file : path->second.filename().generic_string();
            rows.push_back(Link(file, name, [order, file, name] {
                return MakePage("mods/" + file, name, [order, file] {
                    auto current = std::find(order->begin(), order->end(), file);
                    if (current == order->end()) return std::vector<Row>{};
                    const size_t index = static_cast<size_t>(current - order->begin());
                    auto up = Action("up", NativeOptions::Text("move_up"), [order, index] {
                        std::swap(order->at(index), order->at(index + 1));
                    });
                    auto down = Action("down", NativeOptions::Text("move_down"), [order, index] {
                        std::swap(order->at(index), order->at(index - 1));
                    });
                    up.enabled = index + 1 < order->size();
                    down.enabled = index > 0;
                    return std::vector<Row>{std::move(up), std::move(down)};
                });
            }));
        }
        rows.push_back(Action("clear", NativeOptions::Text("clear_list"), [order] {
            Confirm(NativeOptions::Text("clear_list"), NativeOptions::Text("clear_mods_description"), NativeOptions::Text("clear"), [order] { order->clear(); });
        }));
        rows.push_back(Action("apply", NativeOptions::Text("apply_close"), [order] {
            Confirm(NativeOptions::Text("apply_close"), NativeOptions::Text("apply_mods_description"), NativeOptions::Text("close"), [order] {
                enabledModFiles = *order;
                SetEnabledModsCVarValue();
                Ship::Context::GetInstance()->GetConsoleVariables()->Save();
                Ship::Context::GetInstance()->GetWindow()->Close();
            });
        }));
        rows.push_back(Action("cancel", NativeOptions::Text("cancel"), [] { GetModel().Back(); }));
        return rows;
    }, NativeOptions::Text("mod_priority"));
    page->onClose = [] { editing = false; };
    return page;
}

PagePtr ModMenuPage() {
    return MakePage("mods", NativeOptions::Text("mod_menu"), [] {
        std::vector<Row> rows;
        AppendWidget(rows, enableModsWidget, "mods/enabled");
        AppendWidget(rows, tabHotkeyWidget, "mods/hotkey");
        rows.push_back(Link("edit", NativeOptions::Text("edit"), EditMods, NativeOptions::Text("mods_restart")));
        return rows;
    });
}
} // namespace

void InitializeMods() {
    UpdateModFiles(true);
}

void RegisterModMenuWidgets() {
    NativeOptions::RegisterPage("Mod Menu", ModMenuPage);
    enableModsWidget = { .name = "Enable Mods", .type = WidgetType::WIDGET_CVAR_CHECKBOX };
    enableModsWidget.CVar(CVAR_SETTING("AltAssets"))
        .RaceDisable(false)
        .Options(UIWidgets::CheckboxOptions({ { .disabledTooltip = "Temporarily disabled while editing mods list." } })
                     .Color(THEME_COLOR)
                     .Tooltip("Toggle mods. For graphics mods, this means toggling between default and mod graphics.")
                     .DefaultValue(true))
        .PreFunc([&](WidgetInfo& info) {
            auto options = std::static_pointer_cast<UIWidgets::CheckboxOptions>(info.options);
            options->disabled = editing;
        });
    SohGui::mSohMenu->AddSearchWidget({ enableModsWidget, "Settings", "Mod Menu", "Top", "alternate assets" });

    tabHotkeyWidget = { .name = "Mods Tab Hotkey", .type = WidgetType::WIDGET_CVAR_CHECKBOX };
    tabHotkeyWidget.CVar(CVAR_SETTING("Mods.AlternateAssetsHotkey"))
        .RaceDisable(false)
        .Options(UIWidgets::CheckboxOptions()
                     .Color(THEME_COLOR)
                     .Tooltip("Allows pressing the Tab key to toggle mods")
                     .DefaultValue(true));
    SohGui::mSohMenu->AddSearchWidget(
        { tabHotkeyWidget, "Settings", "Mod Menu", "Top", "alternate assets tab hotkey" });
}

static RegisterMenuInitFunc menuInitFunc(RegisterModMenuWidgets);
