# The AOM-6881 is built on top of the generic RK3588 EVB U-Boot support
# (UBOOT_MACHINE = evb-rk3588_defconfig). U-Boot takes the board devicetree
# from the upstream devicetree-rebasing subtree (OF_UPSTREAM), so the EVB
# devicetree source is replaced with the AOM-6881 one. The existing
# arch/arm/dts/rk3588-evb1-v10-u-boot.dtsi (bootph properties for eMMC,
# SD card and debug UART via rk3588s-u-boot.dtsi) still applies, as the
# node labels are identical.

SRC_URI += " \
    file://rk3588-aom6881.dts;subdir=dts/upstream/src/arm64/rockchip \
    file://baudrate-115200.cfg \
"

do_configure:prepend() {
    cp "${UNPACKDIR}/dts/upstream/src/arm64/rockchip/rk3588-aom6881.dts" \
        "${S}/dts/upstream/src/arm64/rockchip/rk3588-evb1-v10.dts"
}
