cmake_minimum_required(VERSION 3.31)

project(eventprocessor VERSION 1.0.0 LANGUAGES CXX)

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_23)

target_include_directories(${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

# example
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    include(cmake/EventprocessorSetupEnvironment.cmake)
    eventprocessor_setup_environment()

    include(cmake/CompilerOptionsTarget.cmake)
    include(cmake/SanitizersOptionsTarget.cmake)
    include(cmake/SetCompileCommands.cmake)

    add_executable(test-sanity-check src/test_sanity_check.cpp)
    target_link_libraries(
        test-sanity-check
        PRIVATE eventprocessor
                eventprocessor-internal-compile-options
    )

    enable_testing()
    add_test(test-sanity-check test-sanity-check)
endif()
