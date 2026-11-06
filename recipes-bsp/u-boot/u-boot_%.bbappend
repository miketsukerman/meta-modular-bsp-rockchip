FILESEXTRAPATHS:prepend := "${THISDIR}/u-boot:"

SRC_URI += " \
    file://common/saveenv.cfg \
"

require ${@ 'recipes-bsp/u-boot/u-boot/${MACHINE}.inc' if (d.getVar('MACHINE') or '').startswith(('rsb4810', 'aom6881')) else '' }
