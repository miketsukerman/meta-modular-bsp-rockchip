SUMMARY = "LVGL widgets demo for Wayland (Weston) and DRM/KMS"
DESCRIPTION = "Runs the LVGL widgets demo with the system LVGL library, as a \
Wayland client (full screen under Weston) when a compositor is running, or \
directly on a DRM/KMS output with evdev input otherwise. The backend, DRM \
card/connector and input device are selected with environment variables \
(see main.c). The optional ${PN}-service package adds a systemd unit that \
is not enabled by default."
HOMEPAGE = "https://lvgl.io/"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=6f0d9f0a075681144195dcfdc82e7914"

SRC_URI = " \
    file://main.c \
    file://CMakeLists.txt \
    file://LICENSE \
    file://lvgl-demo.service \
"

S = "${UNPACKDIR}"

# libdrm: needed by the LVGL headers (lvgl.pc "Requires: libdrm")
DEPENDS = "lvgl libdrm"

inherit cmake pkgconfig systemd

# Needs an LVGL built with the DRM/Wayland, evdev and widgets demo options,
# which lvgl_%.bbappend only enables for RK3568, and is built against that
# machine-specific lv_conf.h.
COMPATIBLE_MACHINE = "(rk3568)"
PACKAGE_ARCH = "${MACHINE_ARCH}"

do_install:append() {
    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
        install -d ${D}${systemd_system_unitdir}
        install -m 0644 ${UNPACKDIR}/lvgl-demo.service ${D}${systemd_system_unitdir}/
    fi
}

PACKAGES =+ "${PN}-service"

SYSTEMD_PACKAGES = "${PN}-service"
SYSTEMD_SERVICE:${PN}-service = "lvgl-demo.service"
SYSTEMD_AUTO_ENABLE:${PN}-service = "disable"

FILES:${PN}-service = "${systemd_system_unitdir}/lvgl-demo.service"
RDEPENDS:${PN}-service = "${PN}"
