cmake_minimum_required(VERSION 3.20)
project(keel VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Which backend to build. Only `sdl` exists in this pass. A device backend
# (DRM/fbdev + evdev) will be a second directory under platform/.
set(KEEL_PLATFORM sdl CACHE STRING "Keel platform backend")
set(KEEL_PLATFORM_DIR ${CMAKE_SOURCE_DIR}/platform/${KEEL_PLATFORM})

# ---- LVGL (MIT) -------------------------------------------------------------
include(FetchContent)

set(LV_BUILD_CONF_PATH ${KEEL_PLATFORM_DIR}/lv_conf.h CACHE PATH "" FORCE)
# Resolve FreeType from the toolbox. Never download or build a dependency
# behind our back. LVGL's SDL driver is off: the platform links SDL2 itself.
set(LV_FETCH_DEPENDENCIES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_SDL OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_FREETYPE ON CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LV_BUILD_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(lvgl
    GIT_REPOSITORY https://github.com/lvgl/lvgl.git
    GIT_TAG        v9.6.0
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(lvgl)

# ---- Keel UI (no platform code) ---------------------------------------------
file(GLOB KEEL_UI_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/ui/*.c)
add_library(keel_ui STATIC ${KEEL_UI_SOURCES})
target_include_directories(keel_ui PUBLIC ${CMAKE_SOURCE_DIR}/ui)
target_link_libraries(keel_ui PUBLIC lvgl::lvgl)
target_compile_options(keel_ui PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_compile_definitions(keel_ui PRIVATE KEEL_VERSION="${PROJECT_VERSION}")

# ---- Platform backend -------------------------------------------------------
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

file(GLOB KEEL_PLATFORM_SOURCES CONFIGURE_DEPENDS ${KEEL_PLATFORM_DIR}/*.c)
add_executable(keel ${KEEL_PLATFORM_SOURCES})
target_link_libraries(keel PRIVATE keel_ui PkgConfig::SDL2 m)
target_compile_options(keel PRIVATE -Wall -Wextra -Wno-unused-parameter)
# Default asset directory. Override at run time with KEEL_ASSETS=/path.
target_compile_definitions(keel PRIVATE KEEL_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets")
