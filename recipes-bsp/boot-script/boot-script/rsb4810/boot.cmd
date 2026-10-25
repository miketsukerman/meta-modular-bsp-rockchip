echo "Booting from ${devtype} ${devnum}:${distro_bootpart}"

setenv bootargs "root=/dev/mmcblk${devnum}p${distro_bootpart} rootwait rw rootfstype=ext4 earlycon console=tty1 console=ttyS2,115200n8"

load ${devtype} ${devnum}:${distro_bootpart} ${kernel_addr_r} /boot/fitImage
bootm ${kernel_addr_r}#conf-rk3568-rsb4810.dtb