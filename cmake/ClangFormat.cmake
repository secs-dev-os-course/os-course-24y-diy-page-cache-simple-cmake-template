if (OS_LAB2_DEVELOPER)
    find_program(CLANG_FORMAT NAMES clang-format)
    if (CLANG_FORMAT)
        add_custom_target(
                clang-format-check
                COMMAND ${CLANG_FORMAT} --dry-run --Werror ${PROJECT_SOURCE_DIR}/source/**/*.c ${PROJECT_SOURCE_DIR}/source/**/*.h
                COMMENT "Running clang-format check..."
        )
        add_custom_target(
                clang-format
                COMMAND ${CLANG_FORMAT} -i ${PROJECT_SOURCE_DIR}/source/**/*.c ${PROJECT_SOURCE_DIR}/source/**/*.h
                COMMENT "Running clang-format..."
        )
    endif ()
endif ()