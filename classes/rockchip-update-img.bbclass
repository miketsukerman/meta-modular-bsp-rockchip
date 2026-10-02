# Rockchip update.img image type
#
# Packs a Rockchip "update.img" suitable for flashing the eMMC with the
# vendor tools, as used by the Advantech BSPs
# (https://docs.aim-linux.advantech.com/docs/bsp/rockchip/General/Update-Image/SD-Card):
#   - an SD upgrade card created with SDDiskTool / SD_Firmware_Tool (Windows)
#   - USB OTG loader/maskrom mode with upgrade_tool / rkdeveloptool (Linux)
#
# The image contains:
#   bootloader  MiniLoaderAll.bin  (rkbin miniloader, from rockchip-rkbin-loader)
#   parameter   parameter.txt      (Rockchip partition map, generated here)
#   uboot       uboot.img          (U-Boot proper FIT, as deployed by u-boot)
#   rootfs      rootfs.img         (the ext4 rootfs; kernel fitImage and
#                                   extlinux.conf live in /boot of the rootfs)
#
# The parameter.txt partition offsets mirror the wic layout from upstream
# meta-rockchip (rockchip.wks): U-Boot proper at sector 0x4000, rootfs at
# sector 0x8000, so an eMMC flashed from update.img is identical to one
# flashed from the wic image.
#
# Usage (machine configuration):
#   IMAGE_CLASSES += "rockchip-update-img"
#   IMAGE_FSTYPES += "update-img"

# Chip id passed to rkImageMaker
RK_UPDATE_CHIP ?= ""
RK_UPDATE_CHIP:rk3568 = "RK3568"

RK_UPDATE_FIRMWARE_VER ?= "1.0"
RK_UPDATE_MACHINE_MODEL ?= "${MACHINE}"
RK_UPDATE_MANUFACTURER ?= "Advantech"

# U-Boot proper image flashed to the "uboot" partition. The RK356x rkbin
# miniloader (FlashBoot SPL, NEWIDB) loads a standard FIT from this
# partition, which is exactly what mainline U-Boot deploys as u-boot.itb.
# If mainline U-Boot proper turns out not to chain-load under the vendor
# miniloader, override this with a loaderimage-wrapped vendor U-Boot.
RK_UPDATE_UBOOT_IMAGE ?= "u-boot.${UBOOT_SUFFIX}"

# Partition map (Rockchip mtdparts syntax, units are 512-byte sectors),
# kept consistent with rockchip.wks and with root=PARTLABEL=rootfsA used
# by the extlinux configuration from upstream meta-rockchip
RK_UPDATE_PARAMETER_MTDPARTS ?= "0x00002000@0x00004000(uboot),-@0x00008000(rootfsA:grow)"

IMAGE_TYPEDEP:update-img = "ext4"

do_image_update_img[depends] += " \
    rockchip-pack-tools-native:do_populate_sysroot \
    rockchip-rkbin-loader:do_deploy \
    virtual/bootloader:do_deploy \
"

IMAGE_CMD:update-img () {
    PKGDIR="${WORKDIR}/update-img"
    rm -rf "${PKGDIR}"
    mkdir -p "${PKGDIR}/Image"

    cp "${DEPLOY_DIR_IMAGE}/MiniLoaderAll.bin" "${PKGDIR}/Image/MiniLoaderAll.bin"
    cp "${DEPLOY_DIR_IMAGE}/${RK_UPDATE_UBOOT_IMAGE}" "${PKGDIR}/Image/uboot.img"
    ln -sf "${IMGDEPLOYDIR}/${IMAGE_NAME}.ext4" "${PKGDIR}/Image/rootfs.img"

    cat > "${PKGDIR}/Image/parameter.txt" <<EOF
FIRMWARE_VER: ${RK_UPDATE_FIRMWARE_VER}
MACHINE_MODEL: ${RK_UPDATE_MACHINE_MODEL}
MACHINE_ID: 007
MANUFACTURER: ${RK_UPDATE_MANUFACTURER}
MAGIC: 0x5041524B
ATAG: 0x00200800
MACHINE: 0xffffffff
CHECK_MASK: 0x80
PWR_HLD: 0,0,A,0,1
TYPE: GPT
CMDLINE: mtdparts=rk29xxnand:${RK_UPDATE_PARAMETER_MTDPARTS}
EOF

    cat > "${PKGDIR}/package-file" <<EOF
# NAME          Relative path
package-file    package-file
bootloader      Image/MiniLoaderAll.bin
parameter       Image/parameter.txt
uboot           Image/uboot.img
rootfsA         Image/rootfs.img
backup          RESERVED
EOF

    if [ -z "${RK_UPDATE_CHIP}" ]; then
        bbfatal "Non-empty RK_UPDATE_CHIP:<MACHINE> required!"
    fi

    cd "${PKGDIR}"
    afptool -pack ./ "${PKGDIR}/Image/update.img"
    rkImageMaker "-${RK_UPDATE_CHIP}" "${PKGDIR}/Image/MiniLoaderAll.bin" \
        "${PKGDIR}/Image/update.img" \
        "${IMGDEPLOYDIR}/${IMAGE_NAME}.update-img" \
        -os_type:androidos

    rm -rf "${PKGDIR}"
}
