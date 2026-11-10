// SPDX-License-Identifier: MIT
/*
 * lvgl-demo-drm: LVGL widgets demo on a Linux DRM/KMS display with evdev
 * input.
 *
 * Environment:
 *   LV_DRM_DEVICE     DRM card to use (default: first card with a
 *                     connector, from lv_linux_drm_find_device_path())
 *   LV_DRM_CONNECTOR  DRM connector id (default: first connected one);
 *                     list ids with "modetest -M rockchip -c"
 *   LV_EVDEV_DEVICE   evdev input device for the pointer/touchscreen
 *                     (default: /dev/input/event0), "none" to disable
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "lvgl.h"
#include "demos/lv_demos.h"

#if !LV_USE_LINUX_DRM
#error "lvgl must be built with LV_USE_LINUX_DRM"
#endif
#if !LV_USE_EVDEV
#error "lvgl must be built with LV_USE_EVDEV"
#endif
#if !LV_USE_DEMO_WIDGETS
#error "lvgl must be built with LV_BUILD_DEMOS and LV_USE_DEMO_WIDGETS"
#endif

static volatile sig_atomic_t running = 1;

static void on_signal(int sig)
{
    (void)sig;
    running = 0;
}

static uint32_t tick_get_cb(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

int main(void)
{
    const char *env_card = getenv("LV_DRM_DEVICE");
    const char *env_conn = getenv("LV_DRM_CONNECTOR");
    const char *env_evdev = getenv("LV_EVDEV_DEVICE");
    char *found_card = NULL;
    const char *card;
    int64_t connector = -1;
    lv_display_t *disp;

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    lv_init();
    lv_tick_set_cb(tick_get_cb);

    if (env_card && *env_card) {
        card = env_card;
    } else {
        found_card = lv_linux_drm_find_device_path();
        if (!found_card) {
            fprintf(stderr, "lvgl-demo-drm: no DRM card with a connector found\n");
            return EXIT_FAILURE;
        }
        card = found_card;
    }

    if (env_conn && *env_conn) {
        char *end;

        errno = 0;
        connector = strtoll(env_conn, &end, 10);
        if (errno || *end || connector < 0) {
            fprintf(stderr, "lvgl-demo-drm: invalid LV_DRM_CONNECTOR '%s'\n", env_conn);
            return EXIT_FAILURE;
        }
    }

    disp = lv_linux_drm_create();
    if (!disp) {
        fprintf(stderr, "lvgl-demo-drm: lv_linux_drm_create() failed\n");
        return EXIT_FAILURE;
    }

    printf("lvgl-demo-drm: using %s, connector %s\n", card,
           connector < 0 ? "auto" : env_conn);
    lv_linux_drm_set_file(disp, card, connector);
    if (found_card)
        lv_free(found_card);

    if (!env_evdev || !*env_evdev)
        env_evdev = "/dev/input/event0";
    if (strcmp(env_evdev, "none") != 0) {
        lv_indev_t *indev = lv_evdev_create(LV_INDEV_TYPE_POINTER, env_evdev);

        if (indev)
            lv_indev_set_display(indev, disp);
        else
            fprintf(stderr, "lvgl-demo-drm: cannot open %s, running without input\n",
                    env_evdev);
    }

    lv_demo_widgets();

    while (running) {
        uint32_t idle_ms = lv_timer_handler();

        if (idle_ms == LV_NO_TIMER_READY || idle_ms > 50)
            idle_ms = 50;
        usleep(idle_ms * 1000);
    }

    lv_display_delete(disp);
    lv_deinit();
    return EXIT_SUCCESS;
}
