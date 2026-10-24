SUMMARY = "Rockchip miniloader (MiniLoaderAll.bin)"
DESCRIPTION = "Builds the Rockchip miniloader from the prebuilt rkbin blobs \
with boot_merger. The miniloader (DDR init + usbplug + SPL) is the \
'bootloader' entry of a Rockchip update.img and is what the BootROM runs in \
SD-upgrade or USB (maskrom/loader) mode to flash the eMMC."

# Newer than the rockchip-rkbin-* pin in upstream meta-rockchip: rkbin
# master ships rk3568_ddr_1560MHz_v1.26.bin (eMMC init fixes over v1.23)
SRC_URI = "git://github.com/rockchip-linux/rkbin;protocol=https;branch=master"
SRCREV = "3e288fe814e059dd06833495f845cab04ac20a5c"
LICENSE = "Proprietary"
LIC_FILES_CHKSUM = "file://LICENSE;md5=e1d83c54b4e0b5e550769dd04f092f6c"

S = "${UNPACKDIR}/${BP}"

inherit deploy nopackages

INHIBIT_DEFAULT_DEPS = "1"

COMPATIBLE_MACHINE = "^$"
COMPATIBLE_MACHINE:rk3568 = "rk3568"
COMPATIBLE_MACHINE:rk3588 = "rk3588"

PACKAGE_ARCH = "${MACHINE_ARCH}"

# boot_merger ini describing the DDR blob and SPL that make up the miniloader
RKBIN_LOADER_INI ?= ""
RKBIN_LOADER_INI:rk3568 = "RKBOOT/RK3568MINIALL.ini"
RKBIN_LOADER_INI:rk3588 = "RKBOOT/RK3588MINIALL.ini"

# Chip name understood by rkbin's tools/ddrbin_tool.py
RKBIN_DDRBIN_CHIP ?= ""
RKBIN_DDRBIN_CHIP:rk3568 = "rk3568"
RKBIN_DDRBIN_CHIP:rk3588 = "rk3588"

# Debug UART baud rate of the loader's DDR init blob. The Rockchip blobs
# default to 1500000; set e.g. "115200" for USB-UART adapters that cannot
# generate an accurate 1.5 MBd. Leave empty to keep the blob default.
# Note: this only changes the miniloader (DDR banner/training) output;
# U-Boot proper and the kernel keep their own console baud rate.
RKBIN_UART_BAUDRATE ?= ""

# NOTE: rkbin's boot_merger is a prebuilt static x86-64 binary; this recipe
# can only be built on an x86-64 build host.
do_compile() {
	if [ -z "${RKBIN_LOADER_INI}" ]; then
		bbfatal "Non-empty RKBIN_LOADER_INI:<MACHINE> required!"
	fi
	cd "${S}"
	if [ -n "${RKBIN_UART_BAUDRATE}" ]; then
		if [ -z "${RKBIN_DDRBIN_CHIP}" ]; then
			bbfatal "RKBIN_UART_BAUDRATE set but RKBIN_DDRBIN_CHIP is empty"
		fi
		sed -e 's/^uart baudrate=.*/uart baudrate=${RKBIN_UART_BAUDRATE}/' \
			tools/ddrbin_param.txt > "${WORKDIR}/ddrbin_param_baud.txt"
		# Patch every DDR blob the ini references: Path1 under
		# [CODE471_OPTION] (the CODE472 Path1 is the usbplug, not DDR)
		# and the FlashData loader entry
		awk -F= '/^\[/{s=$0} s=="[CODE471_OPTION]" && /^Path1=/{print $2} /^FlashData=/{print $2}' \
			"${RKBIN_LOADER_INI}" | tr -d '\r' | sort -u | while read -r DDRBIN; do
			python3 tools/ddrbin_tool.py "${RKBIN_DDRBIN_CHIP}" \
				"${WORKDIR}/ddrbin_param_baud.txt" "${DDRBIN}"
		done
	fi
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
