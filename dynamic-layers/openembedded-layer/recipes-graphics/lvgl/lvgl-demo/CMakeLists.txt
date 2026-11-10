# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.13)
project(lvgl-demo-drm LANGUAGES C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LVGL REQUIRED IMPORTED_TARGET lvgl)

add_executable(lvgl-demo-drm main.c)
# lvgl.pc only lists liblvgl; the widgets demo lives in liblvgl_demos
target_link_libraries(lvgl-demo-drm PRIVATE PkgConfig::LVGL lvgl_demos)

include(GNUInstallDirs)
install(TARGETS lvgl-demo-drm RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
