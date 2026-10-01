require common/realtek-phy.inc

SRC_URI += " \
    file://rk3568-rsb4810.dts;subdir=arch/arm64/boot/dts/rockchip \
"

do_configure:prepend() {
    cp "${UNPACKDIR}/arch/arm64/boot/dts/rockchip/rk3568-rsb4810.dts" \
        "${S}/arch/arm64/boot/dts/rockchip/"

    echo 'dtb-$(CONFIG_ARCH_ROCKCHIP) += rk3568-rsb4810.dtb' >> \
        "${S}/arch/arm64/boot/dts/rockchip/Makefile"
}
