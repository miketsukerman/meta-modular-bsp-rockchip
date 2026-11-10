SUMMARY = "LVGL runtime and DRM demo for Advantech boards"
DESCRIPTION = "LVGL library and the lvgl-demo-drm widgets demo. The GPU \
runtime (libEGL/libGLESv2) is pulled in by lvgl itself when it is built \
with PACKAGECONFIG \"egl\". Add it to an image with \
EXTRA_IMAGE_FEATURES += \"lvgl\"."

# Machine-specific: lvgl is configured per SoC (lvgl_%.bbappend)
PACKAGE_ARCH = "${MACHINE_ARCH}"

inherit packagegroup

COMPATIBLE_MACHINE = "(rk3568)"

RDEPENDS:${PN} = " \
    lvgl \
    lvgl-demo-drm \
"
