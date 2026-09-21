cmake_minimum_required(VERSION 3.24)
project(NativeOptionsChecks LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
find_package(nlohmann_json CONFIG REQUIRED)
enable_testing()
set(OPTIONS_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../../soh/soh/NativeOptions")
add_executable(native-options-checks checks.cpp display_list_checks.cpp
    "${OPTIONS_SOURCE}/OptionsModel.cpp"
    "${OPTIONS_SOURCE}/OptionsControls.cpp"
    "${OPTIONS_SOURCE}/OptionsFormatting.cpp"
    "${OPTIONS_SOURCE}/OptionsFileIO.cpp"
    "${OPTIONS_SOURCE}/OptionsLayout.cpp"
    "${OPTIONS_SOURCE}/OptionsDisplayList.cpp"
    "${OPTIONS_SOURCE}/OptionsTextInput.cpp")
target_include_directories(native-options-checks PRIVATE "${OPTIONS_SOURCE}")
target_include_directories(native-options-checks PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../soh")
target_include_directories(native-options-checks PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../libultraship/include")
target_link_libraries(native-options-checks PRIVATE nlohmann_json::nlohmann_json)
target_compile_definitions(native-options-checks PRIVATE
    F3DEX_GBI_2
    OPTIONS_TEXT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../../soh/assets/custom/accessibility/texts/options_eng.json")
add_test(NAME native-options-navigation COMMAND native-options-checks)

set(IMGUI_SOURCE "" CACHE PATH "The ImGui source directory used by the game build")
if(IMGUI_SOURCE)
    add_executable(native-options-overlay-checks overlay_checks.cpp
        "${IMGUI_SOURCE}/imgui.cpp" "${IMGUI_SOURCE}/imgui_draw.cpp"
        "${IMGUI_SOURCE}/imgui_tables.cpp" "${IMGUI_SOURCE}/imgui_widgets.cpp")
    target_include_directories(native-options-overlay-checks PRIVATE "${OPTIONS_SOURCE}" "${IMGUI_SOURCE}")
    add_test(NAME native-options-overlays COMMAND native-options-overlay-checks)
endif()
