#include "Navigation.h"
#include "NavigationTargets.h"
#include "WalkingQuery.h"
#include "InputCapture.h"
#include "StepEstimate.h"
#include "TraversalProgress.h"
#include "RouteFollower.h"
#include "SceneGraph.h"
#include "ApproachBounds.h"
#include "soh/Enhancements/enhancementTypes.h"
#include "soh/NativeOptions/NativeOptions.h"
#include "soh/Enhancements/audio/spatial/CueActors.h"
#include "soh/Enhancements/audio/spatial/WorldCompass.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/speechsynthesizer/SpeechSynthesizer.h"
#include "soh/Enhancements/tts/SpeechText.h"
#include <libultraship/libultraship.h>
#include <SDL.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <optional>
extern "C" {
#include "global.h"
}

namespace Navigation {
namespace {
constexpr uint64_t Voice = uint64_t{1} << 60;
constexpr int InputOwner = 0x4e4156;
std::vector<Target> targets;
Category category = Category::People;
std::optional<Target> destination;
RouteSearch search;
QueryStats searchStats;
RouteFollower follower;
SceneGraph graph;
float graphRadius = 0;
bool meshSearch = false, searchReported = false;
bool alternativeSearch = false;
enum class SearchMethod { Automatic, Surfaces, Detailed };
std::chrono::steady_clock::time_point searchBegan;
double searchWorkMs = 0;
Point searchOrigin{}, searchGoal{}, soundPosition{};
int scene = -1, room = -1;
InputCapture capture;
bool audioActive = false, started = false, suspended = false;
bool initialized = false;
float soundInterval = 0;
uint32_t previousButtons = 0, refreshAt = 0, repeatAt = 0;
int previousDirection = 0;
std::shared_ptr<NativeOptions::Page> page;

void Feedback(NativeOptions::Feedback feedback) {
    NativeOptions::GetModel().PlayFeedback(feedback);
}
NativeOptions::Model& Menu() {
    static NativeOptions::Model model([](const std::string& text, size_t position, size_t count) {
        return SpeechText::WithPosition(text, NativeOptions::Text("position"), static_cast<int>(position), static_cast<int>(count));
    }, Feedback);
    return model;
}
void Speak(const std::string& text, bool interrupt = true) {
    if (!text.empty() && CVarGetInteger(CVAR_SETTING("A11yTTS"), 1) && SpeechSynthesizer::Instance)
        SpeechSynthesizer::Instance->Speak(text.c_str(), "en-US", interrupt);
}
Point PlayerPosition(PlayState* play) {
    const auto& p = GET_PLAYER(play)->actor.world.pos;
    return {p.x, p.y, p.z};
}
bool Available(PlayState* play) {
    return play && GameInteractor::IsSaveLoaded() && GET_PLAYER(play) && !NativeOptions_IsOpen() &&
        (!ImGui::GetCurrentContext() || !ImGui::GetIO().AppFocusLost) &&
        !play->pauseCtx.state && play->transitionTrigger == TRANS_TRIGGER_OFF && !Play_InCsMode(play) &&
        Message_GetState(&play->msgCtx) == TEXT_STATE_NONE &&
        !(GET_PLAYER(play)->stateFlags1 & (PLAYER_STATE1_IN_CUTSCENE | PLAYER_STATE1_TALKING |
            PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_DEAD | PLAYER_STATE1_LOADING));
}
void Stop(bool announce) {
    const bool active = destination.has_value();
    destination.reset(); search.Reset(); follower.Reset();
    audioActive = started = suspended = false;
    SpatialAudio::GetCueMixer().Stop(Voice);
    if (announce && active) Speak(Text("stopped"));
}
void Close() {
    if (!Menu().IsOpen()) return;
    Menu().Close(); page.reset();
    capture.Close();
    Feedback(NativeOptions::Feedback::Back);
}
void Reset() {
    Close(); Stop(false); targets.clear(); ClearObstacles(); graph.Clear(); graphRadius = 0;
    scene = room = -1;
}
void Init() {
    if (initialized) return;
    initialized = true;
    InitTargets();
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>(Reset);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>([](int32_t) { Reset(); });
}

WalkingQuery Query(PlayState* play, QueryStats* diagnostics = nullptr) {
    JumpMotion motion;
    const auto* player = GET_PLAYER(play);
    // Hover boots, forced sliding and speed cheats use different movement paths.
    // Normal boots use these exact live values in func_8083A4A8 and Actor_UpdatePos.
    if (gGameInfo && player->currentBoots != PLAYER_BOOTS_HOVER && !GameInteractor_GetSlipperyFloorActive() &&
        GameInteractor_MovementSpeedMultiplier() == 1 &&
        !(player->currentMask == PLAYER_MASK_BUNNY &&
          CVarGetInteger(CVAR_ENHANCEMENT("MMBunnyHood"), BUNNY_HOOD_VANILLA) != BUNNY_HOOD_VANILLA) &&
        CVarGetFloat(CVAR_CHEAT("SpeedModifier.Value"), 1) == 1 && !player->meleeWeaponState &&
        !(player->stateFlags1 & (PLAYER_STATE1_FLOOR_DISABLED | PLAYER_STATE1_CARRYING_ACTOR |
                                 PLAYER_STATE1_HOSTILE_LOCK_ON)) && gSaveContext.respawn[RESPAWN_MODE_TOP].data <= 40 &&
        !(player->stateFlags2 & PLAYER_STATE2_CRAWLING)) {
        motion = {R_RUN_SPEED_LIMIT / 100.0f, REG(68) / 100.0f, R_UPDATE_RATE * 0.5f, REG(19) / 100.0f,
                  IREG(66) / 100.0f, IREG(67) / 100.0f, IREG(68) / 100.0f, IREG(69) / 1000.0f};
    }
    return WalkingQuery(play, motion, diagnostics, !GameInteractor_GetDisableLedgeGrabsActive());
}
void BeginSearch(PlayState* play, const char* reason, SearchMethod method = SearchMethod::Automatic) {
    if (!destination) return;
    audioActive = false;
    SpatialAudio::GetCueMixer().Stop(Voice);
    searchOrigin = PlayerPosition(play); searchGoal = destination->position;
    searchStats = {};
    auto query = Query(play, &searchStats);
    const auto target = *destination;
    searchBegan = std::chrono::steady_clock::now(); searchWorkMs = 0; searchReported = false;
    alternativeSearch = method == SearchMethod::Automatic;
    const bool useMesh = method == SearchMethod::Surfaces ||
        (method == SearchMethod::Automatic && PreferSceneGraph(searchOrigin, searchGoal));
    if (useMesh && (!graph.Ready() || graphRadius != query.Radius())) {
        const auto began = std::chrono::steady_clock::now();
        graph.Build(play, query.Radius()); graphRadius = query.Radius();
        SPDLOG_INFO("Navigation scene graph: {} candidates, {:.3f} ms", graph.Anchors(),
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - began).count());
    }
    meshSearch = useMesh && graph.Ready();
    RouteSearch::Expand expand;
    if (meshSearch) expand = [play, query, target](Point p, std::vector<Connection>& out) {
        graph.Expand(play, query, p, target.position, target.radius, out);
    };
    search.Begin(searchOrigin, searchGoal, target.radius, std::max(10.0f, query.Radius()),
        [query](Point a, Point b, Point& out) { return query.Walk(a, b, out); },
        [query, target](Point at) { return CanApproach(target, at, query); },
        [query](Point a, Point b, JumpLink& out) { return query.Traverse(a, b, out); }, std::move(expand),
        [query](Point p, Point& out) { return query.Support(p, out); });
    SPDLOG_INFO("Navigation search start: {}; {}; target {}; origin ({}, {}, {})", reason,
        meshSearch ? "native surfaces" : "detailed collision", target.name, searchOrigin.x, searchOrigin.y, searchOrigin.z);
}
void Select(uint64_t id) {
    const auto found = std::find_if(targets.begin(), targets.end(), [id](const Target& t) { return t.identity == id; });
    if (found == targets.end() || !Available(gPlayState)) return;
    Stop(false);
    destination = *found;
    Menu().Announce(Format("searching", found->name));
    BeginSearch(gPlayState, "selected");
}
std::string Description(const Target& target, PlayState* play) {
    if (!gGameInfo) return "";
    const Point player = PlayerPosition(play);
    const WalkingStride stride{R_RUN_SPEED_LIMIT / 100.0f, REG(48) / 100.0f, REG(35) / 1000.0f,
                               REG(36) / 1000.0f, REG(37) / 1000.0f, REG(38) / 1000.0f,
                               R_UPDATE_RATE * 0.5f};
    const int steps = stride.Estimate(Distance(player, target.position));
    if (steps < 0) return "";
    std::string result = steps == 0 ? Text("distance_near") : steps == 1 ? Text("distance_one") :
                         Format("distance", std::to_string(steps));
    if (SpatialAudio::HasMapCompass(play->sceneNum)) {
        const int direction = SpatialAudio::MapDirection(target.position.x - player.x, target.position.z - player.z,
            CVarGetInteger(CVAR_ENHANCEMENT("MirroredWorld"), 0) != 0, 8);
        constexpr const char* keys[]{"north", "northeast", "east", "southeast", "south", "southwest", "west", "northwest"};
        if (direction >= 0) result += ", " + Text(keys[direction]);
    }
    if (target.position.y - player.y > 40) result += ", " + Text("above");
    else if (player.y - target.position.y > 40) result += ", " + Text("below");
    return result;
}
std::vector<NativeOptions::Row> Rows() {
    std::vector<NativeOptions::Row> rows;
    if (!gPlayState || !GET_PLAYER(gPlayState)) return rows;
    for (const auto& target : targets) if (target.category == category) {
        NativeOptions::Row row;
        row.id = std::to_string(target.identity); row.label = target.name;
        row.description = Description(target, gPlayState);
        row.activate = [id = target.identity] { Select(id); };
        rows.push_back(std::move(row));
    }
    return rows;
}
bool SelectCategory(int direction, bool includeCurrent) {
    constexpr int count = static_cast<int>(Category::Count);
    int next = static_cast<int>(category);
    for (int i = 0; i < count; ++i) {
        if (i || !includeCurrent) next = (next + direction + count) % count;
        if (std::any_of(targets.begin(), targets.end(), [next](const Target& t) { return static_cast<int>(t.category) == next; })) {
            category = static_cast<Category>(next);
            return true;
        }
    }
    return false;
}
void Open(PlayState* play) {
    targets = CollectTargets(play);
    SelectCategory(1, true);
    page = std::make_shared<NativeOptions::Page>();
    page->id = "navigation"; page->title = Text("title"); page->section = CategoryName(category);
    page->hints = Text("hints"); page->footer = Text("footer"); page->rows = Rows;
    Menu().Open(page);
    if (Menu().Rows().empty()) {
        Menu().Announce(Text("empty"), false);
        Menu().Announce(page->hints, false);
        page->hints.clear();
    }
    Feedback(NativeOptions::Feedback::Confirm);
    capture.Open(); previousDirection = 0; refreshAt = SDL_GetTicks() + 500;
}

enum Button : uint32_t { Toggle = 1, Confirm = 2, Back = 4, Cancel = 8, Up = 16, Down = 32, Left = 64, Right = 128 };
struct RawInput { uint32_t buttons = 0; bool held = false; };
RawInput ReadInput() {
    RawInput result;
    auto devices = Ship::Context::GetInstance()->GetControlDeck()->GetConnectedPhysicalDeviceManager();
    for (const auto& [id, controller] : devices->GetConnectedSDLGamepadsForPort(0)) {
        for (const auto& [button, action] : {std::pair{SDL_CONTROLLER_BUTTON_RIGHTSTICK, Toggle},
             {SDL_CONTROLLER_BUTTON_A, Confirm}, {SDL_CONTROLLER_BUTTON_B, Back}, {SDL_CONTROLLER_BUTTON_X, Cancel},
             {SDL_CONTROLLER_BUTTON_DPAD_UP, Up}, {SDL_CONTROLLER_BUTTON_DPAD_DOWN, Down},
             {SDL_CONTROLLER_BUTTON_DPAD_LEFT, Left}, {SDL_CONTROLLER_BUTTON_DPAD_RIGHT, Right}})
            if (SDL_GameControllerGetButton(controller, button)) result.buttons |= action;
        for (int button = 0; button < SDL_CONTROLLER_BUTTON_MAX; ++button)
            result.held |= SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(button)) != 0;
        for (int axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; ++axis)
            result.held |= std::abs(static_cast<int>(SDL_GameControllerGetAxis(controller, static_cast<SDL_GameControllerAxis>(axis)))) > 10000;
    }
    if (ImGui::GetCurrentContext()) {
        for (const auto& [key, action] : {std::pair{ImGuiKey_F8, Toggle}, {ImGuiKey_Enter, Confirm},
             {ImGuiKey_Backspace, Back}, {ImGuiKey_Delete, Cancel}, {ImGuiKey_UpArrow, Up},
             {ImGuiKey_DownArrow, Down}, {ImGuiKey_LeftArrow, Left}, {ImGuiKey_RightArrow, Right}})
            if (ImGui::IsKeyDown(key)) result.buttons |= action;
        for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_GamepadStart; ++key)
            result.held |= ImGui::IsKeyDown(static_cast<ImGuiKey>(key));
        for (bool down : ImGui::GetIO().MouseDown) result.held |= down;
    }
    return result;
}
void ReportSearch(PlayState* play) {
    if (searchReported || search.State() == SearchState::Idle || search.State() == SearchState::Searching) return;
    searchReported = true;
    const auto position = PlayerPosition(play), closest = search.Closest();
    SPDLOG_INFO("Navigation route result {}: {} nodes; {} work {:.3f} ms, elapsed {:.3f} ms; scene {} room {}; target {} ({}, {}, {}); start ({}, {}, {}); current ({}, {}, {}); closest ({}, {}, {}); walks {} bodies {} traversals {}",
        static_cast<int>(search.State()), search.Visited(), meshSearch ? "surfaces" : "detailed", searchWorkMs,
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - searchBegan).count(),
        play->sceneNum, play->roomCtx.curRoom.num, destination->name, searchGoal.x, searchGoal.y, searchGoal.z,
        searchOrigin.x, searchOrigin.y, searchOrigin.z, position.x, position.y, position.z, closest.x, closest.y, closest.z,
        searchStats.walks, searchStats.bodies, searchStats.traversals);
    if (search.State() == SearchState::Found) return;
    constexpr const char* reasons[]{"floor", "hazard", "water", "actor", "bounds", "wall", "ceiling", "jump state",
        "edge", "jump floor", "runway", "early arc", "late arc", "landing", "departure floor", "rising floor",
        "landing support", "fall"};
    static_assert(std::size(reasons) == static_cast<size_t>(QueryFailure::Count));
    for (size_t i = 0; i < searchStats.rejected.size(); ++i)
        if (searchStats.rejected[i]) SPDLOG_INFO("Navigation rejected {}: {}", reasons[i], searchStats.rejected[i]);
}
void UpdateRoute(PlayState* play) {
    audioActive = false;
    if (!destination) return;
    const auto found = std::find_if(targets.begin(), targets.end(), [](const Target& t) { return t.identity == destination->identity; });
    if (found == targets.end()) {
        const auto position = PlayerPosition(play);
        const bool reached = destination->loosePickup && PickupApproach({position.x - destination->position.x,
            position.y - destination->position.y, position.z - destination->position.z});
        const auto name = destination->name; Stop(false); Speak(Format(reached ? "arrived" : "lost", name)); return;
    }
    destination = *found;
    const auto position = PlayerPosition(play);
    auto query = Query(play);
    const auto* player = GET_PLAYER(play);
    const bool grounded = (player->actor.bgCheckFlags & 1) != 0;
    const bool ledge = player->stateFlags1 & (PLAYER_STATE1_CLIMBING_LEDGE | PLAYER_STATE1_HANGING_OFF_LEDGE);
    const bool ladder = player->stateFlags1 & PLAYER_STATE1_CLIMBING_LADDER;
    const bool stable = grounded && !ledge && !ladder;
    if (started && stable && CanApproach(*destination, position, query)) {
        const auto name = destination->name; Stop(false); Speak(Format("arrived", name)); return;
    }
    if (suspended) {
        if (!stable) return;
        suspended = false;
        if (follower.HasRoute() && follower.Rejoin(position, query)) search.Reset();
        else BeginSearch(play, "resume disconnected");
    }
    if (Distance(destination->position, searchGoal) > 30 && stable) BeginSearch(play, "target moved");
    if (search.State() == SearchState::InvalidStart) {
        ReportSearch(play);
        Point supported;
        if (!stable || !query.Support(position, supported)) return;
        BeginSearch(play, "footing restored");
    }
    // Preserve a search while Link moves. Its completed route will be joined
    // from his current position; repeated 30-unit restarts starved the old search.
    // A connection to an old waypoint does not establish that the blocked
    // corridor beyond it reopened. Let a repair search finish as well.
    if (search.State() == SearchState::Searching) {
        const auto began = std::chrono::steady_clock::now();
        const auto deadline = began + std::chrono::milliseconds(6);
        do { search.Step(1); } while (search.State() == SearchState::Searching && std::chrono::steady_clock::now() < deadline);
        searchWorkMs += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - began).count();
        if (search.State() == SearchState::Searching) return;
    }
    ReportSearch(play);
    if (search.State() == SearchState::Unreachable || search.State() == SearchState::Limit) {
        if (!stable) { suspended = true; return; }
        if (alternativeSearch) {
            BeginSearch(play, "trying alternate search", meshSearch ? SearchMethod::Detailed : SearchMethod::Surfaces); return;
        }
        // A negative result from an old origin is not a result for Link's new
        // location. Retain the destination and search from the supported position.
        if (Distance(position, searchOrigin) > query.Radius()) { BeginSearch(play, "origin moved during search"); return; }
        const auto name = destination->name;
        const bool limited = search.State() == SearchState::Limit;
        Stop(false); Speak(Format(limited ? "search_limit" : "unreachable", name)); return;
    }
    if (search.State() == SearchState::Found) {
        if (!stable) return;
        follower.Set(search);
        search.Reset();
        if (!follower.Rejoin(position, query)) { BeginSearch(play, "completed route needs new origin"); return; }
        if (!started) { Close(); Speak(Format("started", destination->name)); started = true; }
    }
    if (!follower.HasRoute() || Menu().IsOpen()) return;
    const auto result = follower.Update(position, grounded, ledge, ladder, query);
    if (result.state == FollowState::Waiting) return;
    if (result.state == FollowState::Replan) {
        if (stable) BeginSearch(play, result.reason);
        else suspended = true;
        return;
    }
    soundPosition = result.cue;
    soundInterval = BeepInterval(Distance(position, destination->position));
    audioActive = soundInterval > 0;
}
}
}

extern "C" int Navigation_IsOpen() { return Navigation::Menu().IsOpen(); }
extern "C" int Navigation_HasRoute() { return Navigation::destination.has_value(); }

extern "C" void Navigation_UpdateInput(GameState* state) {
    using namespace Navigation;
    Init();
    const auto input = ReadInput();
    const auto pressed = input.buttons & ~previousButtons;
    previousButtons = input.buttons;
    auto deck = Ship::Context::GetInstance()->GetControlDeck();
    if (ImGui::GetCurrentContext() && ImGui::GetIO().AppFocusLost) capture.Close();
    auto* play = gPlayState && state == &gPlayState->state ? gPlayState : nullptr;
    if (!play || !GameInteractor::IsSaveLoaded()) {
        Reset();
    } else {
        if (scene != play->sceneNum || room != play->roomCtx.curRoom.num) {
            Reset(); scene = play->sceneNum; room = play->roomCtx.curRoom.num;
        }
        if (!Available(play)) {
            Close(); audioActive = false; suspended = destination.has_value();
        } else {
            if ((pressed & Toggle) && capture.CanToggle()) {
                if (Menu().IsOpen()) Close(); else Open(play);
            } else if (Menu().IsOpen()) {
                if (capture.Ready(input.held)) {
                    if (pressed & Back) Close();
                    else if (pressed & Cancel) Stop(true);
                    else if (pressed & Confirm) Menu().Navigate(NativeOptions::Navigation::Confirm);
                    else {
                        const int direction = input.buttons & Up ? -1 : input.buttons & Down ? 1 :
                                              input.buttons & Left ? -2 : input.buttons & Right ? 2 : 0;
                        if (direction && (direction != previousDirection || static_cast<int32_t>(SDL_GetTicks() - repeatAt) >= 0)) {
                            if (std::abs(direction) == 1) Menu().Navigate(direction < 0 ? NativeOptions::Navigation::Up : NativeOptions::Navigation::Down);
                            else if (SelectCategory(direction < 0 ? -1 : 1, false)) {
                                Feedback(NativeOptions::Feedback::Cursor); Menu().SetSection(CategoryName(category));
                            }
                            repeatAt = SDL_GetTicks() + (direction != previousDirection ? 350 : 120);
                        }
                        previousDirection = direction;
                    }
                }
                if (Menu().IsOpen() && static_cast<int32_t>(SDL_GetTicks() - refreshAt) >= 0) {
                    targets = CollectTargets(play);
                    if (!std::any_of(targets.begin(), targets.end(), [](const Target& t) { return t.category == category; }) && SelectCategory(1, true))
                        Menu().SetSection(CategoryName(category));
                    else Menu().Refresh();
                    refreshAt = SDL_GetTicks() + 500;
                }
            }
            if (destination) {
                targets = CollectTargets(play);
                const auto* player = GET_PLAYER(play);
                const auto* traversal = player->ageProperties ?
                    follower.Planned(PlayerPosition(play), player->ageProperties->wallCheckRadius) : nullptr;
                const bool plannedLedge = traversal && traversal->kind == Traversal::Ledge;
                const bool plannedLadder = traversal && IsLadder(traversal->kind);
                if ((player->stateFlags1 & (PLAYER_STATE1_ON_HORSE | PLAYER_STATE1_IN_WATER)) ||
                    ((player->stateFlags1 & PLAYER_STATE1_CLIMBING_LADDER) && !plannedLadder) ||
                    ((player->stateFlags1 & PLAYER_STATE1_HANGING_OFF_LEDGE) && !plannedLedge)) {
                    audioActive = false; suspended = true;
                } else UpdateRoute(play);
            }
        }
    }
    for (const auto& speech : Menu().TakeSpeech()) Speak(speech.text, speech.interrupt);
    if (capture.Consume(Menu().IsOpen(), input.held)) {
        std::memset(state->input, 0, sizeof(state->input));
    }
    if (capture.BlocksGame(Menu().IsOpen())) deck->BlockGameInput(InputOwner);
    else deck->UnblockGameInput(InputOwner);
}

extern "C" void Navigation_Draw(GraphicsContext* gfxCtx) {
    if (Navigation_IsOpen()) NativeOptions::DrawModel(gfxCtx, Navigation::Menu(), Navigation::Text("footer"));
}
extern "C" void Navigation_PublishAudio(PlayState* play) {
    using namespace Navigation;
    auto& mixer = SpatialAudio::GetCueMixer();
    const float gain = CVarGetInteger(CVAR_SETTING("A11yAudio.pathfinder.Enabled"), 1) ?
        std::clamp(CVarGetInteger(CVAR_SETTING("A11yAudio.pathfinder.Volume"), 50), 0, 100) / 100.0f : 0;
    if (!audioActive || !Available(play) || Navigation_IsOpen() || gain <= 0) { mixer.Stop(Voice); return; }
    const auto source = SpatialAudio_WorldSource(Voice, soundPosition.x, soundPosition.y, soundPosition.z);
    if (!source.identity || !mixer.KeepPlayingInterval(Voice, SpatialAudio::Cue::Pathfinder, soundInterval)) {
        mixer.Stop(Voice); return;
    }
    mixer.Update(Voice, source, gain);
}
