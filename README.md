# Advantech BSP for Rockchip based boards

This reference BSP adds minimalistic changes to the upstream
[meta-rockchip](https://git.yoctoproject.org/meta-rockchip) layer to support
Advantech boards. Note that some boards may have only limited features with
this BSP. However, more features can be added in another layer on top of this
layer.

The following boards are supported by this layer:

Preliminary support:

* RSB-4810 (MACHINE=`rsb4810` Rockchip RK3568)

## Dependencies

This layer depends on the following layers (all on the `wrynose` branch):

* [openembedded-core](https://git.openembedded.org/openembedded-core) (or poky)
* [meta-arm](https://git.yoctoproject.org/meta-arm) (`meta-arm` and `meta-arm-toolchain`)
* [meta-rockchip](https://git.yoctoproject.org/meta-rockchip)

## Building

Set up a build as usual for openembedded-core/poky, add the layers above and
this layer to `conf/bblayers.conf`, then:

```
MACHINE=rsb4810 bitbake core-image-minimal
```

The resulting wic image can be written to an SD card or directly to the
eMMC. The DDR init blob (TPL), TF-A (BL31) and OP-TEE come prebuilt from the
`rockchip-rkbin-*` recipes provided by upstream meta-rockchip; no additional
firmware setup is needed.

## Bootable SD card image (wic)

Every image build produces a bootable disk image
`tmp/deploy/images/rsb4810/<image>.rootfs.wic` (plus a `.wic.bmap`), using
the canonical Rockchip SD card layout from upstream meta-rockchip
(`rockchip.wks`): GPT partition table, `idbloader.img` (TPL+SPL) as raw
sectors at sector 64, U-Boot proper (`u-boot.itb`) at sector 16384 and the
ext4 root filesystem (partition label `rootfsA`) at sector 32768. There is
no FAT boot partition — the kernel fitImage and the extlinux configuration
live in `/boot` of the root filesystem and are loaded by U-Boot's extlinux
support.

Flash it to an SD card with bmaptool (fast, recommended):

```
bmaptool copy tmp/deploy/images/rsb4810/<image>.rootfs.wic /dev/sdX
```

or with plain dd:

```
dd if=tmp/deploy/images/rsb4810/<image>.rootfs.wic of=/dev/sdX bs=4M conv=fsync
```

Board-specific notes:

* **Boot order:** the RK3568 BootROM probes SPI NOR, then eMMC, then SD.
  The board therefore boots from SD only while the eMMC bootloader area is
  empty — on a factory board the preinstalled Advantech firmware on eMMC
  wins. To force SD boot, erase the eMMC loader first (loader/maskrom mode
  over USB OTG: `rkdeveloptool ef` or `upgrade_tool ef`, see below), or use
  the `update.img` flow below to replace the eMMC contents entirely.
* In **maskrom** mode only the BootROM answers on USB and it has no flash
  driver: every flash command (`ef`, `ppt`, `td`, `wl`, ...) fails with
  "Getting flash info from device failed!" until a loader is downloaded
  into RAM first. Run `rkdeveloptool db MiniLoaderAll.bin` (deployed by
  the build next to the images, from the `rockchip-rkbin-loader` recipe),
  wait for the device to re-enumerate as *Loader* in `rkdeveloptool ld`,
  then run `rkdeveloptool ef` (or `wl 0 <image>.wic` to flash directly).
* Once SPL (loaded from SD) runs, it continues from the SD card
  (`same-as-spl` boot order), so U-Boot, kernel and rootfs are all taken
  from the card.
* If both the SD card and the eMMC carry a `rootfsA` partition label,
  extlinux's `root=PARTLABEL=rootfsA` may resolve to the eMMC partition;
  prefer a blank/erased eMMC while validating SD boot.

## Updating the RSB-4810 (update.img)

The build automatically produces a Rockchip `update.img`
(`tmp/deploy/images/rsb4810/<image>.update-img`) packing the miniloader
(`MiniLoaderAll.bin`, from rkbin via `boot_merger`), the partition map
(`parameter.txt`), U-Boot proper and the ext4 rootfs with `afptool` +
`rkImageMaker` — the same flow Advantech uses in its
[Debian BSP](https://docs.aim-linux.advantech.com/docs/bsp/rockchip/Debian/Debian11/RK3568/).
The partition offsets in `parameter.txt` match the wic layout, so an eMMC
flashed from `update.img` is identical to one flashed from the wic image.

Two ways to flash it:

* **SD upgrade card (Windows):** write `update.img` to a microSD card with
  SDDiskTool / SD_Firmware_Tool ("Upgrade Firmware" mode, per the
  [Advantech instructions](https://docs.aim-linux.advantech.com/docs/bsp/rockchip/General/Update-Image/SD-Card)),
  insert it into the powered-off board and power on. The miniloader flashes
  the eMMC and prints "Please remove SD CARD!!!" on the serial console;
  remove the card and the board reboots from eMMC.
* **USB OTG (Linux, scriptable):** put the board into loader/maskrom mode
  (recovery button during power-on), connect the OTG port and run
  `upgrade_tool uf update.img` (from rkbin `tools/`) or use `rkdeveloptool`.

## SD upgrade card (Linux, no SDDiskTool)

The build also produces a ready-made SD upgrade card image
(`tmp/deploy/images/rsb4810/<image>.sdupdate-img`), the Linux replacement
for the SDDiskTool step: write it to a microSD card with `dd` (the file is
sparse, so use `conv=sparse` only to another file — to a real card use):

```
dd if=tmp/deploy/images/rsb4810/<image>.sdupdate-img of=/dev/sdX bs=4M conv=fsync
```

The card is the normal bootable wic image plus an extra FAT32 partition
labelled `RK_UPDATE` holding `sd_boot_config.config` (`fw_update=1`) and a
copy of the wic image as flash payload. When the board boots from the
card, the `rockchip-sd-installer` service (installed in every image for
this machine) detects the upgrade volume, writes the payload to the eMMC,
clears the `fw_update` flag, prints `Please remove SD CARD!!!` on the
console and reboots — after which the board boots the new firmware from
eMMC, exactly like the vendor upgrade flow.

Notes:

* **Triggering SD boot:** the RK3568 BootROM prefers the eMMC. On a board
  with a working eMMC loader, erase it once (loader/maskrom mode over USB
  OTG: `rkdeveloptool db MiniLoaderAll.bin` then `rkdeveloptool ef`, or
  `upgrade_tool ef`) or hold the recovery
  button so the BootROM falls through to the SD card. A board with a
  blank/bricked eMMC boots the upgrade card directly.
* The vendor SDDiskTool card format itself (RC4-encoded legacy IDBlock at
  sector 64 + vendor recovery ramdisk) is not used: RK356x BootROMs
  consume the newer unencrypted NEWIDB loader format and this BSP's
  mainline U-Boot has no vendor recovery handoff. The wic-based card
  boots through the standard idbloader/U-Boot/extlinux path instead.
* The flag is cleared after flashing, so rebooting with the card still
  inserted does not flash twice; the card can be re-armed by setting
  `fw_update=1` in `sd_boot_config.config` again.

The three build artifacts at a glance:

| Artifact          | Use                                                        |
| ----------------- | ---------------------------------------------------------- |
| `<image>.wic`     | direct dd to SD/eMMC; SD boot needs empty eMMC loader      |
| `<image>.update-img` | `upgrade_tool uf` / `rkdeveloptool` over USB OTG, or Windows SDDiskTool input |
| `<image>.sdupdate-img` | dd to SD card → boots and self-flashes the eMMC       |

## RSB-4810

3.5" SBC based on the Rockchip RK3568 (quad Cortex-A53/A55 class, Cortex-A55),
using the mainline-based `linux-yocto` kernel and mainline U-Boot
(`evb-rk3568_defconfig` with the RSB-4810 devicetree).

Serial console: 1500000;ttyS2 (debug UART).

| Device    | Status | Comment                                     |
| --------- | ------ | ------------------------------------------- |
| eMMC      | ⚠️     | Not tested                                  |
| SD Card   | ⚠️     | Boots wic image (needs empty eMMC loader), not tested |
| ETH0      | ⚠️     | 1Gbps (GMAC0, RTL8211F PHY), not tested     |
| ETH1      | ⚠️     | 1Gbps (GMAC1, RTL8211F PHY), not tested     |
| USB 2.0   | ⚠️     | Not tested                                  |
| USB 3.0   | ⚠️     | Not tested                                  |
| HDMI      | ⚠️     | Not tested                                  |
| LVDS/eDP  | ❌     | Needs porting from the vendor kernel        |
| PCIe/M.2  | ⚠️     | Not tested                                  |
| Mini-PCIe | ⚠️     | Not tested                                  |
| UART      | ⚠️     | Debug console expected on ttyS2 @ 1500000   |
| I2C       | ⚠️     | Not tested                                  |
| CAN       | ❌     | Needs porting from the vendor kernel        |
| RTC       | ❌     | External RTC needs porting                  |
| Watchdog  | ⚠️     | Internal watchdog, not tested; EC watchdog needs porting |
| Wi-Fi/BT  | ❌     | M.2 module firmware packaging needed        |

Legend: ✅ working, ⚠️ untested/partial, ❌ not yet supported.

## Layer structure

```
classes/rockchip-update-img.bbclass    Rockchip update.img image type (eMMC upgrade)
classes/rockchip-sdupdate-img.bbclass  SD upgrade card image type (wic + RK_UPDATE volume)
conf/machine/rsb4810.conf              machine configuration (thin, on top of
                                       meta-rockchip's conf/machine/include/rk3568.inc)
recipes-bsp/rkbin/                     Rockchip miniloader (MiniLoaderAll.bin via boot_merger)
recipes-bsp/rockchip-pack-tools/       native afptool/rkImageMaker packaging tools
recipes-bsp/rockchip-sd-installer/     boot service flashing the eMMC from an SD upgrade card
recipes-bsp/u-boot/                    U-Boot board integration (per-machine .inc + files)
recipes-kernel/linux/                  kernel board integration:
  linux-yocto/<machine>.inc            per-machine devicetree/config wiring
  linux-yocto/<machine>/               board devicetree sources
  linux-yocto/common/                  shared kernel config fragments
```

RAM/storage variants of a board follow the per-variant machine configuration
pattern (e.g. `rsb4810-4g.conf` requiring `rsb4810.conf`), as done in
[meta-modular-bsp-nxp](https://github.com/miketsukerman/meta-modular-bsp-nxp).
