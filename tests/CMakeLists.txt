if(BUILD_TESTS)
    set(CATCH2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/catch2")

    if(EXISTS "${CATCH2_DIR}/CMakeLists.txt")
        add_subdirectory(${CATCH2_DIR})
        add_executable(tests
            main.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/bayes-tests/bayes-tests.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/search-tests/heap-tests.cpp
        )
        target_enable_clang_tidy(tests)
        target_enable_sanitizers(tests)
        target_include_directories(tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(tests PRIVATE search_core)

        if(BUILD_TESTS_WITH_MAIN)
            target_link_libraries(tests
                PRIVATE Catch2::Catch2WithMain
                PRIVATE bayes_core
            )
            target_compile_definitions(tests PRIVATE
                BAYES_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/bayes"
            )
        else()
            target_link_libraries(tests PUBLIC Catch2::Catch2)
        endif()

        include(Catch)
        catch_discover_tests(tests)
    else()
        message(FATAL_ERROR
            "Catch2 submodule not found at ${CATCH2_DIR}. "
            "Run: git submodule update --init --recursive"
        )
    endif()
endif()
