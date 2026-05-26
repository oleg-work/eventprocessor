if(TARGET eventprocessor-internal-compile-options)
    return()
endif()

add_library(eventprocessor-internal-compile-options INTERFACE)
target_compile_features(eventprocessor-internal-compile-options INTERFACE cxx_std_23)
target_compile_options(
    eventprocessor-internal-compile-options
    INTERFACE "-Wall"
              "-Wextra"
              "-Wpedantic"
              "-ftemplate-backtrace-limit=0"
              "-Wdisabled-optimization"
              "-Winvalid-pch"
              "-Wimplicit-fallthrough"
              "-Wformat=2"
              "-Wno-error=deprecated-declarations"
              # This warning is unavoidable in generic code with templates
              # Starting from C++20, passing zero arguments to a variadic macro
              # parameter is allowed
              "-Wno-gnu-zero-variadic-macro-arguments"
              "-Wno-range-loop-analysis"
)

option(EVENTPROCESSOR_ENABLE_COMPILER_COLORS "Highlight complier output" OFF)
if(CMAKE_CXX_COMPILER_ID MATCHES "(Apple)?[Cc]lang")
    target_compile_options(
        eventprocessor-internal-compile-options
        INTERFACE
            $<$<BOOL:${EVENTPROCESSOR_ENABLE_COMPILER_COLORS}>:-fcolor-diagnostics>
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(
        eventprocessor-internal-compile-options
        INTERFACE
            $<$<BOOL:${EVENTPROCESSOR_ENABLE_COMPILER_COLORS}>:-fdiagnostics-color=always>
    )
endif()

option(EVENTPROCESSOR_ENABLE_WERROR "Treat warnings as errors" OFF)
if(EVENTPROCESSOR_ENABLE_WERROR)
    message(STATUS "Forcing warnings as errors!")
    target_compile_options(
        eventprocessor-internal-compile-options INTERFACE "-Werror"
    )
endif()
