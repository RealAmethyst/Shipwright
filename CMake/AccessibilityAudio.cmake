set(accessibility_cue_files)
foreach(cue item person door transition destructible crawlspace ladder elevator pathfinder
    wall_north wall_east wall_south wall_west)
    list(APPEND accessibility_cue_files "${CMAKE_SOURCE_DIR}/dependencies/accessibility-audio/${cue}.wav")
endforeach()
add_custom_command(TARGET soh POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:soh>/accessibility/audio"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${accessibility_cue_files} "$<TARGET_FILE_DIR:soh>/accessibility/audio")
install(FILES ${accessibility_cue_files} DESTINATION accessibility/audio COMPONENT ship)
install(FILES "${CMAKE_SOURCE_DIR}/dependencies/accessibility-audio/manifest.json"
    "${CMAKE_SOURCE_DIR}/dependencies/accessibility-audio/NOTICE.md"
    DESTINATION licenses/accessibility-audio COMPONENT ship)
