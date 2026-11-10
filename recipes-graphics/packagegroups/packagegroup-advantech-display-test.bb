SUMMARY = "Display bring-up and test tools for Advantech boards"
DESCRIPTION = "KMS/DRM test programs used to bring up and validate display \
outputs (HDMI, LVDS): modetest & co. from libdrm-tests, on distros with \
OpenGL kmscube and, with meta-oe, gpioset/gpioget for checking panel and \
backlight enable lines (see the bbappend in \
dynamic-layers/openembedded-layer)."

inherit packagegroup

RDEPENDS:${PN} = " \
    libdrm-tests \
    ${@bb.utils.contains('DISTRO_FEATURES', 'opengl', 'kmscube', '', d)} \
"
