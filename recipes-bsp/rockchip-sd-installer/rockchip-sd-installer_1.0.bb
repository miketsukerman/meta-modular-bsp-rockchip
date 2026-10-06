SUMMARY = "Flash the eMMC from a Rockchip SD upgrade card"
DESCRIPTION = "Boot-time service that, when the system was booted from an \
SD card carrying a FAT volume labelled RK_UPDATE with fw_update=1 in \
sd_boot_config.config (as produced by the sdupdate-img image type), writes \
the wic payload from that volume to the on-board eMMC, clears the flag and \
reboots. The Linux replacement for the vendor SDDiskTool upgrade flow."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://rockchip-sd-installer \
    file://rockchip-sd-installer.init \
    file://rockchip-sd-installer.service \
"

S = "${UNPACKDIR}"

inherit allarch update-rc.d systemd

do_install() {
    install -d ${D}${sbindir}
    install -m 0755 ${S}/rockchip-sd-installer ${D}${sbindir}/rockchip-sd-installer

    install -d ${D}${sysconfdir}/init.d
    install -m 0755 ${S}/rockchip-sd-installer.init \
        ${D}${sysconfdir}/init.d/rockchip-sd-installer

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${S}/rockchip-sd-installer.service \
        ${D}${systemd_system_unitdir}/
}

INITSCRIPT_NAME = "rockchip-sd-installer"
INITSCRIPT_PARAMS = "defaults 99"

SYSTEMD_SERVICE:${PN} = "rockchip-sd-installer.service"

RDEPENDS:${PN} = "util-linux-blkid"
