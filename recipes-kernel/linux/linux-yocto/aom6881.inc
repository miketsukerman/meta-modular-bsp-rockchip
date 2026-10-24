require common/realtek-phy.inc

SRC_URI += " \
    file://rk3588-aom6881.dts;subdir=arch/arm64/boot/dts/rockchip \
    file://aom6881-eth-usb.cfg \
"

do_configure:prepend() {
    cp "${UNPACKDIR}/arch/arm64/boot/dts/rockchip/rk3588-aom6881.dts" \
        "${S}/arch/arm64/boot/dts/rockchip/"

    echo 'dtb-$(CONFIG_ARCH_ROCKCHIP) += rk3588-aom6881.dtb' >> \
        "${S}/arch/arm64/boot/dts/rockchip/Makefile"
}
