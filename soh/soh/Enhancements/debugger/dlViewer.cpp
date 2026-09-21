#include "dlViewer.h"
#include "soh/NativeOptions/NativeOptions.h"
#include "soh/OTRGlobals.h"
#include <fast/resource/type/DisplayList.h>
#include <algorithm>
#include <cctype>

extern "C" {
#include "global.h"
}

namespace {
namespace N = NativeOptions;
std::string search;
std::map<int, std::string> cmdMap = {
    { G_SETPRIMCOLOR, "gsDPSetPrimColor" },
    { G_SETENVCOLOR, "gsDPSetEnvColor" },
    { G_RDPPIPESYNC, "gsDPPipeSync" },
    { G_SETGRAYSCALE, "gsSPGrayscale" },
    { G_SETINTENSITY, "gsDPSetGrayscaleColor" },
    { G_LOADTLUT, "gsDPLoadTLUT" },
    { G_ENDDL, "gsSPEndDisplayList" },
    { G_TEXTURE, "gsSPTexture" },
    { G_SETTIMG, "gsDPSetTextureImage" },
    { G_SETTIMG_OTR_HASH, "gsDPSetTextureImage" },
    { G_SETTIMG_OTR_FILEPATH, "gsDPSetTextureImage" },
    { G_RDPTILESYNC, "gsDPTileSync" },
    { G_SETTILE, "gsDPSetTile" },
    { G_RDPLOADSYNC, "gsDPLoadSync" },
    { G_LOADBLOCK, "gsDPLoadBlock" },
    { G_SETTILESIZE, "gsDPSetTileSize" },
    { G_DL, "gsSPDisplayList" },
    { G_DL_OTR_FILEPATH, "gsSPDisplayList" },
    { G_DL_OTR_HASH, "gsSPDisplayList" },
    { G_MTX, "gsSPMatrix" },
    { G_MTX_OTR, "gsSPMatrix" },
    { G_VTX, "gsSPVertex" },
    { G_VTX_OTR_FILEPATH, "gsSPVertex" },
    { G_VTX_OTR_HASH, "gsSPVertex" },
    { G_GEOMETRYMODE, "gsSPSetGeometryMode" },
    { G_SETOTHERMODE_H, "gsSPSetOtherMode_H" },
    { G_SETOTHERMODE_L, "gsSPSetOtherMode_L" },
    { G_TRI1, "gsSP1Triangle" },
    { G_TRI1_OTR, "gsSP1Triangle" },
    { G_TRI2, "gsSP2Triangles" },
    { G_SETCOMBINE, "gsDPSetCombineLERP" },
    { G_CULLDL, "gsSPCullDisplayList" },
    { G_NOOP, "gsDPNoOp" },
    { G_SPNOOP, "gsSPNoOp" },
    { G_MARKER, "LUS Custom Marker" },
};


size_t InstructionWords(int command) {
    switch (command) {
        case G_SETTIMG_OTR_HASH:
        case G_DL_OTR_HASH:
        case G_VTX_OTR_HASH:
        case G_BRANCH_Z_OTR:
        case G_MARKER:
        case G_MTX_OTR:
        case G_MOVEMEM_OTR:
        case G_VTX_OTR_FILEPATH:
        case G_FILLWIDERECT:
        case G_READFB:
        case G_REGBLENDEDTEX:
            return 2;
        case G_TEXRECT:
        case G_TEXRECTFLIP:
        case G_TEXRECT_WIDE:
        case G_IMAGERECT:
            return 3;
        default:
            return 1;
    }
}

std::string Description(const std::shared_ptr<Fast::DisplayList>& resource, size_t index) {
    const auto* gfx = reinterpret_cast<const Gfx*>(&resource->Instructions[index]);
    const int command = gfx->words.w0 >> 24;
    std::string text = fmt::format("w0: 0x{:x}, w1: 0x{:x}", gfx->words.w0, gfx->words.w1);
    auto field = [&text](const char* key, uint64_t value) { text += fmt::format("\n{}: {}", key, value); };
    if (command == G_SETTILE) {
        for (auto [key, value] : std::initializer_list<std::pair<const char*, uint64_t>>{
            {"FMT", _SHIFTR(gfx->words.w0, 21, 3)}, {"SIZ", _SHIFTR(gfx->words.w0, 19, 2)},
            {"LINE", _SHIFTR(gfx->words.w0, 9, 9)}, {"TMEM", _SHIFTR(gfx->words.w0, 0, 9)},
            {"TILE", _SHIFTR(gfx->words.w1, 24, 3)}, {"PAL", _SHIFTR(gfx->words.w1, 20, 4)},
            {"CMT", _SHIFTR(gfx->words.w1, 18, 2)}, {"MASKT", _SHIFTR(gfx->words.w1, 14, 4)},
            {"SHIFT", _SHIFTR(gfx->words.w1, 10, 4)}, {"CMS", _SHIFTR(gfx->words.w1, 8, 2)},
            {"MASKS", _SHIFTR(gfx->words.w1, 4, 4)}, {"SHIFTS", _SHIFTR(gfx->words.w1, 0, 4)}})
            field(key, value);
    }
    if (command == G_SETTIMG || command == G_SETTIMG_OTR_HASH || command == G_SETTIMG_OTR_FILEPATH) {
        field("FMT", _SHIFTR(gfx->words.w0, 21, 3));
        field("SIZ", _SHIFTR(gfx->words.w0, 19, 2));
        field("WIDTH", _SHIFTR(gfx->words.w0, 0, 10));
    }
    if (command == G_VTX || command == G_VTX_OTR_HASH) {
        field("Num VTX", _SHIFTR(gfx->words.w0, 12, 8));
        field("Offset", _SHIFTR(gfx->words.w0, 1, 7) - _SHIFTR(gfx->words.w0, 12, 8));
    }
    const char* name = nullptr;
    const char* label = nullptr;
    if (command == G_SETTIMG_OTR_HASH || command == G_VTX_OTR_HASH || command == G_DL_OTR_HASH) {
        if (index + 1 < resource->Instructions.size()) {
            const auto& next = resource->Instructions[index + 1];
            name = ResourceGetNameByCrc((static_cast<uint64_t>(next.words.w0) << 32) + next.words.w1);
        }
        label = command == G_SETTIMG_OTR_HASH ? "dl_texture_name" :
                command == G_VTX_OTR_HASH ? "dl_vertex_name" : "dl_name";
    }
    if (command == G_SETTIMG_OTR_FILEPATH || command == G_VTX_OTR_FILEPATH || command == G_DL_OTR_FILEPATH) {
        name = reinterpret_cast<const char*>(gfx->words.w1);
        label = command == G_SETTIMG_OTR_FILEPATH ? "dl_texture_name" :
                command == G_VTX_OTR_FILEPATH ? "dl_vertex_name" : "dl_name";
        if (command == G_VTX_OTR_FILEPATH && index + 1 < resource->Instructions.size()) {
            const auto& next = resource->Instructions[index + 1];
            field("Num VTX", next.words.w0);
            field("Offset", next.words.w1 >> 16);
            field("Data Offset", next.words.w1 & 0xFFFF);
        }
    }
    if (name && label) text += "\n" + N::Text(label) + ": " + name;
    return text;
}

N::PagePtr InstructionPage(std::shared_ptr<Fast::DisplayList> resource, size_t index) {
    return N::MakePage("dl/instruction/" + std::to_string(index), N::Text("dl_instruction"), [resource, index] {
        std::vector<N::Row> rows;
        if (index >= resource->Instructions.size()) return rows;
        auto* gfx = reinterpret_cast<Gfx*>(&resource->Instructions[index]);
        const int command = gfx->words.w0 >> 24;
        rows.push_back(N::Choice("command", N::Text("dl_command"), command,
            {{G_SETPRIMCOLOR, cmdMap.at(G_SETPRIMCOLOR)}, {G_SETENVCOLOR, cmdMap.at(G_SETENVCOLOR)},
             {G_RDPPIPESYNC, cmdMap.at(G_RDPPIPESYNC)}, {G_SETGRAYSCALE, cmdMap.at(G_SETGRAYSCALE)},
             {G_SETINTENSITY, cmdMap.at(G_SETINTENSITY)}}, [resource, index](int value) {
                auto* current = reinterpret_cast<Gfx*>(&resource->Instructions[index]);
                const auto words = InstructionWords(current->words.w0 >> 24);
                switch (value) {
                    case G_SETPRIMCOLOR: *current = gsDPSetPrimColor(0, 0, 0, 0, 0, 255); break;
                    case G_SETENVCOLOR: *current = gsDPSetEnvColor(0, 0, 0, 255); break;
                    case G_RDPPIPESYNC: *current = gsDPPipeSync(); break;
                    case G_SETGRAYSCALE: *current = gsSPGrayscale(true); break;
                    case G_SETINTENSITY: *current = gsDPSetGrayscaleColor(0, 0, 0, 255); break;
                    default: return;
                }
                for (size_t i = 1; i < words && index + i < resource->Instructions.size(); ++i)
                    *reinterpret_cast<Gfx*>(&resource->Instructions[index + i]) = gsDPPipeSync();
            }));
        if (command == G_SETPRIMCOLOR || command == G_SETENVCOLOR || command == G_SETINTENSITY) {
            const char* channels[] = {"red", "green", "blue", "alpha"};
            for (int channel = 0; channel < 4; ++channel) {
                const int shift = 24 - channel * 8;
                rows.push_back(N::Integer(channels[channel], N::Text(channels[channel]),
                    (gfx->words.w1 >> shift) & 255, 0, 255, 1, [resource, index, shift](int value) {
                        auto& word = resource->Instructions[index].words.w1;
                        word = (word & ~(static_cast<uintptr_t>(255) << shift)) |
                               (static_cast<uintptr_t>(value) << shift);
                    }));
            }
        }
        if (command == G_SETGRAYSCALE)
            rows.push_back(N::Toggle("state", N::Text("dl_state"), gfx->words.w1 != 0,
                [resource, index](bool value) { resource->Instructions[index].words.w1 = value; }));
        rows.push_back(N::Action("details", N::Text("read_details"), [] { N::ReadCurrentDescription(); },
                                 Description(resource, index)));
        return rows;
    });
}

N::PagePtr DisplayListPage(const std::string& name) {
    std::shared_ptr<Fast::DisplayList> resource;
    try {
        resource = std::dynamic_pointer_cast<Fast::DisplayList>(
            Ship::Context::GetInstance()->GetResourceManager()->LoadResource(name));
    } catch (const std::exception& error) {
        SPDLOG_ERROR("Display List Viewer failed to load {}: {}", name, error.what());
    }
    return N::MakePage("dl/" + name, name, [resource] {
        std::vector<N::Row> rows;
        if (!resource) {
            rows.push_back(N::Action("error", N::Text("dl_load_error"), [] { N::ReadCurrentDescription(); }));
            return rows;
        }
        auto count = N::Action("count", N::Text("dl_instruction_count"), [] { N::ReadCurrentDescription(); });
        count.value = std::to_string(resource->Instructions.size());
        rows.push_back(std::move(count));
        for (size_t index = 0; index < resource->Instructions.size();) {
            const int command = resource->Instructions[index].words.w0 >> 24;
            const auto words = InstructionWords(command);
            if (words > resource->Instructions.size() - index) break;
            if (const auto found = cmdMap.find(command); found != cmdMap.end())
                rows.push_back(N::Link("instruction/" + std::to_string(index), found->second,
                    [resource, index] { return InstructionPage(resource, index); },
                    std::to_string(index) + "\n" + Description(resource, index)));
            index += words;
        }
        return rows;
    });
}

N::PagePtr SearchResultsPage() {
    std::vector<std::string> names;
    const auto results = Ship::Context::GetInstance()->GetResourceManager()->GetArchiveManager()->ListFiles("*" + search + "*DL*");
    for (const auto& name : *results)
        if (name.ends_with("DL") || name.find("DL_") != std::string::npos) names.push_back(name);
    std::sort(names.begin(), names.end(), [](const auto& a, const auto& b) {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
            [](unsigned char x, unsigned char y) { return std::tolower(x) < std::tolower(y); });
    });
    return N::MakePage("dl/results", N::Text("dl_active"), [names] {
        std::vector<N::Row> rows;
        for (const auto& name : names)
            rows.push_back(N::Link(name, name, [name] { return DisplayListPage(name); }));
        return rows;
    });
}
}

void InitializeDisplayListViewer() {
    N::RegisterPage("Display List Viewer", [] {
        return N::MakePage("advanced/dl", N::Text("dl_viewer"), [] {
            std::vector<N::Row> rows{
                N::String("search", N::Text("dl_search"), search, [](std::string value) { search = std::move(value); }),
                N::Link("lists", N::Text("dl_active"), SearchResultsPage)};
            if (CVarGetInteger(CVAR_SETTING("DisableChanges"), 0)) N::Disable(rows, N::Text("race_disabled"));
            return rows;
        });
    }, N::Text("dl_viewer"));
}
