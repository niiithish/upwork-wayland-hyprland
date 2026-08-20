/*
 * LD_PRELOAD hook so the Upwork tracker can capture the screen on Hyprland.
 *
 * Upwork's native module (uta_native.node) calls gdk_pixbuf_get_from_window().
 * On Wayland that path is disabled and the tracker uploads a grey "switch to
 * Xorg" placeholder. This library intercepts the GDK call and fills it with
 * a grim capture instead.
 *
 * Adapted from https://github.com/tiesselune/upwork-wayland (GPL-3.0)
 * for Hyprland: grim instead of flameshot, hyprctl instead of swaymsg.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dlfcn.h>
#include <errno.h>

#include <gdk-pixbuf-2.0/gdk-pixbuf/gdk-pixbuf.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/scrnsaver.h>

#define TEMPFILE "/tmp/upwork-screenshot.png"
#define LOGFILE "/tmp/upwork-wayland.log"

static void logmsg(const char *fmt, ...) {
    FILE *fp = fopen(LOGFILE, "a");
    if (!fp) return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(fp, fmt, ap);
    va_end(ap);
    fputc('\n', fp);
    fclose(fp);
}

static int take_screenshot(void) {
    pid_t pid = fork();
    if (pid < 0) {
        logmsg("fork failed: %s", strerror(errno));
        return -1;
    }

    if (pid == 0) {
        const char *wld = getenv("WAYLAND_DISPLAY_REAL");
        const char *command = getenv("UPWORK_SCREENSHOT_COMMAND");
        if (!command || !*command) command = "grim -t png";

        if (wld && *wld) {
            setenv("WAYLAND_DISPLAY", wld, 1);
        } else {
            logmsg("WARNING: WAYLAND_DISPLAY_REAL is unset; grim may fail");
        }

        size_t command_len = strlen(command);
        size_t path_len = strlen(TEMPFILE);
        char *complete = malloc(command_len + path_len + 2);
        if (!complete) _exit(127);
        memcpy(complete, command, command_len);
        complete[command_len] = ' ';
        memcpy(complete + command_len + 1, TEMPFILE, path_len + 1);

        logmsg("running: %s", complete);
        execl("/bin/sh", "sh", "-c", complete, (char *)NULL);
        logmsg("exec failed: %s", strerror(errno));
        _exit(127);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) return -1;
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return -1;
}

GdkPixbuf *gdk_pixbuf_get_from_window(void *window, gint src_x, gint src_y, gint width, gint height) {
    (void)window;
    (void)src_x;
    (void)src_y;
    (void)width;
    (void)height;

    logmsg("gdk_pixbuf_get_from_window requested %dx%d", width, height);
    int res = take_screenshot();
    if (res != 0) {
        logmsg("screenshot command failed: %d", res);
        return NULL;
    }

    GError *err = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(TEMPFILE, &err);
    unlink(TEMPFILE);
    if (err || !pixbuf) {
        logmsg("pixbuf load failed: %s", err ? err->message : "unknown");
        if (err) g_error_free(err);
        return NULL;
    }
    logmsg("screenshot pixbuf %dx%d", gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf));
    return pixbuf;
}

static int wanna_break_dimensions = 0;

gboolean (*real_gdk_pixbuf_save_to_callback)(GdkPixbuf *, GdkPixbufSaveFunc, gpointer, const char *, GError **, ...);

gboolean gdk_pixbuf_save_to_callback(GdkPixbuf *pb, GdkPixbufSaveFunc fn, gpointer dat, const char *typ, GError **err, ...) {
    if (!real_gdk_pixbuf_save_to_callback)
        real_gdk_pixbuf_save_to_callback = dlsym(RTLD_NEXT, "gdk_pixbuf_save_to_callback");

    wanna_break_dimensions = 1;
    return real_gdk_pixbuf_save_to_callback(pb, fn, dat, typ, err, NULL);
}

Status (*real_XGetWindowAttributes)(Display *display, Window w, XWindowAttributes *attrs);

Status XGetWindowAttributes(Display *display, Window w, XWindowAttributes *attrs) {
    if (!real_XGetWindowAttributes)
        real_XGetWindowAttributes = dlsym(RTLD_NEXT, "XGetWindowAttributes");

    Status res = real_XGetWindowAttributes(display, w, attrs);
    if (!res) {
        attrs->x = 0;
        attrs->y = 0;
        attrs->width = 622;
        attrs->height = 450;
        return 1;
    }

    /* Spoiling root size stops Electron from replacing the native snapshot. */
    if (wanna_break_dimensions) {
        wanna_break_dimensions = 0;
        attrs->width = 0;
        attrs->height = 0;
    }
    return res;
}

static int hypr_active_field(const char *field, char *buf, int bufsize) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "hyprctl activewindow -j 2>/dev/null | jq -r '.%s // empty'", field);
    FILE *fp = popen(cmd, "r");
    if (!fp) return 0;
    if (!fgets(buf, bufsize, fp)) {
        pclose(fp);
        return 0;
    }
    pclose(fp);
    size_t n = strlen(buf);
    if (n && buf[n - 1] == '\n') buf[--n] = '\0';
    return (int)n;
}

static int get_active_window_name(char *buf, int bufsize) {
    return hypr_active_field("title", buf, bufsize);
}

static int get_active_window_pid(void) {
    char buf[128];
    if (!hypr_active_field("pid", buf, sizeof(buf))) return 0;
    return atoi(buf);
}

int (*real_XGetWindowProperty)(Display *display, Window w, Atom property, long offset, long length, Bool delete, Atom req_type, Atom *actual_type, int *actual_fmt, unsigned long *nitems, unsigned long *bytes_after, unsigned char **prop);

int XGetWindowProperty(Display *display, Window w, Atom property, long offset, long length, Bool delete, Atom req_type, Atom *actual_type, int *actual_fmt, unsigned long *nitems, unsigned long *bytes_after, unsigned char **prop) {
    if (!real_XGetWindowProperty)
        real_XGetWindowProperty = dlsym(RTLD_NEXT, "XGetWindowProperty");

    char *propname = XGetAtomName(display, property);
    if (!propname)
        return real_XGetWindowProperty(display, w, property, offset, length, delete, req_type, actual_type, actual_fmt, nitems, bytes_after, prop);

    if (strcmp(propname, "_NET_WM_PID") == 0) {
        XFree(propname);
        *actual_type = XA_CARDINAL;
        *actual_fmt = 32;
        *nitems = 1;
        *bytes_after = 0;
        int *val = malloc(sizeof(int));
        *val = get_active_window_pid();
        *prop = (unsigned char *)val;
        return Success;
    }

    if (!strstr(propname, "NAME")) {
        XFree(propname);
        return real_XGetWindowProperty(display, w, property, offset, length, delete, req_type, actual_type, actual_fmt, nitems, bytes_after, prop);
    }
    XFree(propname);

    *actual_type = XA_STRING;
    *actual_fmt = 8;
    *bytes_after = 0;

    char buf[2048];
    int len = get_active_window_name(buf, sizeof(buf));
    if (!len) {
        *nitems = 0;
        *prop = NULL;
        return Success;
    }
    *nitems = (unsigned long)(len + 1);
    *prop = malloc((size_t)len + 1);
    memcpy(*prop, buf, (size_t)len + 1);
    return Success;
}

Bool (*real_XScreenSaverQueryExtension)(Display *dpy, int *event_base_return, int *error_base_return);

Bool XScreenSaverQueryExtension(Display *dpy, int *event_base_return, int *error_base_return) {
    if (!real_XScreenSaverQueryExtension)
        real_XScreenSaverQueryExtension = dlsym(RTLD_NEXT, "XScreenSaverQueryExtension");
    return real_XScreenSaverQueryExtension(dpy, event_base_return, error_base_return);
}

#ifdef TEST
int main(void) {
    GdkPixbuf *res = gdk_pixbuf_get_from_window(NULL, 0, 0, 1024, 1024);
    printf("res: %p\n", res);
    if (!res) return 1;
    GError *err = NULL;
    gboolean success = gdk_pixbuf_save(res, "/tmp/upwork-wayland-test.jpg", "jpeg", &err, NULL);
    printf("Pixbuf save: %d %s\n", success, err ? err->message : "");
    return success ? 0 : 1;
}
#endif
