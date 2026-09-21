cmake_minimum_required(VERSION 3.26)
project(speech_checks LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
find_package(spdlog CONFIG REQUIRED)
set(PRISM_SDK "" CACHE PATH "Extracted Prism 0.18.2 platform SDK")
if(NOT EXISTS "${PRISM_SDK}/include/prism.h")
    message(FATAL_ERROR "Set PRISM_SDK to the extracted Prism release")
endif()

add_executable(speech_checks speech_checks.cpp
    ../../soh/soh/Enhancements/speechsynthesizer/SpeechSynthesizer.cpp)
target_include_directories(speech_checks PRIVATE "${PRISM_SDK}/include"
    ../../soh/soh/Enhancements/speechsynthesizer)
target_compile_definitions(speech_checks PRIVATE SOH_PRISM PRISM_STATIC)
target_link_libraries(speech_checks PRIVATE spdlog::spdlog)
if(MSVC)
    target_compile_options(speech_checks PRIVATE /utf-8)
endif()
enable_testing()
add_test(NAME speech_checks COMMAND speech_checks)

if(WIN32)
    add_executable(prism_probe prism_probe.cpp)
    target_include_directories(prism_probe PRIVATE "${PRISM_SDK}/include")
    target_link_libraries(prism_probe PRIVATE "${PRISM_SDK}/dynamic/release/lib/prism.lib")
    add_custom_command(TARGET prism_probe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PRISM_SDK}/dynamic/release/bin/prism.dll" "$<TARGET_FILE_DIR:prism_probe>")
endif()
