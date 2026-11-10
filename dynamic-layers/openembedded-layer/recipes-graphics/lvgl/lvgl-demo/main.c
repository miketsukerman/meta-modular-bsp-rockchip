// SPDX-License-Identifier: MIT
/*
 * lvgl-demo: LVGL widgets demo, either as a Wayland client (a window of
 * Weston) or full-screen on a DRM/KMS output with evdev input.
 *
 * Backend selection (LV_BACKEND=wayland|drm forces one):
 *   - Wayland, if LVGL has it and a compositor socket is found: the
 *     WAYLAND_DISPLAY environment variable, Weston's system socket
 *     /run/wayland-0 (core-image-weston) or $XDG_RUNTIME_DIR/wayland-0;
 *   - DRM/KMS otherwise. This needs the DRM device for itself, so it does
 *     not work while Weston or another display server is running.
 *
 * Wayland environment:
 *   LV_WAYLAND_FULLSCREEN  "0" for a window instead of full screen
 *   LV_WAYLAND_SIZE        window size, default 800x480
 * DRM/KMS environment:
 *   LV_DRM_DEVICE     DRM card to use (default: first card with a
 *                     connector, from lv_linux_drm_find_device_path())
 *   LV_DRM_CONNECTOR  DRM connector id (default: first connected one);
 *                     list ids with "modetest -M rockchip -c"
 *   LV_EVDEV_DEVICE   evdev input device for the pointer/touchscreen
 *                     (default: /dev/input/event0), "none" to disable
 */

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "lvgl.h"
#include "demos/lv_demos.h"

#if !LV_USE_LINUX_DRM && !LV_USE_WAYLAND
#error "lvgl must be built with LV_USE_LINUX_DRM and/or LV_USE_WAYLAND"
#endif
#if LV_USE_LINUX_DRM && !LV_USE_EVDEV
#error "lvgl must be built with LV_USE_EVDEV for the DRM backend"
#endif
#if !LV_USE_DEMO_WIDGETS
#error "lvgl must be built with LV_BUILD_DEMOS and LV_USE_DEMO_WIDGETS"
#endif

#define MAX_IDLE_MS 50

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

static uint32_t clamp_idle(uint32_t idle_ms)
{
    return (idle_ms == LV_NO_TIMER_READY || idle_ms > MAX_IDLE_MS) ? MAX_IDLE_MS : idle_ms;
}

#if LV_USE_WAYLAND
static bool wayland_socket_found(void)
{
    const char *display = getenv("WAYLAND_DISPLAY");
    const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
    char path[512];

    if (display && *display)
        return true;
    /* Weston started by weston-init (core-image-weston) */
    if (access("/run/wayland-0", F_OK) == 0) {
        setenv("WAYLAND_DISPLAY", "/run/wayland-0", 1);
        return true;
    }
    if (runtime_dir && *runtime_dir) {
        snprintf(path, sizeof(path), "%s/wayland-0", runtime_dir);
        if (access(path, F_OK) == 0) {
            setenv("WAYLAND_DISPLAY", "wayland-0", 1);
            return true;
        }
    }
    return false;
}

static bool on_window_close(lv_display_t *disp)
{
    (void)disp;
    running = 0;
    return true;
}

static int run_wayland(void)
{
    const char *env_fs = getenv("LV_WAYLAND_FULLSCREEN");
    const char *env_size = getenv("LV_WAYLAND_SIZE");
    unsigned int width = 800, height = 480;
    struct pollfd pfd;
    lv_display_t *disp;

    if (env_size && sscanf(env_size, "%ux%u", &width, &height) != 2) {
        fprintf(stderr, "lvgl-demo: invalid LV_WAYLAND_SIZE '%s'\n", env_size);
        return EXIT_FAILURE;
    }

    disp = lv_wayland_window_create(width, height, "LVGL demo", on_window_close);
    if (!disp) {
        fprintf(stderr, "lvgl-demo: cannot connect to the Wayland compositor (%s)\n",
                getenv("WAYLAND_DISPLAY"));
        return EXIT_FAILURE;
    }
    if (!env_fs || strcmp(env_fs, "0") != 0)
        lv_wayland_window_set_fullscreen(disp, true);

    printf("lvgl-demo: Wayland backend on %s\n", getenv("WAYLAND_DISPLAY"));
    lv_demo_widgets();

    pfd.fd = lv_wayland_get_fd();
    pfd.events = POLLIN;
    while (running) {
        /* also handles Wayland input/output; tears down on window close */
        uint32_t idle_ms = lv_wayland_timer_handler();

        if (!running)
            break;
        poll(&pfd, 1, (int)clamp_idle(idle_ms));
    }

    return EXIT_SUCCESS;
}
#endif /* LV_USE_WAYLAND */

#if LV_USE_LINUX_DRM
static int run_drm(void)
{
    const char *env_card = getenv("LV_DRM_DEVICE");
    const char *env_conn = getenv("LV_DRM_CONNECTOR");
    const char *env_evdev = getenv("LV_EVDEV_DEVICE");
    char *found_card = NULL;
    const char *card;
    int64_t connector = -1;
    lv_display_t *disp;

    if (env_card && *env_card) {
        card = env_card;
    } else {
        found_card = lv_linux_drm_find_device_path();
        if (!found_card) {
            fprintf(stderr, "lvgl-demo: no DRM card with a connector found\n");
            return EXIT_FAILURE;
        }
        card = found_card;
    }

    if (env_conn && *env_conn) {
        char *end;

        errno = 0;
        connector = strtoll(env_conn, &end, 10);
        if (errno || *end || connector < 0) {
            fprintf(stderr, "lvgl-demo: invalid LV_DRM_CONNECTOR '%s'\n", env_conn);
            return EXIT_FAILURE;
        }
    }

    disp = lv_linux_drm_create();
    if (!disp) {
        fprintf(stderr, "lvgl-demo: lv_linux_drm_create() failed\n");
        return EXIT_FAILURE;
    }

    printf("lvgl-demo: DRM/KMS backend on %s, connector %s\n", card,
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
            fprintf(stderr, "lvgl-demo: cannot open %s, running without input\n",
                    env_evdev);
    }

    lv_demo_widgets();

    while (running)
        usleep(clamp_idle(lv_timer_handler()) * 1000);

    lv_display_delete(disp);
    return EXIT_SUCCESS;
}
#endif /* LV_USE_LINUX_DRM */

int main(void)
{
    const char *backend = getenv("LV_BACKEND");
    bool use_wayland = false;
    int ret;

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    if (backend && *backend) {
        if (strcmp(backend, "wayland") == 0) {
            use_wayland = true;
        } else if (strcmp(backend, "drm") != 0) {
            fprintf(stderr, "lvgl-demo: LV_BACKEND must be \"wayland\" or \"drm\"\n");
            return EXIT_FAILURE;
        }
    }
#if LV_USE_WAYLAND
    else {
        use_wayland = wayland_socket_found();
    }
#endif

    lv_init();
    lv_tick_set_cb(tick_get_cb);

    if (use_wayland) {
#if LV_USE_WAYLAND
        ret = run_wayland();
#else
        fprintf(stderr, "lvgl-demo: lvgl was built without Wayland support\n");
        ret = EXIT_FAILURE;
#endif
    } else {
#if LV_USE_LINUX_DRM
        ret = run_drm();
#else
        fprintf(stderr, "lvgl-demo: lvgl was built without DRM support\n");
        ret = EXIT_FAILURE;
#endif
    }

    if (!use_wayland)
        lv_deinit();
    return ret;
}
