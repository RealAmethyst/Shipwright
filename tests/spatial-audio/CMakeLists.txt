cmake_minimum_required(VERSION 3.26)
project(spatial_audio_checks LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(STEAM_AUDIO_SDK "" CACHE PATH "Extracted Steam Audio 4.8.1 SDK")
if(NOT EXISTS "${STEAM_AUDIO_SDK}/include/phonon.h")
    message(FATAL_ERROR "Set STEAM_AUDIO_SDK to the verified Steam Audio 4.8.1 SDK")
endif()
add_executable(spatial_audio_checks spatial_audio_checks.cpp
    ../../soh/soh/Enhancements/audio/spatial/BinauralMixer.cpp
    ../../soh/soh/Enhancements/audio/spatial/SpeakerPanner.cpp)
target_include_directories(spatial_audio_checks PRIVATE "${STEAM_AUDIO_SDK}/include"
    ../../soh/soh/Enhancements/audio/spatial)
target_compile_definitions(spatial_audio_checks PRIVATE SOH_STEAM_AUDIO)
if(WIN32)
    add_executable(spatial_output_probe spatial_output_probe.cpp)
    target_link_libraries(spatial_output_probe PRIVATE ole32 uuid)
    target_link_libraries(spatial_audio_checks PRIVATE "${STEAM_AUDIO_SDK}/lib/windows-x64/phonon.lib")
    add_custom_command(TARGET spatial_audio_checks POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${STEAM_AUDIO_SDK}/lib/windows-x64/phonon.dll" "$<TARGET_FILE_DIR:spatial_audio_checks>")
else()
    find_library(STEAM_AUDIO_LIBRARY phonon REQUIRED PATHS "${STEAM_AUDIO_SDK}/lib/linux-x64")
    target_link_libraries(spatial_audio_checks PRIVATE "${STEAM_AUDIO_LIBRARY}")
endif()
enable_testing()
add_executable(cue_mixer_checks cue_mixer_checks.cpp ../../soh/soh/Enhancements/audio/spatial/CueMixer.cpp)
target_include_directories(cue_mixer_checks PRIVATE ../../soh/soh/Enhancements/audio/spatial)
add_test(NAME cue_mixer_checks COMMAND cue_mixer_checks "${CMAKE_CURRENT_SOURCE_DIR}/../../dependencies/accessibility-audio")
add_test(NAME spatial_audio_checks COMMAND spatial_audio_checks "${CMAKE_CURRENT_SOURCE_DIR}/../../dependencies/steam-audio/cipic_124.sofa")
set(SPATIAL_VCPKG "" CACHE PATH "Existing project vcpkg x64-windows-static triplet")
if(SPATIAL_VCPKG)
    add_executable(wall_compass_checks wall_compass_checks.cpp
        ../../soh/soh/Enhancements/audio/spatial/WallProbes.cpp
        ../../soh/soh/Enhancements/audio/spatial/WorldCompass.cpp)
    target_include_directories(wall_compass_checks PRIVATE ../../soh ../../soh/include ../../libultraship/include
        ../../soh/soh/Enhancements/audio/spatial "${SPATIAL_VCPKG}/include")
    add_test(NAME wall_compass_checks COMMAND wall_compass_checks)
    if(MSVC)
        target_compile_options(wall_compass_checks PRIVATE /Zc:preprocessor /utf-8)
    endif()
    if(WIN32)
        add_executable(spatial_backend_probe spatial_backend_probe.cpp
            ../../libultraship/src/ship/audio/SpatialAudioPlayer.cpp
            ../../libultraship/src/ship/audio/AudioPlayer.cpp
            ../../libultraship/src/ship/audio/SoundMatrixDecoder.cpp)
        target_include_directories(spatial_backend_probe PRIVATE ../../libultraship/include "${SPATIAL_VCPKG}/include")
        target_compile_definitions(spatial_backend_probe PRIVATE NOMINMAX SDL_MAIN_HANDLED)
        target_compile_options(spatial_backend_probe PRIVATE /utf-8)
        target_link_libraries(spatial_backend_probe PRIVATE "${SPATIAL_VCPKG}/lib/SDL2-static.lib"
            "${SPATIAL_VCPKG}/lib/spdlog.lib" "${SPATIAL_VCPKG}/lib/fmt.lib"
            ole32 uuid winmm version imm32 setupapi)
    endif()
    add_executable(native_mixer_checks native_mixer_checks.cpp ../../soh/soh/mixer.c)
    target_include_directories(native_mixer_checks PRIVATE ../../soh ../../soh/include
        ../../libultraship/include "${SPATIAL_VCPKG}/include" "${SPATIAL_VCPKG}/include/opus")
    target_link_libraries(native_mixer_checks PRIVATE "${SPATIAL_VCPKG}/lib/opusfile.lib"
        "${SPATIAL_VCPKG}/lib/opus.lib" "${SPATIAL_VCPKG}/lib/ogg.lib")
    add_test(NAME native_mixer_checks COMMAND native_mixer_checks)
endif()
