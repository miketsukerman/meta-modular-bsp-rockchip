require common/realtek-phy.inc

SRC_URI += " \
    file://rk3568-rsb4810.dts;subdir=arch/arm64/boot/dts/rockchip \
    file://rsb4810-eth-usb.cfg \
    file://rsb4810-lvds.cfg \
"

# RK3568 LVDS transmitter support (not in mainline): out-of-tree series
# "[PATCH v2 0/4] drm/rockchip: add RK3568 LVDS support" by Rok Markovic,
# Message-ID 20260724080824.115260-1-rok@kanardia.eu (tested upstream on
# 7.1.3, carried here on linux-yocto 6.18).
RSB4810_LVDS_PATCHES = " \
    0001-drm-rockchip-lvds-propagate-bus_flags-to-the-CRTC-state.patch \
    0002-dt-bindings-display-rockchip-lvds-add-RK3568.patch \
    0003-drm-rockchip-lvds-add-RK3568-support.patch \
    0004-arm64-dts-rockchip-rk356x-add-LVDS-node.patch \
"
SRC_URI += "${@' '.join('file://' + p for p in d.getVar('RSB4810_LVDS_PATCHES').split())}"

# Fail early and readably if the series has not been fetched yet (the
# files in linux-yocto/rsb4810/ are placeholders until replaced).
python rsb4810_check_lvds_patches() {
    import os
    missing = []
    for p in (d.getVar('RSB4810_LVDS_PATCHES') or '').split():
        with open(os.path.join(d.getVar('UNPACKDIR'), p), errors='replace') as f:
            if 'RSB4810-LVDS-PLACEHOLDER' in f.read(256):
                missing.append(p)
    if missing:
        bb.fatal("RK3568 LVDS patches are still placeholders: %s\n"
                 "Fetch them with 'b4 am 20260724080824.115260-1-rok@kanardia.eu' "
                 "and replace the files in recipes-kernel/linux/linux-yocto/rsb4810/ "
                 "(see the placeholder files for details)." % ' '.join(missing))
}
do_patch[prefuncs] += "rsb4810_check_lvds_patches"

do_configure:prepend() {
    cp "${UNPACKDIR}/arch/arm64/boot/dts/rockchip/rk3568-rsb4810.dts" \
        "${S}/arch/arm64/boot/dts/rockchip/"

    echo 'dtb-$(CONFIG_ARCH_ROCKCHIP) += rk3568-rsb4810.dtb' >> \
        "${S}/arch/arm64/boot/dts/rockchip/Makefile"
}
