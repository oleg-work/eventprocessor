function(_eventprocessor_get_sanitize_options COMPILE_FLAGS_VAR LINK_FLAGS_VAR)
    # https://clang.llvm.org/docs/AddressSanitizer.html
    option(EVENTPROCESSOR_ENABLE_ASAN "Enable AddressSanitizer" NO)
    # https://clang.llvm.org/docs/LeakSanitizer.html
    option(EVENTPROCESSOR_ENABLE_LSAN "Enable LeakSanitizer" NO)
    # https://clang.llvm.org/docs/ThreadSanitizer.html
    option(EVENTPROCESSOR_ENABLE_TSAN "Enable ThreadSanitizer" NO)
    # https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
    option(EVENTPROCESSOR_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" NO)
    # https://clang.llvm.org/docs/MemorySanitizer.html
    option(EVENTPROCESSOR_ENABLE_MSAN "Enable MemorySanitizer" NO)

    if(${EVENTPROCESSOR_ENABLE_MSAN} AND "${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
        message(FATAL_ERROR "MSAN is not supported on macOS")
    endif()

    if((EVENTPROCESSOR_ENABLE_TSAN AND EVENTPROCESSOR_ENABLE_ASAN)
       OR (EVENTPROCESSOR_ENABLE_TSAN AND EVENTPROCESSOR_ENABLE_LSAN)
       OR (EVENTPROCESSOR_ENABLE_TSAN AND EVENTPROCESSOR_ENABLE_MSAN)
       OR (EVENTPROCESSOR_ENABLE_ASAN AND EVENTPROCESSOR_ENABLE_MSAN)
       OR (EVENTPROCESSOR_ENABLE_LSAN AND EVENTPROCESSOR_ENABLE_MSAN)
    )
        message(
            FATAL_ERROR
                "Invalid sanitizer combination:\n"
                " EVENTPROCESSOR_ENABLE_ASAN: ${EVENTPROCESSOR_ENABLE_ASAN}\n"
                " EVENTPROCESSOR_ENABLE_LSAN: ${EVENTPROCESSOR_ENABLE_LSAN}\n"
                " EVENTPROCESSOR_ENABLE_TSAN: ${EVENTPROCESSOR_ENABLE_TSAN}\n"
                " EVENTPROCESSOR_ENABLE_UBSAN: ${EVENTPROCESSOR_ENABLE_UBSAN}\n"
                " EVENTPROCESSOR_ENABLE_MSAN: ${EVENTPROCESSOR_ENABLE_MSAN}"
        )
    endif()

    set(sanitizer_list)
    if(EVENTPROCESSOR_ENABLE_ASAN)
        list(APPEND sanitizer_list "ASAN")
    endif()
    if(EVENTPROCESSOR_ENABLE_LSAN)
        list(APPEND sanitizer_list "LSAN")
    endif()
    if(EVENTPROCESSOR_ENABLE_MSAN)
        list(APPEND sanitizer_list "MSAN")
    endif()
    if(EVENTPROCESSOR_ENABLE_TSAN)
        list(APPEND sanitizer_list "TSAN")
    endif()
    if(EVENTPROCESSOR_ENABLE_UBSAN)
        list(APPEND sanitizer_list "UBSAN")
    endif()

    if(NOT sanitizer_list)
        return()
    endif()

    set(sanitize_cxx_and_link_flags "-g")
    set(sanitize_cxx_flags)
    set(sanitize_link_flags)

    set(asan_flags "-fsanitize=address")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # gcc links with ASAN dynamically by default, and that leads to all
        # sorts of problems when we intercept dl_iterate_phdr, which ASAN uses
        # in initialization before main.
        set(asan_flags "-fsanitize=address -static-libasan")
    endif()
    set(lsan_flags -fsanitize=leak)
    set(msan_flags -fsanitize=memory)
    set(tsan_flags -fsanitize=thread)
    set(ubsan_flags -fsanitize=undefined -fno-sanitize-recover=undefined)

    list(
        APPEND
        sanitize_cxx_and_link_flags
        $<$<BOOL:${EVENTPROCESSOR_ENABLE_ASAN}>:${asan_flags}>
        $<$<BOOL:${EVENTPROCESSOR_ENABLE_LSAN}>:${lsan_flags}>
        $<$<BOOL:${EVENTPROCESSOR_ENABLE_MSAN}>:${msan_flags}>
        $<$<BOOL:${EVENTPROCESSOR_ENABLE_TSAN}>:${tsan_flags}>
        $<$<BOOL:${EVENTPROCESSOR_ENABLE_UBSAN}>:${ubsan_flags}>
    )

    list(APPEND sanitize_cxx_flags
         $<$<BOOL:${EVENTPROCESSOR_ENABLE_ASAN}>:-fno-omit-frame-pointer>
         $<$<BOOL:${EVENTPROCESSOR_ENABLE_MSAN}>:-fno-omit-frame-pointer>
         $<$<BOOL:${EVENTPROCESSOR_ENABLE_TSAN}>:-fno-omit-frame-pointer>
    )

    message(STATUS "Sanitizers are ON: ${sanitizer_list}")

    set("${COMPILE_FLAGS_VAR}"
        ${sanitize_cxx_flags} ${sanitize_cxx_and_link_flags}
        PARENT_SCOPE
    )
    set("${LINK_FLAGS_VAR}"
        ${sanitize_link_flags} ${sanitize_cxx_and_link_flags}
        PARENT_SCOPE
    )
endfunction()

if(TARGET eventprocessor-internal-sanitize-options)
    return()
endif()

set(sanitize_cxx_flags)
set(sanitize_link_flags)
_eventprocessor_get_sanitize_options(sanitize_cxx_flags sanitize_link_flags)

add_library(eventprocessor-internal-sanitize-options INTERFACE)
target_compile_options(
    eventprocessor-internal-sanitize-options INTERFACE ${sanitize_cxx_flags}
)
target_link_options(
    eventprocessor-internal-sanitize-options INTERFACE ${sanitize_link_flags}
)
