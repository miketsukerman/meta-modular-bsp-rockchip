# LVGL on RK3568: Linux DRM/KMS display (VOP2: HDMI, LVDS) with evdev input.
#
# Configuration is done the meta-oe way, with Kconfig fragments on top of
# lv-conf.inc's defconfig and drm.cfg (LV_USE_LINUX_DRM, LV_USE_EVDEV):
#   lvgl-rk3568.cfg     always: 32 bpp, pthread OS layer, 4 software draw
#                       threads, logging, widgets demo (liblvgl_demos)
#   lvgl-drm-egl.cfg    PACKAGECONFIG "egl": EGL/OpenGL ES on a GBM surface
#                       instead of software rendering into DRM dumb buffers
#   lvgl-libinput.cfg   PACKAGECONFIG "libinput": libinput input driver
#   lvgl-gles-draw.cfg  PACKAGECONFIG "gles-draw" (opt-in, needs "egl", not
#                       with "wayland"): OpenGL ES draw unit
#   lvgl-wayland.cfg    PACKAGECONFIG "wayland": Wayland client backend
#                       (windows under Weston), next to DRM/KMS
#
# "egl" is the default when the distro has the "opengl" feature: Mesa then
# provides EGL, GLES2 and GBM, with the panfrost driver for the Mali-G52
# (meta-rockchip's mesa.bbappend). Fall back to software rendering with
#   PACKAGECONFIG:remove:pn-lvgl = "egl"
# "wayland" is the default when the distro has the "wayland" feature
# (core-image-weston); applications pick DRM/KMS or Wayland at runtime.

# The configuration is SoC-specific, so are the packages
PACKAGE_ARCH = "${MACHINE_ARCH}"

PACKAGECONFIG:append = "${@bb.utils.contains('DISTRO_FEATURES', 'opengl', ' egl', '', d)}"
PACKAGECONFIG:append = "${@bb.utils.contains('DISTRO_FEATURES', 'wayland', ' wayland', '', d)}"

FILESEXTRAPATHS:prepend := "${THISDIR}/rk3568:"

SRC_URI += " \
    file://0001-Kconfig-add-a-Linux-DRM-rendering-backend-choice.patch \
    file://0002-wayland-shm-reallocate-the-draw-buffer-when-it-has-t.patch \
    file://lvgl-rk3568.cfg \
    file://lv_assert_abort.h \
    ${@bb.utils.contains('PACKAGECONFIG', 'egl', 'file://lvgl-drm-egl.cfg', '', d)} \
    ${@bb.utils.contains('PACKAGECONFIG', 'gles-draw', 'file://lvgl-gles-draw.cfg', '', d)} \
    ${@bb.utils.contains('PACKAGECONFIG', 'libinput', 'file://lvgl-libinput.cfg', '', d)} \
    ${@bb.utils.contains('PACKAGECONFIG', 'wayland', 'file://lvgl-wayland.cfg', '', d)} \
"

# EGL/GBM: libgbm is linked, libEGL and libGLESv2 are dlopen()ed by LVGL's
# bundled GLAD loader at runtime, hence the explicit RDEPENDS. The EGL/KHR
# headers come from virtual/egl. The GPU needs Mesa's DRI drivers and the
# panfrost kernel module (=m in the arm64 defconfig).
PACKAGECONFIG[egl] = ",,virtual/egl virtual/libgles2 virtual/libgbm,libegl libgles2,mesa-megadriver kernel-module-panfrost"
PACKAGECONFIG[gles-draw] = ",,"
python () {
    pkgconfig = (d.getVar('PACKAGECONFIG') or '').split()
    if 'gles-draw' in pkgconfig:
        if 'egl' not in pkgconfig:
            raise bb.parse.SkipRecipe("PACKAGECONFIG 'gles-draw' needs 'egl'")
        if 'wayland' in pkgconfig:
            raise bb.parse.SkipRecipe("PACKAGECONFIG 'gles-draw' cannot be combined with 'wayland' "
                                      "(the OpenGL ES draw unit needs a GL context, LVGL's Wayland "
                                      "backend renders into wl_shm buffers)")
}
PACKAGECONFIG[libinput] = ",,libinput"
PACKAGECONFIG[wayland] = ",,wayland wayland-native wayland-protocols libxkbcommon"

# LVGL v9.4's Wayland driver includes <wayland_xdg_shell.h> but its build
# does not generate it. Generate the xdg-shell client header and glue code
# next to the driver before CMake globs the sources.
LVGL_WL_DIR = "${S}/src/drivers/wayland"
CFLAGS += "${@bb.utils.contains('PACKAGECONFIG', 'wayland', '-I${LVGL_WL_DIR}', '', d)}"
do_configure:prepend() {
    # assert handler header (lvgl-rk3568.cfg), next to src/misc/lv_assert.h
    # which includes it; it is installed with the other headers
    install -m 0644 ${UNPACKDIR}/lv_assert_abort.h ${S}/src/misc/
    if ${@bb.utils.contains('PACKAGECONFIG', 'wayland', 'true', 'false', d)}; then
        xml=${STAGING_DATADIR}/wayland-protocols/stable/xdg-shell/xdg-shell.xml
        wayland-scanner client-header $xml ${LVGL_WL_DIR}/wayland_xdg_shell.h
        wayland-scanner private-code $xml ${LVGL_WL_DIR}/wayland_xdg_shell.c
    fi
}

# LVGL's CMake build links no system libraries into liblvgl.so, leaving the
# libdrm/libgbm/libinput/wayland symbols for every application to resolve. Link
# them into the library so it has correct DT_NEEDED entries (and runtime
# dependencies via shlibdeps); CMAKE_*_STANDARD_LIBRARIES go after the
# objects, so --as-needed keeps them.
LVGL_LINK_LIBS = "-lm"
LVGL_LINK_LIBS:append = "${@bb.utils.contains('PACKAGECONFIG', 'drm', ' -ldrm', '', d)}"
LVGL_LINK_LIBS:append = "${@bb.utils.contains('PACKAGECONFIG', 'egl', ' -lgbm', '', d)}"
LVGL_LINK_LIBS:append = "${@bb.utils.contains('PACKAGECONFIG', 'libinput', ' -linput', '', d)}"
LVGL_LINK_LIBS:append = "${@bb.utils.contains('PACKAGECONFIG', 'wayland', ' -lwayland-client -lwayland-cursor -lxkbcommon', '', d)}"
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
    # generated above for the build only, no public header includes it
    rm -f ${D}${includedir}/lvgl/src/drivers/wayland/wayland_xdg_shell.h
}
