SUMMARY = "Rockchip miniloader (MiniLoaderAll.bin)"
DESCRIPTION = "Builds the Rockchip miniloader from the prebuilt rkbin blobs \
with boot_merger. The miniloader (DDR init + usbplug + SPL) is the \
'bootloader' entry of a Rockchip update.img and is what the BootROM runs in \
SD-upgrade or USB (maskrom/loader) mode to flash the eMMC."

# Same source pin as the rockchip-rkbin-* recipes in upstream meta-rockchip
SRC_URI = "git://github.com/rockchip-linux/rkbin;protocol=https;branch=master"
SRCREV = "f43a462e7a1429a9d407ae52b4745033034a6cf9"
LICENSE = "Proprietary"
LIC_FILES_CHKSUM = "file://LICENSE;md5=11e3673115959bf596feaaa6ea7ce9a5"

S = "${UNPACKDIR}/${BP}"

inherit deploy nopackages

INHIBIT_DEFAULT_DEPS = "1"

COMPATIBLE_MACHINE = "^$"
COMPATIBLE_MACHINE:rk3568 = "rk3568"

PACKAGE_ARCH = "${MACHINE_ARCH}"

# boot_merger ini describing the DDR blob and SPL that make up the miniloader
RKBIN_LOADER_INI ?= ""
RKBIN_LOADER_INI:rk3568 = "RKBOOT/RK3568MINIALL.ini"

# NOTE: rkbin's boot_merger is a prebuilt static x86-64 binary; this recipe
# can only be built on an x86-64 build host.
do_compile() {
	if [ -z "${RKBIN_LOADER_INI}" ]; then
		bbfatal "Non-empty RKBIN_LOADER_INI:<MACHINE> required!"
	fi
	cd "${S}"
	./tools/boot_merger "${RKBIN_LOADER_INI}"
}

do_install() {
	# Nothing in this recipe is useful in a filesystem
	:
}

do_deploy() {
	LOADER="$(sed -n 's/^PATH=//p' "${S}/${RKBIN_LOADER_INI}" | tr -d '\r')"
	if [ -z "${LOADER}" ] || [ ! -f "${S}/${LOADER}" ]; then
		bbfatal "boot_merger output '${LOADER}' not found"
	fi
	install -D -m 644 "${S}/${LOADER}" "${DEPLOYDIR}/MiniLoaderAll.bin"
}

addtask deploy after do_compile
