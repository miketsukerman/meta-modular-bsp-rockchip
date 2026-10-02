# The RSB-4810 is built on top of the generic RK3568 EVB U-Boot support
# (UBOOT_MACHINE = evb-rk3568_defconfig). U-Boot takes the board devicetree
# from the upstream devicetree-rebasing subtree (OF_UPSTREAM), so the EVB
# devicetree source is replaced with the RSB-4810 one. The existing
# arch/arm/dts/rk3568-evb1-v10-u-boot.dtsi (bootph properties for eMMC,
# SD card and debug UART) still applies, as the node labels are identical.

SRC_URI += " \
    file://rk3568-rsb4810.dts;subdir=dts/upstream/src/arm64/rockchip \
"

do_configure:prepend() {
    cp "${UNPACKDIR}/dts/upstream/src/arm64/rockchip/rk3568-rsb4810.dts" \
        "${S}/dts/upstream/src/arm64/rockchip/rk3568-evb1-v10.dts"
}
