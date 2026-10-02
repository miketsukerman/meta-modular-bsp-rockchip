SUMMARY = "Rockchip firmware packaging tools (afptool, rkImageMaker)"
DESCRIPTION = "Prebuilt Linux x86-64 binaries of the Rockchip Linux_Pack_Firmware \
tools used to pack a flashable update.img: afptool packs the partition images \
according to a package-file manifest, rkImageMaker combines the result with \
the Rockchip miniloader into the final update.img."
HOMEPAGE = "https://github.com/TinkerBoard-Android/rockchip-android-RKTools"
# Prebuilt proprietary Rockchip tools, no license file is shipped upstream
LICENSE = "CLOSED"

SRC_URI = "git://github.com/TinkerBoard-Android/rockchip-android-RKTools;protocol=https;branch=android12-rockchip"
SRCREV = "b6c0dbb389047f250fb40e11222c7aafec7e2415"

S = "${UNPACKDIR}/${BP}/linux/Linux_Pack_Firmware/rockdev"

inherit native

# Prebuilt static x86-64 binaries, nothing to build and no toolchain needed
INHIBIT_DEFAULT_DEPS = "1"

do_compile() {
	:
}

do_install() {
	install -d "${D}${bindir}"
	install -m 0755 "${S}/afptool" "${D}${bindir}"
	install -m 0755 "${S}/rkImageMaker" "${D}${bindir}"
}
