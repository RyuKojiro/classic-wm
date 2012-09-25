#include <stdio.h>
#include <stdlib.h> // getenv
#include <X11/Xlib.h>
#include <X11/Xutil.h> // XSizeHints
#include <stdarg.h> // va_list
#include <string.h> // logError
#include <time.h> // time()

#define LOG_PREFIX	"classic-wm: "
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#include "eventnames.h"
#include "decorations.h"
#include "pool.h"

typedef enum {
	MouseDownStateUnknown = 0,
	MouseDownStateMove,
	MouseDownStateClose,
	MouseDownStateMaximize,
	MouseDownStateResize
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

static void resizeWindow(Display *display, ManagedWindow *mw, int w, int h) {
	XResizeWindow(display, mw->decorationWindow, w, h);
	XResizeWindow(display, mw->actualWindow, w, h - TITLEBAR_THICKNESS);
	XMoveWindow(display, mw->resizer, w - RESIZE_CONTROL_SIZE, h - RESIZE_CONTROL_SIZE);	
}

static void lowerAllWindowsInPool(Display *display, ManagedWindowPool *pool, GC gc) {
	XWindowAttributes attr;
	char *title;
	ManagedWindow *this = pool->head;
	if (!this) {
		// No windows to lower
		return;
	}
	do {
		XFetchName(display, this->actualWindow, &title);
		XGetWindowAttributes(display, this->decorationWindow, &attr);
		whiteOutTitleBar(display, this->decorationWindow, gc, attr);
		drawTitle(display, this->decorationWindow, gc, title, attr);
	} while ((this = this->next));
}

static void collapseWindow(Display *display, ManagedWindow *mw) {
	XWindowAttributes attr;
	char *title;
	XGetWindowAttributes(display, mw->decorationWindow, &attr);
	if (attr.height == (TITLEBAR_THICKNESS + 1)) {
		// collapsed, uncollapse it
		XResizeWindow(display, mw->decorationWindow, mw->last_w, mw->last_h);
		XMapWindow(display, mw->actualWindow);
	}
	else {
		// normal, collapse it
		mw->last_w = attr.width;
		mw->last_h = attr.height;
		XResizeWindow(display, mw->decorationWindow, attr.width, TITLEBAR_THICKNESS + 1);
		XUnmapWindow(display, mw->actualWindow);
	}
	
	XFetchName(display, mw->actualWindow, &title);
	drawDecorations(display, mw->decorationWindow, title);
}

static void claimWindow(Display *display, Window window, Window root, ManagedWindowPool *pool) {
	XSizeHints attr;
	long supplied_return = PPosition | PSize;
	Window resizer;
	
	XGetWMNormalHints(display, window, &attr, &supplied_return);
	
	GC gc = XCreateGC(display, root, 0, 0);	
	lowerAllWindowsInPool(display, pool, gc);
	XFlush(display);
	XFreeGC(display, gc);
	
	XMoveWindow(display, window, attr.x, attr.y);
	XResizeWindow(display, window, attr.width, attr.height);
	
//	logError("Trying to reparent %d at {%d, %d, %d, %d} with flags %d\n", window, attr.x, attr.y, attr.width, attr.height, attr.flags);
	
	Window deco = decorateWindow(display, window, root, attr.x, attr.y, attr.width, attr.height, &resizer);
	XUngrabButton(display, 1, AnyModifier, window);
	XMoveWindow(display, deco, attr.x, attr.y);

	addWindowToPool(deco, window, resizer, pool);
	
	XRaiseWindow(display, deco);
}

static void unclaimWindow(Display *display, Window window, ManagedWindowPool *pool) {
	ManagedWindow *mw = managedWindowForWindow(window, pool);
	if (mw) {
		XUnmapWindow(display, mw->decorationWindow);
		XDestroyWindow(display, mw->decorationWindow);
		removeWindowFromPool(mw, pool);
	}
}

int main (int argc, const char * argv[]) {
    Display *display;
	XEvent ev;
	int screen;
	XWindowAttributes attr;
    XButtonEvent start;
	MouseDownState downState;
	time_t lastClick = 0;
	
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
			//reparentWindow(display, ev.xconfigure.window, root, pool);
		}
		else {
			logError("Could not find window with XID:%ld\n", children[i]);
		}
	}
		
	XFree(children);
	
	XSelectInput(display, root, SubstructureNotifyMask /* CreateNotify */ | FocusChangeMask | PropertyChangeMask /* ConfigureNotify */ | ButtonPressMask);

    for(;;)
    {
        XNextEvent(display, &ev);
				
		switch (ev.type) {
			case ButtonPress: {
				if (ev.xkey.subwindow != None) {
					ManagedWindow *mw = managedWindowForWindow(ev.xkey.subwindow, pool);
					if (!mw) {
						break;
					}
					GC gc = XCreateGC(display, mw->decorationWindow, 0, 0);	

					// Raise and activate the window
					char *title;
					lowerAllWindowsInPool(display, pool, gc);
					XRaiseWindow(display, mw->decorationWindow);
					XGetWindowAttributes(display, mw->decorationWindow, &attr);
					XFetchName(display, mw->actualWindow, &title);
					drawDecorations(display, mw->decorationWindow, title);

					// Check what was downed
					int x, y;
					x = ev.xbutton.x_root - attr.x;
					y = ev.xbutton.y_root - attr.y;
					downState = MouseDownStateUnknown;
					if (pointIsInRect(x, y, RECT_TITLEBAR)) {
						downState = MouseDownStateMove;
						// Grab the pointer
						XGrabPointer(display, ev.xbutton.subwindow, True,
									 PointerMotionMask|ButtonReleaseMask, GrabModeAsync,
									 GrabModeAsync, None, None, CurrentTime);
						start = ev.xbutton;
						
						if (lastClick >= (time(NULL) - 1)) {
							collapseWindow(display, mw);
							lastClick = 0;
						}
						else {
							time(&lastClick);
						}
					}
					if (pointIsInRect(x, y, RECT_CLOSE_BTN)) {
					 	drawCloseButtonDown(display, mw->decorationWindow, gc, RECT_CLOSE_BTN);
						downState = MouseDownStateClose;
					}
					if (pointIsInRect(x, y, RECT_MAX_BTN)) {
						drawCloseButtonDown(display, mw->decorationWindow, gc, RECT_MAX_BTN);
						downState = MouseDownStateMaximize;
						lastClick = 0;
						printPool(pool);
					}
					if (ev.xbutton.subwindow == mw->resizer || pointIsInRect(x, y, RECT_RESIZE_BTN)) {
						// Grab the pointer
						XGrabPointer(display, ev.xbutton.subwindow, True,
									 PointerMotionMask|ButtonReleaseMask, GrabModeAsync,
									 GrabModeAsync, None, None, CurrentTime);
						start = ev.xbutton;
						lastClick = 0;
						downState = MouseDownStateResize;
					}
					XFlush(display);
					XFreeGC(display, gc);
				}
			} break;
			case MotionNotify: {
				int x, y;
				// Invalidate double clicks
				lastClick = 0;
				
				while(XCheckTypedEvent(display, MotionNotify, &ev));
				GC gc = XCreateGC(display, ev.xmotion.window, 0, 0);	
				switch (downState) {
					case MouseDownStateResize: {
						x = ev.xbutton.x_root - start.x_root;
						y = ev.xbutton.y_root - start.y_root;
						ManagedWindow *mw = managedWindowForWindow(start.subwindow, pool);
						XGetWindowAttributes(display, mw->decorationWindow, &attr);
						resizeWindow(display, mw, attr.width + x, attr.height + y);
						start.x_root = ev.xbutton.x_root;
						start.y_root = ev.xbutton.y_root;						
					} break;
					case MouseDownStateMove: {
						x = ev.xbutton.x_root - start.x_root;
						y = ev.xbutton.y_root - start.y_root;
						XMoveWindow(display, ev.xmotion.window, attr.x + x, attr.y + y);
					} break;
					case MouseDownStateClose: {
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
						
						int x, y;
						x = ev.xbutton.x_root - attr.x;
						y = ev.xbutton.y_root - attr.y;
						if (pointIsInRect(x, y, RECT_CLOSE_BTN)) {
							unclaimWindow(display, ev.xmotion.window, pool);
						}
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
			case MapNotify: {
				if (managedWindowForWindow(ev.xmap.window, pool) || ev.xmap.override_redirect) {
					break;
				}
				if (!ev.xmap.window) {
					logError("Recieved invalid window for event \"%s\"\n", event_names[ev.type]);
				}
				claimWindow(display, ev.xmap.window, root, pool);
			} break;
			case DestroyNotify: {
				unclaimWindow(display, ev.xdestroywindow.window, pool);
			} break;
			default: {
				logError("Recieved unhandled event \"%s\"\n", event_names[ev.type]);
			} break;
		}
	}
	
	XCloseDisplay(display);
	destroyPool(pool);
	
	return 0;
}
