/* SPDX-License-Identifier: MIT */
/*
 * LVGL assert handler for Linux, selected with
 * CONFIG_LV_ASSERT_HANDLER_INCLUDE="lv_assert_abort.h" (lvgl-rk3568.cfg).
 *
 * LVGL's default handler is "while(1);", which leaves a process spinning
 * forever on a failed assertion, e.g. when no Wayland compositor answers.
 * LV_ASSERT*() has already logged the error when the handler runs; abort()
 * instead, so the failure is visible and systemd can restart the service.
 */
#include <stdlib.h>

#undef LV_ASSERT_HANDLER
#define LV_ASSERT_HANDLER abort();
