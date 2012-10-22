#include <stdio.h>
#include <stdlib.h> // getenv
#include <X11/Xlib.h>
#include <X11/Xutil.h> // XSizeHints
#include <stdarg.h> // va_list
#include <string.h> // logError
#include <time.h> // time()

#define LOG_PREFIX	"classic-wm: "
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define NEW_WINDOW_OFFSET		22

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
	// Set some absolute minimums
	w = MAX(w, ((TITLEBAR_CONTROL_SIZE) * 5));
	h = MAX(h, ((TITLEBAR_THICKNESS) * 2) + RESIZE_CONTROL_SIZE);
	
	// Respect the window minimums, if they exist
	XSizeHints hints;
	long supplied_return;
	XGetWMNormalHints(display, mw->actualWindow, &hints, &supplied_return);
	if ((supplied_return & PMinSize) == PMinSize) {
		w = MAX(w, hints.min_width);
		h = MAX(h, hints.min_height);
	}
	
	XResizeWindow(display, mw->decorationWindow, w, h);
	XResizeWindow(display, mw->actualWindow, w - 3, h - TITLEBAR_THICKNESS - 2);
	XMoveWindow(display, mw->resizer, w - RESIZE_CONTROL_SIZE - 2, h - RESIZE_CONTROL_SIZE - 2);
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

		// Redraw Resizer
		GC gc = XCreateGC(display, mw->resizer, 0, 0);
		drawResizeButton(display, mw->resizer, gc, RECT_RESIZE_DRAW);
		XFlush(display);
		XFreeGC(display, gc);
		XRaiseWindow(display, mw->resizer);
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

static void maximizeWindow(Display *display, ManagedWindow *mw) {
	XSizeHints attr;
	XSizeHints container;
	long supplied_return;
	long supplied_return_container;
	char *title;
	
	XGetWMNormalHints(display, mw->actualWindow, &attr, &supplied_return);
	XGetWMNormalHints(display, mw->decorationWindow, &container, &supplied_return_container);
	
	int max_w = (supplied_return | PMaxSize && attr.max_width) ? attr.max_width : XDisplayWidth(display, DefaultScreen(display));
	int max_h = (supplied_return | PMaxSize && attr.max_height) ? attr.max_height : XDisplayHeight(display, DefaultScreen(display));
	
	if (mw->last_h || mw->last_w || mw->last_x || mw->last_y) {
		XMoveWindow(display, mw->decorationWindow, mw->last_x, mw->last_y);
		resizeWindow(display, mw, mw->last_w, mw->last_h);
		
		mw->last_h = 0;
		mw->last_w = 0;
		mw->last_x = 0;
		mw->last_y = 0;
	}
	else { // if we aren't at max size, and there is one, go to it
		mw->last_h = attr.height + TITLEBAR_THICKNESS + 2;
		mw->last_w = attr.width + 3;
		mw->last_x = container.x;
		mw->last_y = container.y;
		
		XMoveWindow(display, mw->decorationWindow, 0, NEW_WINDOW_OFFSET);
		resizeWindow(display, mw, max_w, max_h - NEW_WINDOW_OFFSET);
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
	
	//XMoveWindow(display, window, attr.x, attr.y);
	//XResizeWindow(display, window, attr.width, attr.height);
	
//	logError("Trying to reparent %d at {%d, %d, %d, %d} with flags %d\n", window, attr.x, attr.y, attr.width, attr.height, attr.flags);
	
	Window deco = decorateWindow(display, window, root, attr.x, attr.y, attr.width, attr.height, &resizer);
	XUngrabButton(display, 1, AnyModifier, window);
	XMoveWindow(display, deco, XDisplayWidth(display, DefaultScreen(display)) - attr.width - 3, NEW_WINDOW_OFFSET);

	XSelectInput(display, window, SubstructureNotifyMask);
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

static void cleanPool(Display *display, ManagedWindowPool *pool) {
	ManagedWindow *this = pool->head;
	ManagedWindow cache;
	XWindowAttributes attribs;
	while (this) {
		if (XGetWindowAttributes(display, this->actualWindow, &attribs) == Success) {
			cache.next = this->next;
			unclaimWindow(display, this->actualWindow, pool);
			this = &cache;
		}
		this = this->next;
	}
}

int main (int argc, const char * argv[]) {
    Display *display;
	XEvent ev;
	int screen;
	XWindowAttributes attr;
    XButtonEvent start;
	MouseDownState downState;
	time_t lastClickTime = 0;
	Window lastClickWindow = 0;
	int x, y;

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
	char *title;

	XQueryTree(display, root, &root, &parent, &children, &nchildren);
	
	/* Initial Traverse on Startup */
	unsigned int i;
	for (i = 0; i < nchildren; i++) {
		if (children[i] && children[i] != root) {
			claimWindow(display, children[i], root, pool);
		}
		else {
			logError("Could not find window with XID:%ld\n", children[i]);
		}
	}
		
	XFree(children);
	
	XSelectInput(display, root, StructureNotifyMask | SubstructureNotifyMask /* CreateNotify */ | ButtonPressMask);

    for(;;)
    {
        XNextEvent(display, &ev);
		GC gc = XCreateGC(display, ev.xany.window, 0, 0);
		
		switch (ev.type) {
	#pragma mark ButtonPress
			case ButtonPress: {
				if (ev.xkey.subwindow != None) {
					ManagedWindow *mw = managedWindowForWindow(ev.xkey.subwindow, pool);
					if (!mw) {
						break;
					}

					// Raise and activate the window
					lowerAllWindowsInPool(display, pool, gc);
					XRaiseWindow(display, mw->decorationWindow);
					XGetWindowAttributes(display, mw->decorationWindow, &attr);
					XFetchName(display, mw->actualWindow, &title);
					drawDecorations(display, mw->decorationWindow, title);

					// Check what was downed
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
						
						if (lastClickTime >= (time(NULL) - 1) && lastClickWindow == mw->decorationWindow) {
							collapseWindow(display, mw);
							lastClickTime = 0;
						}
						else {
							lastClickWindow = mw->decorationWindow;
							time(&lastClickTime);
						}
					}
					if (pointIsInRect(x, y, RECT_CLOSE_BTN)) {
					 	drawCloseButtonDown(display, mw->decorationWindow, gc, RECT_CLOSE_BTN);
						downState = MouseDownStateClose;
					}
					if (pointIsInRect(x, y, RECT_MAX_BTN)) {
						drawCloseButtonDown(display, mw->decorationWindow, gc, RECT_MAX_BTN);
						downState = MouseDownStateMaximize;
						lastClickTime = 0;
						printPool(pool);
					}
					if (ev.xbutton.subwindow == mw->resizer || pointIsInRect(x, y, RECT_RESIZE_BTN)) {
						// Grab the pointer
						XGrabPointer(display, ev.xbutton.subwindow, True,
									 PointerMotionMask|ButtonReleaseMask, GrabModeAsync,
									 GrabModeAsync, None, None, CurrentTime);
						start = ev.xbutton;
						lastClickTime = 0;
						downState = MouseDownStateResize;
					}
				}
			} break;
	#pragma mark MotionNotify
			case MotionNotify: {
				int x, y;
				// Invalidate double clicks
				lastClickTime = 0;
				
				while(XCheckTypedEvent(display, MotionNotify, &ev));
				
				x = ev.xbutton.x_root - start.x_root;
				y = ev.xbutton.y_root - start.y_root;
				switch (downState) {
					case MouseDownStateResize: {
						ManagedWindow *mw = managedWindowForWindow(start.subwindow, pool);
						XGetWindowAttributes(display, mw->decorationWindow, &attr);

						// Redraw Titlebar
						XFetchName(display, mw->actualWindow, &title);
						drawDecorations(display, mw->decorationWindow, title);
						
						// Redraw Resizer
						drawResizeButton(display, mw->resizer, gc, RECT_RESIZE_DRAW);
						
						// Resize
						resizeWindow(display, mw, attr.width + x, attr.height + y);
						start.x_root = ev.xbutton.x_root;
						start.y_root = ev.xbutton.y_root;
					} break;
					case MouseDownStateMove: {
						XMoveWindow(display, ev.xmotion.window, attr.x + x, attr.y + y);
					} break;
					case MouseDownStateClose: {
						if (pointIsInRect(x, y, RECT_CLOSE_BTN)) {
							drawCloseButtonDown(display, ev.xmotion.window, gc, RECT_CLOSE_BTN);							
						}
						else {
							drawCloseButton(display, ev.xmotion.window, gc, RECT_CLOSE_BTN);							
						}
					} break;
					case MouseDownStateMaximize: {
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
			} break;
	#pragma mark ButtonRelease
			case ButtonRelease: {
				XUngrabPointer(display, CurrentTime);

				x = ev.xbutton.x_root - attr.x;
				y = ev.xbutton.y_root - attr.y;

				switch (downState) {
					case MouseDownStateClose: {
						drawCloseButton(display, ev.xmotion.window, gc, RECT_CLOSE_BTN);
						
						if (pointIsInRect(x, y, RECT_CLOSE_BTN)) {
							unclaimWindow(display, ev.xmotion.window, pool);
						}
					} break;
					case MouseDownStateMaximize: {
						drawMaximizeButton(display, ev.xmotion.window, gc, RECT_MAX_BTN);
						
						if (pointIsInRect(x, y, RECT_MAX_BTN)) {
							ManagedWindow *mw = managedWindowForWindow(ev.xmotion.window, pool);
							if (mw) {
								maximizeWindow(display, mw);
							}
						}
					} break;
					default:
						break;
				}
			} break;
	#pragma mark MapNotify
			case MapNotify: {
				if (managedWindowForWindow(ev.xmap.window, pool) || ev.xmap.override_redirect) {
					break;
				}
				if (!ev.xmap.window) {
					logError("Recieved invalid window for event \"%s\"\n", event_names[ev.type]);
				}
				claimWindow(display, ev.xmap.window, root, pool);
			} break;
	#pragma mark DestroyNotify				
			case DestroyNotify: {
				unclaimWindow(display, ev.xdestroywindow.window, pool);
				cleanPool(display, pool);
			} break;
	// Intentionally unhandled notifications that are caught in the structure notification masks
	#pragma mark UnmapNotify
			case UnmapNotify:
	#pragma mark ReparentNotify
			case ReparentNotify:
	#pragma mark CreateNotify
			case CreateNotify:
	#pragma mark ConfigureNotify
			case ConfigureNotify:
	#pragma mark PropertyNotify
			case PropertyNotify:
				break;
			default: {
				logError("Recieved unhandled event \"%s\"\n", event_names[ev.type]);
			} break;
		}
		XFlush(display);
		XFreeGC(display, gc);
	}
	
	XCloseDisplay(display);
	destroyPool(pool);
	
	return 0;
}
