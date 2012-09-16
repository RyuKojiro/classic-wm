#include <stdio.h>
#include <stdlib.h> // getenv
#include <X11/Xlib.h>
#include <stdarg.h> // va_list
#include <string.h> // logError

#define LOG_PREFIX	"kiosk-wm: "
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#include "window.h"
#include "eventnames.h"
#include "decorations.h"
#include "pool.h"

typedef enum {
	MouseDownStateUnknown = 0,
	MouseDownStateMove,
	MouseDownStateClose,
	MouseDownStateMaximize
} MouseDownState;

static void logError(const char *format, ... ) {
	size_t len = strlen(format) + strlen(LOG_PREFIX);
	char *buf = malloc(len);
	snprintf(buf, strlen(LOG_PREFIX) + 1, LOG_PREFIX);
	strncat(buf, format, len);
	
	va_list in, out;
	va_start(in, format);
	va_copy(out, in);
	vfprintf(stderr, buf, out);
	va_end(in);
	free(buf);
}

static int dealWithIt(Display *display, XErrorEvent *ev) {
	logError("X Error %d\n", ev->error_code);
	return 0;
}

int main (int argc, const char * argv[]) {
    Display *display;
	XEvent ev;
	int screen;
	XWindowAttributes attr;
    XButtonEvent start;
	MouseDownState downState;
	
	ManagedWindowPool *pool = createPool();
	
	/* Set up */
	XSetErrorHandler(dealWithIt);

	display = XOpenDisplay(getenv("DISPLAY"));	
	if (!display) {
		logError("Failed to open display, is X running?\n");
		exit(1);
	}

	screen = DefaultScreen(display);

	/* Find the window */
	Window root = RootWindow(display, screen);
	Window parent;
	Window *children;
	unsigned int nchildren;
		
	XQueryTree(display, root, &root, &parent, &children, &nchildren);
	
	/* Initial Traverse on Startup */
	unsigned int i;
	for (i = 0; i < nchildren; i++) {
		if (children[i] && children[i] != root) {
			repositionWindow(display, screen, children[i], 0, 0, DisplayWidth(display, screen), DisplayHeight(display, screen));	
		}
		else {
			logError("Could not find window with XID:%ld\n", children[i]);
		}
	}
		
	XFree(children);
	
	XSelectInput(display, root, SubstructureNotifyMask /* CreateNotify */ | FocusChangeMask | PropertyChangeMask /* ConfigureNotify */);

	XGrabButton(display, 1, AnyModifier, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);

    for(;;)
    {
        XNextEvent(display, &ev);
				
		switch (ev.type) {
			case ButtonPress: {
				if (ev.xkey.subwindow != None) {
					ManagedWindow *mw = managedWindowForWindow(ev.xkey.subwindow, pool);
					XGetWindowAttributes(display, mw->decorationWindow, &attr);
					XRaiseWindow(display, mw->decorationWindow);

					// Check what was downed
					int x, y;
					x = ev.xbutton.x_root - attr.x;
					y = ev.xbutton.y_root - attr.y;
					downState = MouseDownStateUnknown;
					GC gc = XCreateGC(display, mw->decorationWindow, 0, 0);	
					if (pointIsInRect(x, y, RECT_TITLEBAR)) {
						downState = MouseDownStateMove;
					}
					if (pointIsInRect(x, y, RECT_CLOSE_BTN)) {
						drawCloseButtonDown(display, mw->decorationWindow, gc, RECT_CLOSE_BTN);
						downState = MouseDownStateClose;
					}
					if (pointIsInRect(x, y, RECT_MAX_BTN)) {
						drawCloseButtonDown(display, mw->decorationWindow, gc, RECT_MAX_BTN);
						downState = MouseDownStateMaximize;
					}					
					XFlush(display);
					XFreeGC(display, gc);

					// Grab the pointer
					XGrabPointer(display, ev.xbutton.subwindow, True,
								 PointerMotionMask|ButtonReleaseMask, GrabModeAsync,
								 GrabModeAsync, None, None, CurrentTime);
					start = ev.xbutton;
				}
			} break;
			case MotionNotify: {
				int xdiff, ydiff;
				while(XCheckTypedEvent(display, MotionNotify, &ev));
				GC gc = XCreateGC(display, ev.xmotion.window, 0, 0);	
				switch (downState) {
					case MouseDownStateMove: {
						xdiff = ev.xbutton.x_root - start.x_root;
						ydiff = ev.xbutton.y_root - start.y_root;
						XMoveResizeWindow(display, ev.xmotion.window,
										  attr.x + (start.button==1 ? xdiff : 0),
										  attr.y + (start.button==1 ? ydiff : 0),
										  MAX(1, attr.width + (start.button==3 ? xdiff : 0)),
										  MAX(1, attr.height + (start.button==3 ? ydiff : 0)));
					} break;
					case MouseDownStateClose: {
						int x, y;
						x = ev.xbutton.x_root - attr.x;
						y = ev.xbutton.y_root - attr.y;
						if (pointIsInRect(x, y, RECT_CLOSE_BTN)) {
							drawCloseButtonDown(display, ev.xmotion.window, gc, RECT_CLOSE_BTN);							
						}
						else {
							drawCloseButton(display, ev.xmotion.window, gc, RECT_CLOSE_BTN);							
						}
					} break;
					case MouseDownStateMaximize: {
						int x, y;
						x = ev.xbutton.x_root - attr.x;
						y = ev.xbutton.y_root - attr.y;
						if (pointIsInRect(x, y, RECT_MAX_BTN)) {
							drawCloseButtonDown(display, ev.xmotion.window, gc, RECT_MAX_BTN);							
						}
						else {
							drawMaximizeButton(display, ev.xmotion.window, gc, RECT_MAX_BTN);
						}
					} break;
					default:
						break;
				}
				XFlush(display);
				XFreeGC(display, gc);
			} break;
			case ButtonRelease: {
				XUngrabPointer(display, CurrentTime);
				
				GC gc = XCreateGC(display, ev.xmotion.window, 0, 0);	
				switch (downState) {
					case MouseDownStateClose: {
						drawCloseButton(display, ev.xmotion.window, gc, RECT_CLOSE_BTN);
					} break;
					case MouseDownStateMaximize: {
						drawMaximizeButton(display, ev.xmotion.window, gc, RECT_MAX_BTN);
					} break;
					default:
						break;
				}
				XFlush(display);
				XFreeGC(display, gc);
			} break;
			case ConfigureNotify: {
				if (ev.xconfigure.override_redirect) {
					break;
				}
				if (!ev.xconfigure.window) {
					logError("Recieved invalid window for event \"%s\"\n", event_names[ev.type]);
				}
				resizeWindow(display, screen, ev.xconfigure.window,
							 483, 315);
				repositionWindow(display, screen, ev.xconfigure.window,
								 500, 500, 0, 0);
				Window deco = decorateWindow(display, ev.xconfigure.window, root, ev.xconfigure.x, ev.xconfigure.y, ev.xconfigure.width, ev.xconfigure.height);
				
				addWindowToPool(deco, ev.xconfigure.window, pool);
			} break;
			case Expose: {
				if (managedWindowForWindow(ev.xexpose.window, pool)) {
					drawDecorations(display, ev.xexpose.window, "expose_draw");
				}
			} break;
			default: {
				logError("Recieved unhandled event \"%s\"\n", event_names[ev.type]);
			} break;
		}
	}
	
	XCloseDisplay(display);
	freePool(pool);
	
	return 0;
}
