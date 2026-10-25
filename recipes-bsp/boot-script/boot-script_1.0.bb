SUMMARY = "U-Boot boot script for the Advantech RSB-4810"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = "u-boot-tools-native"

SRC_URI = "file://boot.cmd"
S = "${UNPACKDIR}"

COMPATIBLE_MACHINE = "^rsb4810$"
PACKAGE_ARCH = "${MACHINE_ARCH}"

do_compile() {
    mkimage -A arm64 -O linux -T script -C none \
        -n "RSB-4810 boot script" \
        -d ${S}/boot.cmd ${B}/boot.scr
}

do_install() {
    install -d ${D}/boot
    install -m 0644 ${B}/boot.scr ${D}/boot/boot.scr
}

FILES:${PN} = "/boot/boot.scr"
