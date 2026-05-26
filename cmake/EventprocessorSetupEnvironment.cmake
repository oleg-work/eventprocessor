include_guard(GLOBAL)

set_property(GLOBAL PROPERTY eventprocessor_cmake_dir "${CMAKE_CURRENT_LIST_DIR}")

function(_eventprocessor_setup_environment_impl)
    if(EVENTPROCESSOR_IMPL_SETUP_ENV_WAS_RUN_FOR_THIS_DIR)
        return()
    endif()
    set(EVENTPROCESSOR_IMPL_SETUP_ENV_WAS_RUN_FOR_THIS_DIR
        ON
        PARENT_SCOPE
    )

    get_property(EVENTPROCESSOR_CMAKE_DIR GLOBAL PROPERTY eventprocessor_cmake_dir)

    message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")

    set(CMAKE_CXX_STANDARD
        23
        PARENT_SCOPE
    )

    message(STATUS "C++ standard ${CMAKE_CXX_STANDARD}")

    set(CMAKE_CXX_STANDARD_REQUIRED
        ON
        PARENT_SCOPE
    )
    set(CMAKE_CXX_EXTENSIONS
        OFF
        PARENT_SCOPE
    )
    set(CMAKE_VISIBILITY_INLINES_HIDDEN
        ON
        PARENT_SCOPE
    )

    if(NOT DEFINED CMAKE_CXX_SCAN_FOR_MODULES)
        # save ourselves the overhead of scanning every source file
        set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
    endif()

    add_compile_options("-pipe")

    message(STATUS "Linker global flags: ${CMAKE_EXE_LINKER_FLAGS}")

    option(EVENTPROCESSOR_USE_CCACHE "Use ccache for build" ON)
    if(EVENTPROCESSOR_USE_CCACHE)
        find_program(CCACHE_EXECUTABLE ccache)
        if(CCACHE_EXECUTABLE)
            message(STATUS "ccache: enabled")

            # Ccache 4.7.4 manual says -fno-pch-timestamp is required for Clang
            foreach(lang IN ITEMS C CXX OBJC OBJCXX)
                if(CMAKE_${lang}_COMPILER_ID MATCHES "Clang")
                    add_compile_options(
                        "$<$<COMPILE_LANGUAGE:${lang}>:SHELL: -Xclang -fno-pch-timestamp>"
                    )
                endif()
            endforeach()

            # Use a cache variable so the user can override this
            set(CCACHE_ENV
                CCACHE_SLOPPINESS=pch_defines,time_macros
                CACHE
                    STRING
                    "List of environment variables for ccache, each in key=value form"
            )

            message(STATUS "Setting compiler launcher: ${CCACHE_EXECUTABLE}")
            if(CMAKE_GENERATOR MATCHES "Ninja|Makefiles")
                foreach(lang IN ITEMS C CXX OBJC OBJCXX CUDA)
                    set(CMAKE_${lang}_COMPILER_LAUNCHER
                        ${CMAKE_COMMAND} -E env ${CCACHE_ENV}
                        ${CCACHE_EXECUTABLE}
                        PARENT_SCOPE
                    )
                endforeach()
            endif()
        else()
            message(STATUS "ccache: enabled, but not found")
        endif()
    else()
        message(STATUS "ccache: disabled")
    endif()

    # Build type specific
    if(CMAKE_BUILD_TYPE MATCHES "^.*Rel.*$") # same as in install/Config.cmake
        message(
            STATUS "Release build: CMAKE_BUILD_TYPE == '${CMAKE_BUILD_TYPE}'"
        )

        add_compile_definitions(NDEBUG)

        # enable additional glibc checks (used in debian packaging, requires -O)
        add_compile_definitions("_FORTIFY_SOURCE=2")
    else()
        message(STATUS "Debug build: CMAKE_BUILD_TYPE == '${CMAKE_BUILD_TYPE}'")
        add_compile_definitions(_GLIBCXX_ASSERTIONS)
        add_compile_definitions(BOOST_ENABLE_ASSERT_HANDLER)
    endif()
endfunction()

macro(eventprocessor_setup_environment)
    _eventprocessor_setup_environment_impl()
endmacro()
