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
	}
	else {
		// normal, collapse it
		mw->last_w = attr.width;
		mw->last_h = attr.height;
		XResizeWindow(display, mw->decorationWindow, attr.width, TITLEBAR_THICKNESS + 1);
	}
	
	XFetchName(display, mw->actualWindow, &title);
	drawDecorations(display, mw->decorationWindow, title);
}

static void claimWindow(Display *display, Window window, Window root, ManagedWindowPool *pool) {
	XSizeHints attr;
	long supplied_return = PPosition | PSize;
	
	XGetWMNormalHints(display, window, &attr, &supplied_return);
	
	GC gc = XCreateGC(display, root, 0, 0);	
	lowerAllWindowsInPool(display, pool, gc);
	XFlush(display);
	XFreeGC(display, gc);
	
	XMoveWindow(display, window, attr.x, attr.y);
	XResizeWindow(display, window, attr.width, attr.height);
	
//	logError("Trying to reparent %d at {%d, %d, %d, %d} with flags %d\n", window, attr.x, attr.y, attr.width, attr.height, attr.flags);
	
	Window deco = decorateWindow(display, window, root, attr.x, attr.y, attr.width, attr.height);
	
	XMoveWindow(display, deco, attr.x, attr.y);

	addWindowToPool(deco, window, pool);
	
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
	
	XSelectInput(display, root, SubstructureNotifyMask /* CreateNotify */ | FocusChangeMask | PropertyChangeMask /* ConfigureNotify */);

	XGrabButton(display, 1, AnyModifier, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);

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
					// This might be unnecessary
					//activateWindowInPool(mw, pool);
					
					// TODO: Pass the event into the subwindow

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
						printPool(pool);
					}
					XFlush(display);
					XFreeGC(display, gc);
				}
			} break;
			case MotionNotify: {
				int xdiff, ydiff;
				
				// Invalidate double clicks
				lastClick = 0;
				
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
				if (managedWindowForWindow(ev.xconfigure.window, pool)) {
					break;
				}
				if (!ev.xconfigure.window) {
					logError("Recieved invalid window for event \"%s\"\n", event_names[ev.type]);
				}
				claimWindow(display, ev.xconfigure.window, root, pool);
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
