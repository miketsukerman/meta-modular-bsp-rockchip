echo "Booting from ${devtype} ${devnum}:${distro_bootpart}"

setenv bootargs "root=/dev/mmcblk${devnum}p${distro_bootpart} rootwait rw rootfstype=ext4 earlycon console=tty1 console=ttyS2,115200n8"

load ${devtype} ${devnum}:${distro_bootpart} ${kernel_addr_r} /boot/fitImage

# Optional devicetree overlays, off by default. Enable them from the U-Boot
# prompt with a space separated list of overlay names, e.g.
#   setenv overlays lvds; saveenv
# Each name <ov> selects rk3568-rsb4810-<ov>.dtbo, which is packaged in
# the fitImage (KERNEL_DEVICETREE). If anything fails, the board falls
# back to booting the plain devicetree from the fitImage.
setenv fdt_ov_ok 0
if test -n "${overlays}"; then
	if imxtract ${kernel_addr_r} fdt-rk3568-rsb4810.dtb ${fdt_addr_r}; then
		fdt addr ${fdt_addr_r}
		fdt resize 65536
		setenv fdt_ov_ok 1
		for ov in ${overlays}; do
			echo "Applying devicetree overlay rk3568-rsb4810-${ov}.dtbo"
			if imxtract ${kernel_addr_r} fdt-rk3568-rsb4810-${ov}.dtbo ${fdtoverlay_addr_r}; then
				if fdt apply ${fdtoverlay_addr_r}; then
					echo "Applied devicetree overlay rk3568-rsb4810-${ov}.dtbo"
				else
					echo "ERROR: failed to apply rk3568-rsb4810-${ov}.dtbo"
					setenv fdt_ov_ok 0
				fi
			else
				echo "ERROR: rk3568-rsb4810-${ov}.dtbo not found in fitImage"
				setenv fdt_ov_ok 0
			fi
		done
	fi
fi

if test "${fdt_ov_ok}" = "1"; then
	bootm ${kernel_addr_r}#conf-rk3568-rsb4810.dtb - ${fdt_addr_r}
fi
if test -n "${overlays}"; then
	echo "Overlays not applied, booting without them"
fi
bootm ${kernel_addr_r}#conf-rk3568-rsb4810.dtb
