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

	//XGrabButton(display, 1, Mod1Mask, root, True, ButtonPressMask, GrabModeAsync,
	//			GrabModeAsync, None, None);

    for(;;)
    {
        XNextEvent(display, &ev);
				
		switch (ev.type) {
			/*case ButtonPress: {
				if (ev.xkey.subwindow != None) {
					XRaiseWindow(display, ev.xkey.subwindow);
				}
			} break;*/
			case ConfigureNotify:
			case CreateNotify: {
				if (ev.xcreatewindow.override_redirect) {
					break;
				}
				if (!ev.xcreatewindow.window) {
					logError("Recieved invalid window for event \"%s\"\n", event_names[ev.type]);
				}
				resizeWindow(display, screen, ev.xcreatewindow.window,
							 500, 500);
				repositionWindow(display, screen, ev.xcreatewindow.window,
								 500, 500, 0, 0);
				Window deco = decorateWindow(display, ev.xcreatewindow.window, root, ev.xcreatewindow.x, ev.xcreatewindow.y, ev.xcreatewindow.width, ev.xcreatewindow.height);
				
				addWindowToPool(deco, ev.xcreatewindow.window, pool);
			} break;
			case Expose: {
				if (isMemberOfPool(ev.xexpose.window, pool)) {
					drawDecorations(display, ev.xexpose.window, "expose_draw");
				}
			}
			default: {
				logError("Recieved unhandled event \"%s\"\n", event_names[ev.type]);
			} break;
		}
	}
	
	XCloseDisplay(display);
	freePool(pool);
	
	return 0;
}
