configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/ydotool.service.in"
    "${PROJECT_BINARY_DIR}/ydotool.service"
    @ONLY)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/70-ydotool.rules"
    "${PROJECT_BINARY_DIR}/70-ydotool.rules"
    COPYONLY)
install(FILES "${PROJECT_BINARY_DIR}/ydotool.service" DESTINATION ${SD_UNITDIR})
install(FILES "${PROJECT_BINARY_DIR}/70-ydotool.rules" DESTINATION ${SD_UNITDIR})
