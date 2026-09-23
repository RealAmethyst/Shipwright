cmake_minimum_required(VERSION 3.26)
project(navigation_checks LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(NAV_VCPKG "" CACHE PATH "Existing x64-windows-static dependency triplet")
set(NAV_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../../soh/soh/Enhancements/navigation")
add_executable(navigation_checks navigation_checks.cpp "${NAV_SOURCE}/RouteSearch.cpp" "${NAV_SOURCE}/WalkingQuery.cpp")
target_include_directories(navigation_checks PRIVATE "${NAV_SOURCE}" ../../soh ../../soh/include
    ../../libultraship/include "${NAV_VCPKG}/include")
if(MSVC)
    target_compile_options(navigation_checks PRIVATE /Zc:preprocessor /utf-8)
endif()
set(NAV_KOKIRI_FIXTURE "" CACHE FILEPATH "Local extracted Kokiri collision resource; never package this file")
if(EXISTS "${NAV_KOKIRI_FIXTURE}")
    add_test(NAME scene_crawlspace COMMAND scene_navigation_checks "${NAV_KOKIRI_FIXTURE}" 0 0 500 -784 120 1046)
    add_test(NAME scene_raised_rupee COMMAND scene_navigation_checks "${NAV_KOKIRI_FIXTURE}" 0 0 500 -364 53 -783 --ledge)
    add_test(NAME scene_house_platform COMMAND scene_navigation_checks "${NAV_KOKIRI_FIXTURE}" -31 100 1073 -784 120 1046 --ladder)
    add_test(NAME scene_porch_shop COMMAND scene_navigation_checks "${NAV_KOKIRI_FIXTURE}" -31 100 1073 863 1 -416 --ladder)
    add_test(NAME scene_ground_porch COMMAND scene_navigation_checks "${NAV_KOKIRI_FIXTURE}" 0 0 500 -31 100 1073 --ladder)
    add_test(NAME scene_porch_heart COMMAND scene_navigation_checks "${NAV_KOKIRI_FIXTURE}" -31 100 1073 451 200 810 --roof)
    add_test(NAME scene_porch_lost_woods COMMAND scene_navigation_checks "${NAV_KOKIRI_FIXTURE}" -31 100 1073 -322 372 -1791.67 --ladder)
    add_test(NAME scene_lost_woods_return COMMAND scene_navigation_checks "${NAV_KOKIRI_FIXTURE}" -322 372 -1770 -31 100 1073 --ladder)
    add_test(NAME scene_native_floor_landing COMMAND scene_navigation_checks "${NAV_KOKIRI_FIXTURE}" -519 200 -1030 -533 200 -1030 --jump)
    add_test(NAME scene_wall_floor_drop COMMAND scene_navigation_checks "${NAV_KOKIRI_FIXTURE}" -519 200 -1030 -533 200 -1030 --walls)
    add_test(NAME scene_wall_porch_ladder COMMAND scene_navigation_checks "${NAV_KOKIRI_FIXTURE}" -31 100 1073 -31 100 1000 --walls)
    add_test(NAME scene_entrances COMMAND scene_navigation_checks "${NAV_KOKIRI_FIXTURE}" -31 100 1073 0 0 0 --survey)
endif()
enable_testing()
add_test(NAME navigation_checks COMMAND navigation_checks)
set(NAV_EXTRA_SCENES "" CACHE PATH "Local extra collision fixtures; never package")
foreach(scene IN ITEMS spot00_scene spot01_scene ydan_scene)
    if(EXISTS "${NAV_EXTRA_SCENES}/${scene}.collision")
        add_test(NAME "native_${scene}" COMMAND scene_navigation_checks "${NAV_EXTRA_SCENES}/${scene}.collision" 0 0 0 0 0 0 --walk-survey)
        if(scene STREQUAL "spot00_scene")
            set(scene_id 81)
        elseif(scene STREQUAL "spot01_scene")
            set(scene_id 82)
        else()
            set(scene_id 0)
        endif()
        set_tests_properties("native_${scene}" PROPERTIES ENVIRONMENT "NAV_SCENE_ID=${scene_id}")
    endif()
endforeach()

# Exercise the production queries against extracted scene data, using the game's
# actual collision implementation. Fixtures stay outside source and packages.
add_executable(scene_navigation_checks scene_navigation_checks.cpp
    "${NAV_SOURCE}/RouteSearch.cpp" "${NAV_SOURCE}/WalkingQuery.cpp"
    "${NAV_SOURCE}/SceneGraph.cpp"
    ../../soh/soh/Enhancements/audio/spatial/WallProbes.cpp
    ../../soh/src/code/z_bgcheck.c ../../soh/src/code/sys_math3d.c ../../soh/src/code/z_lib.c
    ../../soh/src/code/z_skin_matrix.c ../../soh/src/code/sys_math_atan.c
    ../../soh/src/libultra/gu/sins.c ../../soh/src/libultra/gu/coss.c ../../soh/src/libultra/gu/sintable.c)
target_include_directories(scene_navigation_checks PRIVATE "${NAV_SOURCE}" ../../soh ../../soh/include
    ../../libultraship/include "${NAV_VCPKG}/include")
target_compile_definitions(scene_navigation_checks PRIVATE LOG_LEVEL_GAME_PRINTS=6
    CVAR_PREFIX_CHEAT="gCheats" CVAR_PREFIX_ENHANCEMENT="gEnhancements")
if(MSVC)
    target_compile_options(scene_navigation_checks PRIVATE /Zc:preprocessor /utf-8 /Gy)
    target_link_options(scene_navigation_checks PRIVATE /OPT:REF)
endif()
