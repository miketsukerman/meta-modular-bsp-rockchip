SUMMARY = "Display bring-up and test tools for Advantech boards"
DESCRIPTION = "KMS/DRM test programs used to bring up and validate display \
outputs (HDMI, LVDS): modetest & co. from libdrm-tests, gpioset/gpioget \
for checking panel and backlight enable lines and, on distros \
with OpenGL, kmscube."

inherit packagegroup

RDEPENDS:${PN} = " \
    libdrm-tests \
    libgpiod-tools \
    ${@bb.utils.contains('DISTRO_FEATURES', 'opengl', 'kmscube', '', d)} \
"
