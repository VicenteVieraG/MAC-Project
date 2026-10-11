add_executable(${CMAKE_PROJECT_NAME} main.cpp)
target_enable_clang_tidy(${CMAKE_PROJECT_NAME})
target_enable_sanitizers(${CMAKE_PROJECT_NAME})

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE ./
    PRIVATE ${CMAKE_BINARY_DIR}/configured_files/includes
)
target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE window
    PRIVATE appLayout
)

target_set_warnings(
    ${CMAKE_PROJECT_NAME}
    ${ENABLE_WARNINGS_AS_ERRORS}
)

# Bundle runtime resources beside the executable
add_custom_target(${CMAKE_PROJECT_NAME}_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/assets"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/data"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/data/dataset.csv"
        "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/data/dataset.csv"
    COMMENT "Copying fonts, images and dataset beside the executable"
    VERBATIM
)
add_dependencies(${CMAKE_PROJECT_NAME} ${CMAKE_PROJECT_NAME}_assets)
