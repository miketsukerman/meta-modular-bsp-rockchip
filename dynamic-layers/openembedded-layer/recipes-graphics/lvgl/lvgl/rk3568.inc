# LVGL on RK3568: Linux DRM/KMS display (VOP2: HDMI, LVDS) with evdev input.
#
# Configuration is done the meta-oe way, with Kconfig fragments on top of
# lv-conf.inc's defconfig and drm.cfg (LV_USE_LINUX_DRM, LV_USE_EVDEV):
#   lvgl-rk3568.cfg     always: 32 bpp, pthread OS layer, 4 software draw
#                       threads, logging, widgets demo (liblvgl_demos)
#   lvgl-drm-egl.cfg    PACKAGECONFIG "egl": EGL/OpenGL ES on a GBM surface
#                       instead of software rendering into DRM dumb buffers
#   lvgl-libinput.cfg   PACKAGECONFIG "libinput": libinput input driver
#
# "egl" is the default when the distro has the "opengl" feature: Mesa then
# provides EGL, GLES2 and GBM, with the panfrost driver for the Mali-G52
# (meta-rockchip's mesa.bbappend). Fall back to software rendering with
#   PACKAGECONFIG:remove:pn-lvgl = "egl"

# The configuration is SoC-specific, so are the packages
PACKAGE_ARCH = "${MACHINE_ARCH}"

PACKAGECONFIG:append = "${@bb.utils.contains('DISTRO_FEATURES', 'opengl', ' egl', '', d)}"

FILESEXTRAPATHS:prepend := "${THISDIR}/rk3568:"

SRC_URI += " \
    file://0001-Kconfig-add-a-Linux-DRM-rendering-backend-choice.patch \
    file://lvgl-rk3568.cfg \
    ${@bb.utils.contains('PACKAGECONFIG', 'egl', 'file://lvgl-drm-egl.cfg', '', d)} \
    ${@bb.utils.contains('PACKAGECONFIG', 'libinput', 'file://lvgl-libinput.cfg', '', d)} \
"

# EGL/GBM: libgbm is linked, libEGL and libGLESv2 are dlopen()ed by LVGL's
# bundled GLAD loader at runtime, hence the explicit RDEPENDS. The EGL/KHR
# headers come from virtual/egl. The GPU needs Mesa's DRI drivers and the
# panfrost kernel module (=m in the arm64 defconfig).
PACKAGECONFIG[egl] = ",,virtual/egl virtual/libgles2 virtual/libgbm,libegl libgles2,mesa-megadriver kernel-module-panfrost"
PACKAGECONFIG[libinput] = ",,libinput"

# LVGL's CMake build links no system libraries into liblvgl.so, leaving the
# libdrm/libgbm/libinput symbols for every application to resolve. Link
# them into the library so it has correct DT_NEEDED entries (and runtime
# dependencies via shlibdeps); CMAKE_*_STANDARD_LIBRARIES go after the
# objects, so --as-needed keeps them.
LVGL_LINK_LIBS = "-lm"
LVGL_LINK_LIBS:append = "${@bb.utils.contains('PACKAGECONFIG', 'drm', ' -ldrm', '', d)}"
LVGL_LINK_LIBS:append = "${@bb.utils.contains('PACKAGECONFIG', 'egl', ' -lgbm', '', d)}"
LVGL_LINK_LIBS:append = "${@bb.utils.contains('PACKAGECONFIG', 'libinput', ' -linput', '', d)}"
EXTRA_OECMAKE += " \
    -DCMAKE_C_STANDARD_LIBRARIES='${LVGL_LINK_LIBS}' \
    -DCMAKE_CXX_STANDARD_LIBRARIES='${LVGL_LINK_LIBS}' \
"

# In Kconfig mode LVGL's CMake only sees the options that are enabled, so
# its own option() defaults build the examples and the internal ThorVG
# library even when Kconfig disables them. Default them off; a Kconfig
# fragment enabling them still wins.
EXTRA_OECMAKE += "-DCONFIG_LV_BUILD_EXAMPLES=OFF -DCONFIG_LV_USE_THORVG_INTERNAL=OFF"

# The public headers include libdrm's (lv_linux_drm.h -> <xf86drmMode.h> ->
# <drm.h> in ${includedir}/libdrm), but LVGL's lvgl.pc only lists liblvgl,
# so pkg-config users fail to compile. Declare the requirement; this also
# makes lvgl-dev RDEPEND on libdrm-dev.
do_install:append() {
    if ${@bb.utils.contains('PACKAGECONFIG', 'drm', 'true', 'false', d)}; then
        sed -i '/^Libs:/i Requires: libdrm' ${D}${datadir}/pkgconfig/lvgl.pc
    fi
}
