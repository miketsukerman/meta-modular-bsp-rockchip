FILESEXTRAPATHS:prepend := "${THISDIR}/linux-yocto:${THISDIR}/linux-yocto/common:"

COMPATIBLE_MACHINE:rsb4810 = "rsb4810"

require ${@ 'recipes-kernel/linux/linux-yocto/${MACHINE}.inc' if (d.getVar('MACHINE') or '').startswith('rsb4810') else '' }
