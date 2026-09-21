include(FetchContent)
if(CMAKE_SYSTEM_NAME MATCHES "NintendoSwitch|CafeOS")
    return()
endif()

FetchContent_Declare(steam_audio
    URL https://github.com/ValveSoftware/steam-audio/releases/download/v4.8.1/steamaudio_4.8.1.zip
    URL_HASH SHA256=4a0aa5ec1176f38f0b0993a37c2259d9e86f27e22d5e24f83ec4c3cb9a1d5449
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(steam_audio)

if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(steam_audio_platform windows-x64)
    set(steam_audio_runtime phonon.dll)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(steam_audio_platform osx)
    set(steam_audio_runtime libphonon.dylib)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    set(steam_audio_platform linux-x64)
    set(steam_audio_runtime libphonon.so)
else()
    message(WARNING "Steam Audio HRTF is unavailable on this target; retaining native audio support")
    return()
endif()

add_library(SteamAudio SHARED IMPORTED)
set_target_properties(SteamAudio PROPERTIES
    IMPORTED_LOCATION "${steam_audio_SOURCE_DIR}/lib/${steam_audio_platform}/${steam_audio_runtime}"
    INTERFACE_INCLUDE_DIRECTORIES "${steam_audio_SOURCE_DIR}/include")
if(WIN32)
    set_target_properties(SteamAudio PROPERTIES
        IMPORTED_IMPLIB "${steam_audio_SOURCE_DIR}/lib/${steam_audio_platform}/phonon.lib")
endif()
target_link_libraries(soh PRIVATE SteamAudio)
target_compile_definitions(soh PRIVATE SOH_STEAM_AUDIO)
add_custom_command(TARGET soh POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:SteamAudio>" "$<TARGET_FILE_DIR:soh>")
add_custom_command(TARGET soh POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:soh>/accessibility/audio"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/dependencies/steam-audio/cipic_124.sofa"
        "$<TARGET_FILE_DIR:soh>/accessibility/audio/cipic_124.sofa")
install(FILES "${CMAKE_SOURCE_DIR}/dependencies/steam-audio/cipic_124.sofa"
    DESTINATION accessibility/audio COMPONENT ship)
install(FILES "$<TARGET_FILE:SteamAudio>" DESTINATION . COMPONENT ship)
install(FILES "${steam_audio_SOURCE_DIR}/THIRDPARTY.md" DESTINATION licenses/steam-audio COMPONENT ship)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/dependencies/steam-audio/" DESTINATION licenses/steam-audio
    COMPONENT ship FILES_MATCHING PATTERN "*.md")
