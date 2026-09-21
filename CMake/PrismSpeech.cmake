# Speech Prism is unrelated to libultraship's prism shader processor.
include(FetchContent)
if(CMAKE_SYSTEM_NAME MATCHES "NintendoSwitch|CafeOS")
    return()
endif()

if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    FetchContent_Declare(prism_speech
        URL https://github.com/ethindp/prism/releases/download/v0.18.2/prism-windows-x64.zip
        URL_HASH SHA256=31c02e3ef2260b4d3b11fb00132f8eb12bc147b5fb17031b580670b117ba7d23
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(prism_speech)
    add_library(PrismSpeech SHARED IMPORTED)
    set_target_properties(PrismSpeech PROPERTIES
        IMPORTED_IMPLIB "${prism_speech_SOURCE_DIR}/dynamic/release/lib/prism.lib"
        IMPORTED_LOCATION "${prism_speech_SOURCE_DIR}/dynamic/release/bin/prism.dll"
        INTERFACE_INCLUDE_DIRECTORIES "${prism_speech_SOURCE_DIR}/include"
    )
    add_custom_command(TARGET soh POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:PrismSpeech>" "$<TARGET_FILE_DIR:soh>"
    )
    install(FILES "$<TARGET_FILE:PrismSpeech>" DESTINATION . COMPONENT ship)
    install(DIRECTORY "${prism_speech_SOURCE_DIR}/LICENSES/" DESTINATION licenses/prism COMPONENT ship)
else()
    find_package(prism 0.18.2 EXACT CONFIG REQUIRED)
    add_library(PrismSpeech INTERFACE)
    target_link_libraries(PrismSpeech INTERFACE prism::prism)
endif()
target_link_libraries(soh PRIVATE PrismSpeech)
target_compile_definitions(soh PRIVATE SOH_PRISM)
install(FILES "${CMAKE_SOURCE_DIR}/dependencies/prism/NOTICE.md" DESTINATION licenses/prism COMPONENT ship)
