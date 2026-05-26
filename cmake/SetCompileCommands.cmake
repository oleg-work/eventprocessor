include_guard(GLOBAL)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(compile_commands_file "${CMAKE_BINARY_DIR}/compile_commands.json")
set(compile_commands_link "${CMAKE_SOURCE_DIR}/compile_commands.json")
add_custom_target(
    symlink-export-compile-commands ALL
    COMMAND ${CMAKE_COMMAND} -E remove -f "${compile_commands_link}"
    # if file doesn't exist, create empty to avoid error
    COMMAND ${CMAKE_COMMAND} -E touch "${compile_commands_file}"
    COMMAND ${CMAKE_COMMAND} -E create_symlink "${compile_commands_file}"
            "${compile_commands_link}"
    COMMENT "Creating symbolic link for compile_commands.json"
)
