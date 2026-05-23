#include "hotkey_x11.h"

#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <chrono>
#include <iostream>
#include <thread>

namespace whisper_dictate {

int run_hotkey_loop(const std::string& keysym_name) {
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

    int keycode = XKeysymToKeycode(display, keysym);
    if (keycode == 0) {
        std::cerr << "error: keysym has no keycode on this layout\n";
        XCloseDisplay(display);
        return 1;
    }

    Window root = DefaultRootWindow(display);
    int grab_result = XGrabKey(display, keycode, AnyModifier, root, True, GrabModeAsync, GrabModeAsync);
    XSync(display, False);
    if (grab_result != GrabSuccess) {
        std::cerr << "error: failed to grab hotkey; another app may own this binding\n";
        XCloseDisplay(display);
        return 1;
    }

    XSelectInput(display, root, KeyPressMask | KeyReleaseMask);
    std::cout << "hotkey grabbed: " << keysym_name << " (keycode " << keycode << ")\n";
    std::cout << "waiting for hotkey press/release... (Ctrl+C to quit)\n";

    while (true) {
        XEvent event;
        XNextEvent(display, &event);
        if ((event.type != KeyPress && event.type != KeyRelease) || event.xkey.keycode != keycode) {
            continue;
        }

        if (event.type == KeyRelease) {
            if (XEventsQueued(display, QueuedAfterReading)) {
                XEvent next;
                XPeekEvent(display, &next);
                if (next.type == KeyPress && next.xkey.keycode == keycode && next.xkey.time == event.xkey.time) {
                    continue;
                }
            }
            std::cout << "released\n";
        } else {
            std::cout << "pressed\n";
        }
        std::cout.flush();
    }

    XUngrabKey(display, keycode, AnyModifier, root);
    XCloseDisplay(display);
    return 0;
}

}
