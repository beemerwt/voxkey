#include "hotkey_x11.h"

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <iostream>

namespace voxkey {
namespace {

bool g_grab_failed = false;

int x_error_handler(Display*, XErrorEvent* error) {
    if (error->error_code == BadAccess) {
        g_grab_failed = true;
    }
    return 0;
}

}  // namespace

int run_hotkey_loop(
    const std::string& keysym_name,
    const std::function<void()>& on_press,
    const std::function<void()>& on_release) {
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "error: failed to open X11 display\n";
        return 1;
    }

    KeySym keysym = XStringToKeysym(keysym_name.c_str());
    if (keysym == NoSymbol) {
        std::cerr << "error: unknown keysym '" << keysym_name << "'\n";
        XCloseDisplay(display);
        return 1;
    }

    const KeyCode keycode = XKeysymToKeycode(display, keysym);
    Window root = DefaultRootWindow(display);

    g_grab_failed = false;
    auto previous_handler = XSetErrorHandler(x_error_handler);
    XGrabKey(display, keycode, AnyModifier, root, True, GrabModeAsync, GrabModeAsync);
    XSync(display, False);
    XSetErrorHandler(previous_handler);

    if (g_grab_failed) {
        std::cerr << "error: failed to grab hotkey '" << keysym_name
                  << "'. Another application may already own this binding.\n";
        XCloseDisplay(display);
        return 1;
    }

    XSelectInput(display, root, KeyPressMask | KeyReleaseMask);
    XSync(display, False);

    std::cout << "hotkey grabbed: " << keysym_name << " (keycode " << keycode << ")\n";
    std::cout << "waiting for hotkey press/release... (Ctrl+C to quit)\n";

    bool pressed = false;
    while (true) {
        XEvent event;
        XNextEvent(display, &event);
        if ((event.type != KeyPress && event.type != KeyRelease) || event.xkey.keycode != keycode) {
            continue;
        }

        if (event.type == KeyPress) {
            if (!pressed) {
                pressed = true;
                std::cout << "pressed\n";
                on_press();
            }
        } else {
            if (XEventsQueued(display, QueuedAfterReading)) {
                XEvent next;
                XPeekEvent(display, &next);
                if (next.type == KeyPress && next.xkey.keycode == keycode && next.xkey.time == event.xkey.time) {
                    continue;
                }
            }
            if (pressed) {
                pressed = false;
                std::cout << "released\n";
                on_release();
            }
        }
    }
}

}  // namespace voxkey
