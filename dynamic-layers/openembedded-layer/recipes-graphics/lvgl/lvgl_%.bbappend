# LVGL configuration for RK3568 boards (RSB-4810), see lvgl/rk3568.inc.
# Other machines build meta-oe's lvgl unchanged.
require ${@ 'lvgl/rk3568.inc' if 'rk3568' in (d.getVar('MACHINEOVERRIDES') or '').split(':') else '' }
