if (OS_LAB2_DEVELOPER)
    find_program(CLANG_TIDY NAMES clang-tidy)
    if (CLANG_TIDY)
        add_custom_target(
                clang-tidy-check
                COMMAND ${CLANG_TIDY} ${PROJECT_SOURCE_DIR}/source/**/*.c ${PROJECT_SOURCE_DIR}/source/**/*.h -p ${PROJECT_SOURCE_DIR}/build/dev/compile_commands.json
                COMMENT "Running clang-tidy check..."
        )
    endif ()
endif ()