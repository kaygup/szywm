#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_WINDOWS 50
#define MAX_WORKSPACES 10
#define BORDER_WIDTH 2
#define BORDER_COLOR 0xff0000 /* Red border */
#define FOCUS_BORDER_COLOR 0x00ff00 /* Green border for focused window */
#define TYPING_BORDER_COLOR 0x00FFFF /* Cyan border for focused window */
#define MAX(a, b) ((a) > (b) ? (a) : (b))

Display *dpy;
Window windows[MAX_WINDOWS];
int window_count = 0;
int workspace_count = 0;
int current_workspace = 0;  // Start with the first workspace
Window workspaces[MAX_WORKSPACES][MAX_WINDOWS];  // Store windows for each workspace
int workspace_window_count[MAX_WORKSPACES] = {0};  // Number of windows in each workspace
int resizing = 0;  // Flag to track if we are resizing
Window resizing_window = None; // The window we are resizing
int start_x, start_y, start_width, start_height; // Starting position and size for resizing
Window focused_window = None; // The window that has focus
Window mouse_over_window = None; // The window the mouse is currently over
Window root; // Root window for the background color
unsigned long background_color = 0x006400; // Dark green background by default

// Function prototypes
void tile_windows(void);
int is_firefox_window(Window win);
int is_polybar_window(Window win);  // Declare the function for Polybar window detection
void switch_workspace(int workspace_number);
void change_background(const char *hexcode);

// Helper function to check if a window is from Firefox (based on class or name)
int is_firefox_window(Window win) {
    char *window_name = NULL;
    XClassHint class_hint;
    if (XGetClassHint(dpy, win, &class_hint)) {
        if (strcmp(class_hint.res_class, "Firefox") == 0 || strcmp(class_hint.res_name, "Navigator") == 0) {
            XFree(class_hint.res_name);
            XFree(class_hint.res_class);
            return 1;
        }
    }
    return 0;
}

// Function to check if a window belongs to Polybar (based on class or name)
int is_polybar_window(Window win) {
    char *window_name = NULL;
    XClassHint class_hint;
    if (XGetClassHint(dpy, win, &class_hint)) {
        // Assuming Polybar's class name or window name is something recognizable like "Polybar"
        if (strcmp(class_hint.res_class, "Polybar") == 0 || strstr(class_hint.res_name, "polybar") != NULL) {
            XFree(class_hint.res_name);
            XFree(class_hint.res_class);
            return 1;  // It's a Polybar window
        }
        XFree(class_hint.res_name);
        XFree(class_hint.res_class);
    }
    return 0;  // Not a Polybar window
}

// Handle switching between workspaces
void switch_workspace(int workspace_number) {
    if (workspace_number < 0 || workspace_number >= MAX_WORKSPACES) return;

    current_workspace = workspace_number;
    // Clear the screen and tile windows for the new workspace
    tile_windows();
}

// Tile windows for the current workspace
void tile_windows() {
    if (workspace_window_count[current_workspace] == 0) return;

    int screen_width = DisplayWidth(dpy, DefaultScreen(dpy));
    int screen_height = DisplayHeight(dpy, DefaultScreen(dpy));

    // Divide the screen into columns and rows based on the number of windows
    int cols = (workspace_window_count[current_workspace] < 3) ? workspace_window_count[current_workspace] : (workspace_window_count[current_workspace] / 2 + (workspace_window_count[current_workspace] % 2));
    int rows = (workspace_window_count[current_workspace] + cols - 1) / cols;

    int win_width = screen_width / cols;
    int win_height = screen_height / rows;

    // Tile windows, adjusting the position and size
    for (int i = 0; i < workspace_window_count[current_workspace]; i++) {
        int x = (i % cols) * win_width;
        int y = (i / cols) * win_height;

        // Adjust window size for resizing window
        int final_width = win_width - BORDER_WIDTH * 2;
        int final_height = win_height - BORDER_WIDTH * 2;

        // Set border for Firefox windows (just like regular windows now)
        if (is_firefox_window(workspaces[current_workspace][i])) {
            XSetWindowBorder(dpy, workspaces[current_workspace][i], FOCUS_BORDER_COLOR);  // Green border for Firefox windows
        }
        // Set border for Polybar windows
        else if (is_polybar_window(workspaces[current_workspace][i])) {
            XMoveResizeWindow(dpy, workspaces[current_workspace][i], 0, 0, screen_width, BORDER_WIDTH);
            XSetWindowBorder(dpy, workspaces[current_workspace][i], BORDER_COLOR);  // Red border for Polybar
        }
        else {
            XSetWindowBorder(dpy, workspaces[current_workspace][i], BORDER_COLOR);  // Red border for regular windows
        }

        // Apply new size and position
        XMoveResizeWindow(dpy, workspaces[current_workspace][i], x, y, final_width, final_height);
        XSetWindowBorderWidth(dpy, workspaces[current_workspace][i], BORDER_WIDTH);
    }
}

// Change the background color
void change_background(const char *hexcode) {
    // Convert hex color string to an unsigned long
    unsigned long new_color = strtol(hexcode, NULL, 16);

    // Update the background color of the root window
    XSetWindowBackground(dpy, root, new_color);
    XClearWindow(dpy, root);  // Refresh the root window to apply the new background color
    background_color = new_color;  // Store the new background color
}

int main(void) {
    XWindowAttributes attr;
    XButtonEvent start;
    XEvent ev;

    if (!(dpy = XOpenDisplay(0x0))) {
        fprintf(stderr, "Failed to open display.\n");
        return 1;
    }

    root = DefaultRootWindow(dpy);
    XSelectInput(dpy, root, SubstructureNotifyMask | ButtonPressMask | KeyPressMask | PointerMotionMask | FocusChangeMask | EnterWindowMask);

    // Set the initial background color to dark green
    XSetWindowBackground(dpy, root, background_color);
    XClearWindow(dpy, root);  // Apply the background color immediately

    XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("F1")), Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("q")), Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("F")), Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("c")), Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("1")), Mod1Mask, root, True, GrabModeAsync, GrabModeAsync); // Alt + 1 for workspace 1
    XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("2")), Mod1Mask, root, True, GrabModeAsync, GrabModeAsync); // Alt + 2 for workspace 2
    XGrabButton(dpy, 1, Mod1Mask, root, True, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(dpy, 3, Mod1Mask, root, True, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);

    start.subwindow = None;
    for (;;) {
        XNextEvent(dpy, &ev);

        if (ev.type == KeyPress) {
            if (ev.xkey.keycode == XKeysymToKeycode(dpy, XStringToKeysym("Return"))) {
                char buf[256];
                int len = XLookupString(&ev.xkey, buf, sizeof(buf), NULL, NULL);

                if (len > 0 && strncmp(buf, "background", 10) == 0) {
                    char *hexcode = strchr(buf, '#');
                    if (hexcode != NULL) {
                        change_background(hexcode);
                    }
                }
            }

            if (ev.xkey.keycode == XKeysymToKeycode(dpy, XStringToKeysym("q"))) {
                if (fork() == 0) {
                    execlp("kitty", "kitty", (char *)NULL);
                    _exit(1);
                }
            } else if (ev.xkey.keycode == XKeysymToKeycode(dpy, XStringToKeysym("F"))) {
                if (fork() == 0) {
                    execlp("firefox", "firefox", (char *)NULL);
                    _exit(1);
                }
            } else if (ev.xkey.keycode == XKeysymToKeycode(dpy, XStringToKeysym("c"))) {
                Window win_under_cursor;
                int x, y;
                unsigned int mask;
                XQueryPointer(dpy, root, &win_under_cursor, &win_under_cursor, &x, &y, &x, &y, &mask);

                if (win_under_cursor != None) {
                    XDestroyWindow(dpy, win_under_cursor);
                }
            } else if (ev.xkey.keycode == XKeysymToKeycode(dpy, XStringToKeysym("1"))) {
                switch_workspace(0); // Switch to workspace 1
            } else if (ev.xkey.keycode == XKeysymToKeycode(dpy, XStringToKeysym("2"))) {
                switch_workspace(1); // Switch to workspace 2
            }
        } else if (ev.type == MapNotify) {
            if (window_count < MAX_WINDOWS) {
                windows[window_count++] = ev.xmap.window;
                workspaces[current_workspace][workspace_window_count[current_workspace]++] = ev.xmap.window;
                tile_windows();
            }
        } else if (ev.type == DestroyNotify) {
            for (int i = 0; i < workspace_window_count[current_workspace]; i++) {
                if (workspaces[current_workspace][i] == ev.xdestroywindow.window) {
                    for (int j = i; j < workspace_window_count[current_workspace] - 1; j++) {
                        workspaces[current_workspace][j] = workspaces[current_workspace][j + 1];
                    }
                    workspace_window_count[current_workspace]--;
                    tile_windows();
                    break;
                }
            }
        } else if (ev.type == ButtonPress) {
            if (ev.xbutton.button == Button1 && ev.xbutton.state & Mod1Mask) {
                // Focus the window under cursor when Alt + Left click
                Window win_under_cursor;
                int x, y;
                unsigned int mask;
                XQueryPointer(dpy, root, &win_under_cursor, &win_under_cursor, &x, &y, &x, &y, &mask);

                if (win_under_cursor != None) {
                    XSetInputFocus(dpy, win_under_cursor, RevertToPointerRoot, CurrentTime);
                    focused_window = win_under_cursor;  // Update the focused window
                }
            }
        }
    }
}
