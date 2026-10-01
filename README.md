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

The resulting wic image can be written to an SD card or to the eMMC. The DDR
init blob (TPL), TF-A (BL31) and OP-TEE come prebuilt from the
`rockchip-rkbin-*` recipes provided by upstream meta-rockchip; no additional
firmware setup is needed.

## RSB-4810

3.5" SBC based on the Rockchip RK3568 (quad Cortex-A53/A55 class, Cortex-A55),
using the mainline-based `linux-yocto` kernel and mainline U-Boot
(`evb-rk3568_defconfig` with the RSB-4810 devicetree).

Serial console: 1500000;ttyS2 (debug UART).

| Device    | Status | Comment                                     |
| --------- | ------ | ------------------------------------------- |
| eMMC      | ⚠️     | Not tested                                  |
| SD Card   | ⚠️     | Not tested                                  |
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
conf/machine/rsb4810.conf              machine configuration (thin, on top of
                                       meta-rockchip's conf/machine/include/rk3568.inc)
recipes-bsp/u-boot/                    U-Boot board integration (per-machine .inc + files)
recipes-kernel/linux/                  kernel board integration:
  linux-yocto/<machine>.inc            per-machine devicetree/config wiring
  linux-yocto/<machine>/               board devicetree sources
  linux-yocto/common/                  shared kernel config fragments
```

RAM/storage variants of a board follow the per-variant machine configuration
pattern (e.g. `rsb4810-4g.conf` requiring `rsb4810.conf`), as done in
[meta-modular-bsp-nxp](https://github.com/miketsukerman/meta-modular-bsp-nxp).
