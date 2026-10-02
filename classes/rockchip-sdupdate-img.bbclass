# Rockchip SD upgrade card image type
#
# Produces a dd-able SD card image that replaces the Windows
# SDDiskTool / SD_Firmware_Tool "upgrade firmware" card for boards running
# this layer's mainline U-Boot (see the Advantech flow at
# https://docs.aim-linux.advantech.com/docs/bsp/rockchip/General/Update-Image/SD-Card):
#
#   sectors 0..<end of wic>   the regular bootable wic image (GPT,
#                             idbloader.img at sector 64, u-boot.itb at
#                             sector 16384, rootfsA at sector 32768)
#   appended GPT partition    FAT32 volume labelled RK_UPDATE containing
#     "userdata"                sd_boot_config.config  (fw_update=1)
#                               <image>.wic            (the eMMC payload)
#
# Booted on a board (eMMC loader erased or SD forced), the rootfs's
# rockchip-sd-installer service detects the RK_UPDATE volume with
# fw_update=1, writes the wic payload to the eMMC, clears the flag,
# prints "Please remove SD CARD!!!" and reboots — after which the board
# boots the freshly flashed eMMC.
#
# Why not the vendor SDDiskTool layout: that format stores an
# RC4-sector-encoded legacy IDBlock at sector 64 and relies on the
# Rockchip vendor U-Boot + recovery ramdisk to perform the flash. RK356x
# BootROMs consume the unencrypted NEWIDB/RKNS loader format
# (RKBOOT/RK3568MINIALL.ini: NEWIDB=true, RC4_OFF=true) and mainline
# U-Boot has no sdfwupdate/recovery handoff, so a vendor-format card can
# not work with this BSP. The wic start of this image is already the
# correct NEWIDB boot layout.
#
# Usage (machine configuration):
#   IMAGE_CLASSES += "rockchip-sdupdate-img"
#   IMAGE_FSTYPES += "sdupdate-img"
#   MACHINE_ESSENTIAL_EXTRA_RRECOMMENDS += "rockchip-sd-installer"

# Free space (MiB) left in the FAT32 RK_UPDATE volume beyond the payload
RK_SDUPDATE_FAT_EXTRA_MB ?= "64"

# FAT32 volume label the installer looks for
RK_SDUPDATE_FAT_LABEL ?= "RK_UPDATE"

IMAGE_TYPEDEP:sdupdate-img = "wic"

do_image_sdupdate_img[depends] += " \
    gptfdisk-native:do_populate_sysroot \
    dosfstools-native:do_populate_sysroot \
    mtools-native:do_populate_sysroot \
"

IMAGE_CMD:sdupdate-img () {
    WIC_IMG="${IMGDEPLOYDIR}/${IMAGE_NAME}.wic"
    OUT_IMG="${IMGDEPLOYDIR}/${IMAGE_NAME}.sdupdate-img"
    WORK="${WORKDIR}/sdupdate-img"

    rm -rf "${WORK}"
    mkdir -p "${WORK}"

    # Apparent (non-sparse) size of the wic payload; FAT32 caps files at
    # 4 GiB - 1 byte.
    WIC_BYTES=$(stat -Lc %s "${WIC_IMG}")
    if [ "${WIC_BYTES}" -ge 4294967295 ]; then
        bbfatal "${IMAGE_NAME}.wic is ${WIC_BYTES} bytes; FAT32 limits the sdupdate-img payload to < 4 GiB. Shrink the image or drop sdupdate-img from IMAGE_FSTYPES."
    fi

    # The RK_UPDATE partition starts at the first MiB boundary at or after
    # the end of the wic image (the wic file ends with its last partition).
    FAT_START_B=$(( (WIC_BYTES + 1048575) / 1048576 * 1048576 ))
    FAT_MB=$(( WIC_BYTES / 1048576 + 1 + ${RK_SDUPDATE_FAT_EXTRA_MB} ))
    # 1 MiB tail for the relocated backup GPT
    TOTAL_B=$(( FAT_START_B + FAT_MB * 1048576 + 1048576 ))

    printf 'fw_update=1\n' > "${WORK}/sd_boot_config.config"

    truncate -s "${FAT_MB}M" "${WORK}/userdata.vfat"
    mkfs.vfat -F 32 -n "${RK_SDUPDATE_FAT_LABEL}" "${WORK}/userdata.vfat" > /dev/null
    mcopy -i "${WORK}/userdata.vfat" "${WORK}/sd_boot_config.config" ::/
    mcopy -i "${WORK}/userdata.vfat" "${WIC_IMG}" "::/${IMAGE_NAME}.wic"

    cp --sparse=always "${WIC_IMG}" "${OUT_IMG}"
    truncate -s "${TOTAL_B}" "${OUT_IMG}"
    dd if="${WORK}/userdata.vfat" of="${OUT_IMG}" bs=1M \
        seek=$(( FAT_START_B / 1048576 )) conv=notrunc,sparse status=none

    # Relocate the backup GPT to the new end of disk and register the
    # RK_UPDATE volume as a "userdata" partition.
    sgdisk -e \
        -n 0:$(( FAT_START_B / 512 )):$(( FAT_START_B / 512 + FAT_MB * 2048 - 1 )) \
        -t 0:0700 -c 0:userdata "${OUT_IMG}" > /dev/null

    rm -rf "${WORK}"
}
