cmake_minimum_required(VERSION 3.16)
project(stonegui C)

set(CMAKE_C_STANDARD 11)

# ── SDL2 ────────────────────────────────────────────────────────────────────
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

# ── QuickJS ─────────────────────────────────────────────────────────────────
set(QJS_DIR ${CMAKE_SOURCE_DIR}/third_party/quickjs)
set(QJS_SOURCES
    ${QJS_DIR}/quickjs.c
    ${QJS_DIR}/libregexp.c
    ${QJS_DIR}/libunicode.c
    ${QJS_DIR}/cutils.c
    ${QJS_DIR}/dtoa.c
    ${QJS_DIR}/quickjs-libc.c
)
add_library(quickjs STATIC ${QJS_SOURCES})
target_include_directories(quickjs PUBLIC ${QJS_DIR})
target_compile_definitions(quickjs PRIVATE
    CONFIG_VERSION="2024-01-13"
    _GNU_SOURCE
)
target_compile_options(quickjs PRIVATE -Wno-unused-result -Wno-sign-compare)

# ── LVGL ────────────────────────────────────────────────────────────────────
set(LV_CONF_PATH ${CMAKE_SOURCE_DIR}/lv_conf.h)
set(LV_CONF_BUILD_DISABLE_EXAMPLES 1)
set(LV_CONF_BUILD_DISABLE_DEMOS 1)
add_subdirectory(third_party/lvgl)
target_include_directories(lvgl PUBLIC ${CMAKE_SOURCE_DIR})

# ── stonegui host ────────────────────────────────────────────────────────────
add_executable(stonegui
    src/main.c
    src/lv_bindings.c
)

target_include_directories(stonegui PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party/lvgl
    ${QJS_DIR}
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(stonegui PRIVATE
    lvgl
    quickjs
    ${SDL2_LIBRARIES}
    m dl pthread
)

target_compile_definitions(stonegui PRIVATE
    _GNU_SOURCE
)

# Copy JS files to build dir so the binary can find them
add_custom_command(TARGET stonegui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/js ${CMAKE_BINARY_DIR}/js
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/examples ${CMAKE_BINARY_DIR}/examples
    COMMENT "Copying JS files to build directory"
)
