FILESEXTRAPATHS:prepend := "${THISDIR}/linux-yocto:${THISDIR}/linux-yocto/common:"

COMPATIBLE_MACHINE:rsb4810 = "rsb4810"
COMPATIBLE_MACHINE:aom6881 = "aom6881"

require ${@ 'recipes-kernel/linux/linux-yocto/${MACHINE}.inc' if (d.getVar('MACHINE') or '').startswith(('rsb4810', 'aom6881')) else '' }
