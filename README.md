# Advantech BSP for Rockchip based boards

This reference BSP adds minimalistic changes to the upstream
[meta-rockchip](https://git.yoctoproject.org/meta-rockchip) layer to support
Advantech boards. Note that some boards may have only limited features with
this BSP. However, more features can be added in another layer on top of this
layer.

The following boards are supported by this layer:

Preliminary support:

* RSB-4810 (MACHINE=`rsb4810` Rockchip RK3568)
* AOM-6881 + SOM-DB2510-R0A1 (MACHINE=`aom6881` Rockchip RK3588)

## Dependencies

This layer depends on the following layers (all on the `wrynose` branch):

* [openembedded-core](https://git.openembedded.org/openembedded-core) (or poky)
* [meta-arm](https://git.yoctoproject.org/meta-arm) (`meta-arm` and `meta-arm-toolchain`)
* [meta-rockchip](https://git.yoctoproject.org/meta-rockchip)

Optional:

* [meta-oe](https://git.openembedded.org/meta-openembedded) (`meta-openembedded/meta-oe`)
  for the LVGL integration (see [LVGL](#rsb-4810-lvgl)); its recipes live in
  `dynamic-layers/openembedded-layer/` and are only parsed when meta-oe is in
  `bblayers.conf`

## Building

Set up a build as usual for openembedded-core/poky, add the layers above and
this layer to `conf/bblayers.conf`, then:

```
MACHINE=rsb4810 bitbake core-image-minimal
```

(or `MACHINE=aom6881` for the AOM-6881 on SOM-DB2510).

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
  then run `rkdeveloptool ef` (or `wl 0 <image>.wic` to flash directly).
  Note: on RK356x the loader's usbplug stage keeps the maskrom USB
  protocol, so `rkdeveloptool ld` still reports *Maskrom* after a
  successful `db` — that is expected, just proceed with the flash
  command. The usbplug state is lost on any USB replug/power-cycle;
  re-run `db` right before flashing if in doubt.
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

Serial console: **115200;ttyS2** (debug UART). The whole stack runs at
115200 instead of the Rockchip-default 1.5 MBd, because many USB-UART
adapters (classic CP2102, older PL2303) cannot generate an accurate
1500000 baud:

* miniloader DDR blob — `RKBIN_UART_BAUDRATE` in `conf/machine/rsb4810.conf`,
  applied with rkbin's `ddrbin_tool` (rockchip-rkbin-loader recipe);
* U-Boot — `baudrate-115200.cfg` fragment (`CONFIG_BAUDRATE=115200`) in
  `recipes-bsp/u-boot/u-boot/rsb4810.inc`;
* kernel `console=`/getty — `SERIAL_CONSOLES = "115200;ttyS2"` (upstream
  `rockchip-extlinux.inc` derives the extlinux console args from it);
* devicetree `stdout-path = "serial2:115200n8"` (earlycon) in both the
  kernel and U-Boot copies of `rk3568-rsb4810.dts`.

To return to the stock 1500000 rate, revert these four spots together —
a mixed-rate stack means garbage from whichever stage disagrees with the
terminal.

The systemd `osc-context` feature (systemd ≥ 257,
`/etc/profile.d/80-systemd-osc-context.sh`) is disabled for this machine
(`recipes-core/systemd/systemd_%.bbappend`): it makes bash wrap every
prompt/command in OSC 3008 escape sequences
(`3008;start=…;machineid=…;bootid=…`) that minicom/picocom print as
literal garbage on the serial console.

| Device    | Status | Comment                                     |
| --------- | ------ | ------------------------------------------- |
| eMMC      | ✅     | Boots flashed image                          |
| SD Card   | ⚠️     | Boots wic image (needs empty eMMC loader), not tested |
| ETH0      | ⚠️     | 1Gbps (GMAC0, RTL8211F PHY); driver now built-in (rsb4810-eth-usb.cfg), retest |
| ETH1      | ⚠️     | 1Gbps (GMAC1, RTL8211F PHY); driver now built-in (rsb4810-eth-usb.cfg), retest |
| USB 2.0   | ⚠️     | Combo-PHY driver now built-in (rsb4810-eth-usb.cfg), retest |
| USB 3.0   | ⚠️     | Combo-PHY driver now built-in (rsb4810-eth-usb.cfg), retest |
| HDMI      | ⚠️     | Not tested; X11 images need XSERVER:remove = "xserver-xorg-xvfb" (in rsb4810.conf) |
| LVDS      | ⚠️     | LVDS0 via out-of-tree patches + `lvds` overlay, not tested (see [LVDS panel](#rsb-4810-lvds-panel)) |
| eDP       | ❌     | Needs porting from the vendor kernel        |
| PCIe/M.2  | ⚠️     | Not tested                                  |
| Mini-PCIe | ⚠️     | Not tested                                  |
| UART      | ⚠️     | Debug console on ttyS2 @ 115200 (whole stack) |
| I2C       | ⚠️     | Not tested                                  |
| CAN       | ❌     | Needs porting from the vendor kernel        |
| RTC       | ❌     | External RTC needs porting                  |
| Watchdog  | ⚠️     | Internal watchdog, not tested; EC watchdog needs porting |
| Wi-Fi/BT  | ❌     | M.2 module firmware packaging needed        |

Legend: ✅ working, ⚠️ untested/partial, ❌ not yet supported.

### RSB-4810 LVDS panel

The RK3568 LVDS transmitter is not supported by mainline Linux. This layer
carries the series "drm/rockchip: add RK3568 LVDS support" by Rok Markovic
([lore](https://lore.kernel.org/all/20260724080824.115260-1-rok@kanardia.eu/),
submitted upstream) as
`recipes-kernel/linux/linux-yocto/rsb4810/000[1-4]-*.patch`, plus the
`rsb4810-lvds.cfg` kernel config fragment and the devicetree overlay
`rk3568-rsb4810-lvds.dtso`. Limitations: single-channel LVDS0 only,
driven by VOP2 video port 1 (vp0 drives HDMI, vp2 has a known bug).

The overlay is set up for the Advantech IDK-1107WR-40WVA1E kit (AUO
G070VW01 V0, 7" 800x480, listed as `lvds-g070vw01` in Advantech's
RSB-4810 BSP). The timings come from Advantech's Rockchip vendor kernel:
33.3 MHz, htotal 1056, vtotal 508 (~62 Hz), DE mode, `vesa-24`.

The panel control lines (schematic sheets 12/26) are listed below. The
schematic net names end in `_GPIO3_C6`/`_GPIO3_C7`; these are stale labels
from the Rockchip reference template. GPIO3_C6..D3 carry SDMMC2 for the
M.2 SDIO Wi-Fi, and the LVDS overlay does not touch them.

| Net            | Pin      | Function                                              |
| -------------- | -------- | ----------------------------------------------------- |
| `LCD_EN_H`     | GPIO3_D4 | panel VDD load switch (`VDD_LVDS`), `vdd_lvds` regulator |
| `LCD_RST_L`    | GPIO3_D5 | 12 V backlight power (`+VDD_BKLT`, Q87/Q86), `vdd_bklt` regulator; not a panel reset |
| `LVDS_BLENH`   | GPIO0_A0 | backlight enable, `BL` pin 3                          |
| `LCD0_BL_PWM4` | PWM4     | backlight dimming, `BL` pin 4                         |

**Enabling the overlay.** `rk3568-rsb4810-lvds.dtbo` is built with the
kernel, deployed next to the DTB and packed into the fitImage. The boot
script applies the overlays listed in the U-Boot variable `overlays`.
The variable is unset by default, so HDMI-only setups are unaffected. At
the U-Boot prompt:

```
setenv overlays lvds
saveenv
boot
```

The U-Boot environment is stored on the eMMC (`saveenv.cfg`), so the
setting persists across reboots. The boot log then shows `Applied
devicetree overlay rk3568-rsb4810-lvds.dtbo`. If extracting or applying
an overlay fails, an error is printed and the board boots with the plain
devicetree. Use `setenv overlays; saveenv` to disable it again.

**Jumpers.** The panel and backlight supply voltages are selected by
jumpers only; there is no software control (see the
[Advantech jumper list](https://docs.aim-linux.advantech.com/docs/processors/rockchip/RK3568/RSB-4810_wiki_data/RSB-4810_Debian11_User_Guide)).
Set them to match the panel before connecting it:

| Jumper | Function                       | Settings                       | IDK-1107WR (G070VW01) |
| ------ | ------------------------------ | ------------------------------ | --------------------- |
| `VDD2` | LVDS panel VDD (`VDD_LVDS`)    | 1-2: 3.3 V (default), 2-3: 5 V | 1-2 (3.3 V)           |
| `BLP1` | LVDS backlight power, 3.3/5 V  | 1-2: 5 V (default), 2-3: 3.3 V | removed               |
| `BLP2` | LVDS backlight power, 12 V     | 1-2: 12 V                      | fitted (12 V)         |

The backlight cable goes to the 5-pin `BL` connector (pin 1 `+VDD_BKLT`,
2 GND, 3 `LVDS_BLEN`, 4 `LVDS_PWM`, 5 +5 V). The LVDS pairs go to the
40-pin `LVDS` connector.

**Testing.** `modetest` (from `libdrm-tests`), `gpioset`/`gpioget`
(from `libgpiod-tools`) and `kmscube` come with
`packagegroup-advantech-display-test`. It is included in core-image-base
and larger images (see `conf/machine/rsb4810.conf`); for
core-image-minimal add it via `CORE_IMAGE_EXTRA_INSTALL`.

```sh
dmesg | grep -iE 'lvds|vop2|dsidphy|panel'
cat /sys/class/drm/card*-LVDS-1/status
modetest -M rockchip -c
modetest -M rockchip -s <conn_id>@<crtc_id>:800x480 -v
echo 255 > /sys/class/backlight/backlight-lvds/brightness
```

Take `<conn_id>` from the LVDS connector line of `modetest -M rockchip
-c` and `<crtc_id>` from the CRTC belonging to VOP2 vp1 (`modetest -M
rockchip -p`).

The G070VW01 dimming input is analog (0.3-5 V), so the brightness may not
track the PWM duty cycle. The overlay defaults to full brightness (255).

**Troubleshooting.**

* *Backlight on, screen black:* check that the LVDS encoder is fed by
  VOP2 **vp1** (`&vp1` endpoint in the overlay, and the CRTC used in
  `modetest`). Then check `data-mapping`: `vesa-24` by default, `jeida-18`
  if the panel's SEL68 pin is strapped low. A wrong mapping usually gives
  a black or garbled picture. If the panel does not lock, try the
  alternative 29.5 MHz timing noted in the overlay. Also measure
  `VDD_LVDS` on the 40-pin connector (pins 1/2/5/6/40); it must be 3.3 V
  while GPIO3_D4 is high.
* *Backlight off:* check the jumpers, then test the lines independently
  with the overlay disabled (`setenv overlays; saveenv`). `gpioset -z`
  holds a line in the background until `pkill gpioset`:

  ```sh
  gpioset -z -c gpiochip3 29=1   # GPIO3_D5: +VDD_BKLT (BL pin 1) -> 12 V
  gpioset -z -c gpiochip0 0=1    # GPIO0_A0: LVDS_BLEN (BL pin 3)
  ```

  Test PWM4 through `/sys/class/pwm`: export channel 0, then set `period`
  25000, `duty_cycle` and `enable` 1.

### RSB-4810 LVGL

[LVGL](https://lvgl.io/) v9.4 from meta-oe (`lvgl_9.4.0.bb`) is configured for
the RK3568 by `dynamic-layers/openembedded-layer/recipes-graphics/lvgl/lvgl_%.bbappend`
(applied only when `rk3568` is in `MACHINEOVERRIDES`; other machines get the
plain meta-oe recipe). LVGL draws directly on the DRM/KMS device through its
own Linux DRM driver, without a display server, and reads pointer and touch
input through evdev.

**Enabling it in an image.** Add meta-oe to `bblayers.conf`, then in
`local.conf`:

```
EXTRA_IMAGE_FEATURES += "lvgl"
```

This installs `packagegroup-advantech-lvgl`: `liblvgl`, `liblvgl_demos` and
the `lvgl-demo-drm` widgets demo. Nothing is added to images by default.
Applications build against the installed library with
`DEPENDS += "lvgl"` and `pkg-config lvgl` (headers in `/usr/include/lvgl`,
including the generated `lv_conf.h`).

**EGL vs. software rendering.**

* *EGL/OpenGL ES (default with the `opengl` distro feature):* LVGL renders
  through EGL on a GBM surface, with its OpenGL ES draw unit, on the
  Mali-G52 GPU. meta-rockchip builds Mesa with the panfrost driver
  (`mesa.bbappend`), and lvgl pulls in Mesa's DRI drivers and the panfrost
  kernel module (`mesa-megadriver`, `kernel-module-panfrost`). LVGL v9.4 has no NanoVG renderer; that
  arrives in v9.5.
* *Software:* LVGL renders on the CPU with 4 draw threads into DRM dumb
  buffers, needing only libdrm. This is used automatically without
  `opengl`, or can be forced with:

  ```
  PACKAGECONFIG:remove:pn-lvgl = "egl"
  ```

* `PACKAGECONFIG:append:pn-lvgl = " libinput"` adds the libinput input
  driver.

Check which backend a build uses in `/usr/include/lvgl/lv_conf.h` of the SDK
or sysroot (`LV_LINUX_DRM_USE_EGL`). If the EGL demo fails to start on the
target, check `dmesg | grep panfrost` and that `/dev/dri/renderD128` exists.

The configuration is in Kconfig fragments next to the bbappend
(`lvgl/rk3568/*.cfg`, LVGL v9.4 symbol names). LVGL v9.4 implements the DRM
EGL backend, but its Kconfig cannot select it, so the bbappend carries a
small Kconfig patch adding a "Linux DRM rendering backend" choice, as found
in LVGL master.

**HDMI vs. LVDS.** The demo uses the first DRM card that has connectors (the
VOP2 `rockchip` card, not the panfrost render node) and the first
*connected* connector. Select them explicitly with environment variables:

| Variable           | Default                           | Meaning                          |
| ------------------ | --------------------------------- | -------------------------------- |
| `LV_DRM_DEVICE`    | auto (`lv_linux_drm_find_device_path()`) | DRM card, e.g. `/dev/dri/card0` |
| `LV_DRM_CONNECTOR` | first connected connector         | connector id                     |
| `LV_EVDEV_DEVICE`  | `/dev/input/event0`               | pointer/touch device, `none` to disable |

Take the connector ids of `HDMI-A-1` and `LVDS-1` from
`modetest -M rockchip -c`. LVDS needs the `lvds` overlay (see
[LVDS panel](#rsb-4810-lvds-panel)).

**Running the demo.** LVGL needs to be the DRM master, so stop any other
display server first (e.g. the X server of `core-image-sato`). Then run:

```sh
lvgl-demo-drm                                            # first connected output
LV_DRM_CONNECTOR=<id> LV_EVDEV_DEVICE=/dev/input/eventN lvgl-demo-drm
```

Find the touchscreen's `eventN` in `/proc/bus/input/devices`. To start the
demo at boot, install `lvgl-demo-drm-service` (not part of the
packagegroup). Set the variables with `systemctl edit lvgl-demo`, then run
`systemctl enable --now lvgl-demo`.

**Known limitations.**

* LVGL has no draw unit for the Rockchip RGA 2D engine; rendering is either
  CPU (software) or GPU via OpenGL ES.
* One LVGL display per DRM device: the LVGL DRM driver opens the card as DRM
  master and drives a single connector/CRTC, so HDMI and LVDS cannot be
  driven as two LVGL displays at the same time.
* No display server: LVGL cannot run while Weston or X owns the DRM device.
* Neither backend has been tested on RSB-4810 hardware yet.

## AOM-6881 + SOM-DB2510

SMARC 2.1 module based on the Rockchip RK3588 (quad Cortex-A76 + quad
Cortex-A55) on the Advantech SOM-DB2510-R0A1 carrier board, using the
mainline-based `linux-yocto` kernel and mainline U-Boot
(`evb-rk3588_defconfig` with the AOM-6881 devicetree). The module follows
the Rockchip RK3588 EVB1 reference design (dual RK806 PMIC on SPI2,
RTL8211F RGMII PHYs), so the devicetree is derived from the mainline
`rk3588-evb1-v10.dts` with board values (GMAC delays, PHY addresses) from
the Advantech ROM-6881 vendor BSP.

All the image artifacts and flashing flows described above for the
RSB-4810 apply unchanged (replace `rsb4810` with `aom6881` in the paths):
bootable wic image, `update.img` for `upgrade_tool`/`rkdeveloptool` over
USB OTG (the `rkdeveloptool db MiniLoaderAll.bin` first step in maskrom
mode applies to the RK3588 as well — the RK3588 miniloader comes from
rkbin's `RKBOOT/RK3588MINIALL.ini`), and the self-flashing SD upgrade
card. Like the RK3568, the RK3588 BootROM prefers the eMMC, so SD boot
needs an empty/erased eMMC loader area.

Serial console: **115200;ttyS2** (debug UART, UART2), the same
whole-stack 115200 setup as the RSB-4810 (miniloader DDR blob via
`RKBIN_UART_BAUDRATE`, U-Boot `baudrate-115200.cfg`, `SERIAL_CONSOLES`,
devicetree `stdout-path = "serial2:115200n8"`); the systemd `osc-context`
feature is disabled for this machine too.

| Device    | Status | Comment                                     |
| --------- | ------ | ------------------------------------------- |
| eMMC      | ⚠️     | Not tested                                  |
| SD Card   | ⚠️     | Not tested (needs empty eMMC loader)        |
| ETH0      | ⚠️     | 1Gbps (GMAC0, RTL8211F PHY); driver built-in (aom6881-eth-usb.cfg), not tested |
| ETH1      | ⚠️     | 1Gbps (GMAC1, RTL8211F PHY); driver built-in (aom6881-eth-usb.cfg), not tested |
| USB 2.0   | ⚠️     | Not tested; VBUS enable GPIOs unknown (regulators modelled always-on) |
| USB 3.0   | ⚠️     | USBDP/combo PHY drivers built-in (aom6881-eth-usb.cfg), not tested |
| USB OTG   | ⚠️     | Runs in host mode; Type-C PD (FUSB302) hookup needs the vendor schematic |
| HDMI/DP   | ❌     | Display output needs porting                 |
| eDP/LVDS  | ❌     | Needs porting from the vendor kernel        |
| PCIe/M.2  | ❌     | Carrier PCIe slots need porting (reset/power GPIOs unknown) |
| SATA      | ❌     | Not wired up yet                            |
| UART      | ⚠️     | Debug console on ttyS2 @ 115200 (whole stack) |
| I2C       | ⚠️     | Not tested                                  |
| CAN       | ❌     | Likely Advantech USB SLCAN-FD MCU, needs porting |
| RTC       | ⚠️     | HYM8563 on I2C6 (EVB1 reference wiring), not tested |
| Watchdog  | ⚠️     | Internal watchdog, not tested               |

Legend: ✅ working, ⚠️ untested/partial, ❌ not yet supported.

## Layer structure

```
classes/rockchip-update-img.bbclass    Rockchip update.img image type (eMMC upgrade)
classes/rockchip-sdupdate-img.bbclass  SD upgrade card image type (wic + RK_UPDATE volume)
conf/machine/rsb4810.conf              machine configuration (thin, on top of
                                       meta-rockchip's conf/machine/include/rk3568.inc)
conf/machine/aom6881.conf              machine configuration (thin, on top of
                                       meta-rockchip's conf/machine/include/rk3588.inc)
recipes-bsp/rkbin/                     Rockchip miniloader (MiniLoaderAll.bin via boot_merger)
recipes-bsp/rockchip-pack-tools/       native afptool/rkImageMaker packaging tools
recipes-bsp/rockchip-sd-installer/     boot service flashing the eMMC from an SD upgrade card
recipes-bsp/u-boot/                    U-Boot board integration (per-machine .inc + files)
recipes-graphics/packagegroups/        display test tools (modetest, kmscube)
recipes-kernel/linux/                  kernel board integration:
  linux-yocto/<machine>.inc            per-machine devicetree/config wiring
  linux-yocto/<machine>/               board devicetree sources, overlays, patches
  linux-yocto/common/                  shared kernel config fragments
dynamic-layers/openembedded-layer/     only parsed with meta-oe:
  recipes-graphics/lvgl/               lvgl bbappend (per-SoC .inc, Kconfig
                                       fragments), lvgl-demo-drm
  recipes-graphics/packagegroups/      packagegroup-advantech-lvgl
```

RAM/storage variants of a board follow the per-variant machine configuration
pattern (e.g. `rsb4810-4g.conf` requiring `rsb4810.conf`), as done in
[meta-modular-bsp-nxp](https://github.com/miketsukerman/meta-modular-bsp-nxp).
