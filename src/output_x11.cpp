#include "output_x11.h"

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>

#include <iostream>
#include <stdexcept>
#include <unistd.h>

namespace voxkey {
namespace {

void set_clipboard_text(Display* display, const std::string& text) {
    Window w = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, 1, 1, 0, 0, 0);
    Atom clipboard = XInternAtom(display, "CLIPBOARD", False);
    Atom utf8 = XInternAtom(display, "UTF8_STRING", False);
    Atom targets = XInternAtom(display, "TARGETS", False);
    Atom xa_string = XA_STRING;

    XSetSelectionOwner(display, clipboard, w, CurrentTime);
    if (XGetSelectionOwner(display, clipboard) != w) {
        XDestroyWindow(display, w);
        throw std::runtime_error("failed to own CLIPBOARD selection");
    }

    // serve selection requests for a short window while paste is triggered
    for (int i = 0; i < 50; ++i) {
        while (XPending(display)) {
            XEvent ev;
            XNextEvent(display, &ev);
            if (ev.type != SelectionRequest) {
                continue;
            }
            XSelectionRequestEvent* req = &ev.xselectionrequest;
            XEvent res{};
            res.xselection.type = SelectionNotify;
            res.xselection.display = req->display;
            res.xselection.requestor = req->requestor;
            res.xselection.selection = req->selection;
            res.xselection.target = req->target;
            res.xselection.time = req->time;
            res.xselection.property = req->property;

            if (req->target == targets) {
                Atom supported[2] = {utf8, xa_string};
                XChangeProperty(display, req->requestor, req->property, XA_ATOM, 32, PropModeReplace,
                                reinterpret_cast<const unsigned char*>(supported), 2);
            } else if (req->target == utf8 || req->target == xa_string) {
                XChangeProperty(display, req->requestor, req->property, req->target, 8, PropModeReplace,
                                reinterpret_cast<const unsigned char*>(text.c_str()), text.size());
            } else {
                res.xselection.property = None;
            }
            XSendEvent(display, req->requestor, False, 0, &res);
            XFlush(display);
        }
        usleep(10000);
    }

    XDestroyWindow(display, w);
}

void send_ctrl_v(Display* display) {
    KeyCode ctrl = XKeysymToKeycode(display, XK_Control_L);
    KeyCode v = XKeysymToKeycode(display, XK_v);
    XTestFakeKeyEvent(display, ctrl, True, CurrentTime);
    XTestFakeKeyEvent(display, v, True, CurrentTime);
    XTestFakeKeyEvent(display, v, False, CurrentTime);
    XTestFakeKeyEvent(display, ctrl, False, CurrentTime);
    XFlush(display);
}

}  // namespace

void emit_output_text(const std::string& text, const std::string& mode) {
    if (mode == "stdout") {
        std::cout << text << "\n";
        return;
    }

    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        throw std::runtime_error("failed to open X11 display for output");
    }

    if (mode == "clipboard_only") {
        set_clipboard_text(display, text);
        std::cout << "clipboard updated\n";
    } else if (mode == "clipboard_paste") {
        set_clipboard_text(display, text);
        send_ctrl_v(display);
        std::cout << "paste complete\n";
    } else {
        XCloseDisplay(display);
        throw std::runtime_error("invalid output_mode: " + mode);
    }

    XCloseDisplay(display);
}

}  // namespace voxkey
