/*
 * Copyright (c) 2012 Daniel Loffgren
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <assert.h>
#include <err.h>       /* warnx */
#include <stdarg.h>    /* va_list */
#include <stdlib.h>    /* getenv */
#include <sysexits.h>  /* EX_UNAVAILABLE */
#include <time.h>      /* time() */
#include <X11/Xlib.h>
#include <X11/Xutil.h> /* XSizeHints */

#include "eventnames.h"
#include "decorations.h"
#include "pool.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define NEW_WINDOW_OFFSET 0 /*((XDisplayWidth(display, DefaultScreen(display)) > 2560) ? 0 : 22) */

typedef enum {
	MouseDownStateUnknown = 0,
	MouseDownStateMove,
	MouseDownStateClose,
	MouseDownStateMaximize,
#if COLLAPSE_BUTTON_ENABLED
	MouseDownStateCollapse,
#endif
	MouseDownStateResize
} MouseDownState;

/* HACK: Find a better way to eat the extraneous Expose events on destruction of decorations */
static Window decorationWindowDestroyed;
static Window resizerDestroyed;

static void resizeWindow(Display *display, ManagedWindow *mw, int w, int h) {
	/* Set some absolute minimums */
	w = MAX(w, ((TITLEBAR_CONTROL_SIZE) * 5));
	h = MAX(h, ((TITLEBAR_THICKNESS) * 2) + RESIZE_CONTROL_SIZE);

	/* Respect the window minimums, if they exist */
	w = MAX(w, mw->min_w);
	h = MAX(h, mw->min_h);

	/* Size the decorations as requested, and inset the actual window */
	XResizeWindow(display, mw->decorationWindow, w, h);
	XResizeWindow(display, mw->actualWindow, w - FRAME_HORIZONTAL_THICKNESS, h - FRAME_VERTICAL_THICKNESS);
	XMoveWindow(display, mw->resizer, w - RESIZE_CONTROL_SIZE - FRAME_RIGHT_THICKNESS, h - RESIZE_CONTROL_SIZE - FRAME_BOTTOM_THICKNESS);
}

static void lowerAllWindowsInPool(Display *display, ManagedWindowPool *pool, GC gc) {
	ManagedWindow *this;
	SLIST_FOREACH(this, &pool->windows, entries) {
		if (this != pool->active) {
			XWindowAttributes attr;
			XGetWindowAttributes(display, this->decorationWindow, &attr);
			DRAW_ACTION(display, this->decorationWindow, {
				drawDecorations(display, this->decorationBuffer, gc, this->title, attr, 0);
			});
			XGrabButton(display, 0, AnyModifier, this->actualWindow, 0, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
		}
	}
}

static void focusWindow(Display *display, ManagedWindow *mw, GC gc, ManagedWindowPool *pool) {
	pool->active = mw;
	lowerAllWindowsInPool(display, pool, gc);
	XRaiseWindow(display, mw->decorationWindow);
	XUngrabButton(display, 0, AnyModifier, mw->actualWindow);

	/* If the window is collapsed, move input focus to the decoration window */
	Window windowToFocus = mw->collapsed ? mw->decorationWindow : mw->actualWindow;
	XSetInputFocus(display, windowToFocus, RevertToNone, CurrentTime);
}

static void collapseWindow(Display *display, ManagedWindowPool *pool, ManagedWindow *mw, GC gc) {
	XWindowAttributes attr;

	XGetWindowAttributes(display, mw->decorationWindow, &attr);

	if (mw->collapsed) {
		/* collapsed, uncollapse it */
		XResizeWindow(display, mw->decorationWindow, mw->last_w, mw->last_h);
		attr.height = mw->last_h;
		XMapWindow(display, mw->actualWindow);

		/* Redraw Resizer */
		drawResizeButton(display, mw->resizer, gc, RECT_RESIZE_DRAW);
		XRaiseWindow(display, mw->resizer);

		mw->collapsed = 0;
	}
	else {
		/* normal, collapse it */
		mw->last_w = attr.width;
		mw->last_h = attr.height;
		XResizeWindow(display, mw->decorationWindow, attr.width, COLLAPSED_THICKNESS);
		attr.height = COLLAPSED_THICKNESS;
		XUnmapWindow(display, mw->actualWindow);

		mw->collapsed = 1;
	}

	drawDecorations(display, mw->decorationBuffer, gc, mw->title, attr, 1);
	focusWindow(display, mw, gc, pool);
}

static void maximizeWindow(Display *display, ManagedWindow *mw, GC gc) {
	XSizeHints attr;
	XSizeHints container;
	long supplied_return;
	long supplied_return_container;

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
	else { /* if we aren't at max size, and there is one, go to it */
		mw->last_h = attr.height + FRAME_VERTICAL_THICKNESS;
		mw->last_w = attr.width + FRAME_HORIZONTAL_THICKNESS;
		mw->last_x = container.x;
		mw->last_y = container.y;

		XMoveWindow(display, mw->decorationWindow, 0, NEW_WINDOW_OFFSET);
		resizeWindow(display, mw, max_w, max_h - NEW_WINDOW_OFFSET);
	}

	/* Get dimensions */
	Window w2; /* unused */
	XWindowAttributes geometry;
	XGetGeometry(display, mw->actualWindow, &w2,
				 (int *)&geometry.x, (int *)&geometry.y,
				 (unsigned int *)&geometry.width, (unsigned int *)&geometry.height,
				 (unsigned int *)&geometry.border_width, (unsigned int *)&geometry.depth);


	DRAW_ACTION(display, mw->decorationWindow, {
		drawDecorations(display, mw->decorationBuffer, gc, mw->title, geometry, 1);
	});
}

static void claimWindow(Display *display, Window window, Window root, GC gc, ManagedWindowPool *pool) {
	XSizeHints attr;
	long supplied_return = PPosition | PSize | PMinSize;
	Window resizer;

	XGetWMNormalHints(display, window, &attr, &supplied_return);

	/*
	XMoveWindow(display, window, attr.x, attr.y);
	XResizeWindow(display, window, attr.width, attr.height);

	warnx("Trying to reparent %d at {%d, %d, %d, %d} with flags %d\n", window, attr.x, attr.y, attr.width, attr.height, attr.flags);
	*/

	Window deco = decorateWindow(display, window, root, gc, attr.x, attr.y, attr.width, attr.height, &resizer);

	/*
	XMoveWindow(display, deco, XDisplayWidth(display, DefaultScreen(display)) - attr.width - 3, NEW_WINDOW_OFFSET);
	*/

	/* Start listening for events on the window */
	/* FIXME: is this where focus events should be listened to? */
	XSelectInput(display, window, SubstructureNotifyMask | ExposureMask);
	XSelectInput(display, deco, ExposureMask);
	XSelectInput(display, resizer, ExposureMask);

	pool->active = addWindowToPool(display, deco, window, resizer, pool);
	focusWindow(display, pool->active, gc, pool);

	if (supplied_return & PMinSize) {
		pool->active->min_w = attr.min_width;
		pool->active->min_h = attr.min_height;
	}
}

static void unclaimWindow(Display *display, Window window, ManagedWindowPool *pool) {
	ManagedWindow *mw = managedWindowForWindow(display, window, pool);
	if (mw) {
		undecorateWindow(display, mw->decorationWindow, mw->resizer);
		decorationWindowDestroyed = mw->decorationWindow;
		resizerDestroyed = mw->resizer;
		removeWindowFromPool(display, mw, pool);
	}
}

static void redrawButtonState(int *stateToken, decorationFunction buttonFunction, Display *display, Drawable window, GC gc, int px, int py, int bx, int by, int bw, int bh) {
	/*
	 * Zero is up state (default). So, a zero initialized stateToken shold always
	 * reflect the correct state for a button that has never seen this function.
	 */
	int newState = pointIsInRect(px, py, bx, by, bw, bh);

	if (newState != *stateToken) {
		if (newState) {
			drawCloseButtonDown(display, window, gc, bx, by, bw, bh);
		}
		else {
			/* This ensures the the button is redrawn unclicked */
			buttonFunction(display, window, gc, bx, by, bw, bh);
		}

		*stateToken = newState;
	}
}

static void claimAllWindows(Display *display, Window root, ManagedWindowPool *pool) {
	/* This should only be called once, and only on startup */
	static int once;
	assert(!once++);

	Window parent;
	Window *children;
	unsigned int nchildren;

	XQueryTree(display, root, &root, &parent, &children, &nchildren);

	unsigned int i;
	for (i = 0; i < nchildren; i++) {
		if (children[i] && children[i] != root) {
			GC gc = XCreateGC(display, children[i], 0, 0);
			claimWindow(display, children[i], root, gc, pool);
			XFlush(display);
			XFreeGC(display, gc);
		}
		else {
			warnx("Could not find window with XID:%ld\n", children[i]);
		}
	}

	XFree(children);
}

int main (int argc, const char * argv[]) {
	(void)argc;
	(void)argv;

	Display *display;
	XEvent ev;
	int screen;
	XWindowAttributes attr;
	XButtonEvent start = {0};
	MouseDownState downState = MouseDownStateUnknown;
	time_t lastClickTime = 0;
	Window lastClickWindow = 0;

	ManagedWindowPool *pool = createPool();

	/* Set up */
	display = XOpenDisplay(getenv("DISPLAY"));
	if (!display) {
		errx(EX_UNAVAILABLE, "Failed to open display, is X running?\n");
	}

	screen = DefaultScreen(display);

	/* Find the window */
	Window root = RootWindow(display, screen);

	/* Initial capture of all windows on startup */
	claimAllWindows(display, root, pool);

	XSelectInput(display, root, StructureNotifyMask | SubstructureNotifyMask /* CreateNotify */ | ButtonPressMask);

	while(XNextEvent(display, &ev) == 0) {
		/*
		warnx("Got event \"%s\"\n", event_names[ev.type]);
		*/
		if (ev.xany.window == decorationWindowDestroyed || ev.xany.window == resizerDestroyed) {
			continue;
		}

		/* This is a collection of everything that should short-circuit */
		switch(ev.type) {
			case DestroyNotify:
				/*
				 * NOTE: The XDestroyWindowEvent structure is tricky.
				 * ev.xany.window lines up with ev.xdestroywindow.event,
				 * because xdestroywindow.event is the window being
				 * destroyed, while xdestroywindow.window is used for some
				 * other toolkit nonsense.
				 */
				unclaimWindow(display, ev.xdestroywindow.event, pool);
			case UnmapNotify:
			case ReparentNotify:
			case CreateNotify:
			case ConfigureNotify:
			case PropertyNotify:
				/*
				 * These are intentionally unhandled notifications that are
				 * caught in the structure notification masks. So, don't
				 * let the default case log them.
				 */
				continue;

			case ButtonPress: {
				if (ev.xkey.subwindow == None) {
					continue;
				}
			} break;
			case Expose: {
				if(downState == MouseDownStateResize) {
					continue;
				}
			} break;
			case MapNotify: {
				if (managedWindowForWindow(display, ev.xmap.window, pool) || ev.xmap.override_redirect) {
					continue;
				}
			} break;
		}

		GC gc = XCreateGC(display, ev.xany.window, 0, 0);
		switch (ev.type) {
			case ButtonPress: {
				ManagedWindow *mw = managedWindowForWindow(display, ev.xkey.subwindow, pool);
				if (!mw) {
					break;
				}

				/* Raise and activate the window, while lowering all others */
				focusWindow(display, mw, gc, pool);

				/* Redraw the decorations, just in case the focus changed */
				XGetWindowAttributes(display, mw->decorationWindow, &attr);
				drawDecorations(display, mw->decorationWindow, gc, mw->title, attr, 1);

				/*
				 * These x,y assignments cannot be consolidated with the other
				 * ones, since these don't recycle the previous attr, but the
				 * other ones do.
				 */
				const int x = ev.xbutton.x_root - attr.x;
				const int y = ev.xbutton.y_root - attr.y;

				/* Check what was downed */
				downState = MouseDownStateUnknown;
				if (pointIsInRect(x, y, RECT_TITLEBAR)) {
					downState = MouseDownStateMove;
					/* Grab the pointer */
					XGrabPointer(display, ev.xbutton.subwindow, True,
								 PointerMotionMask|ButtonReleaseMask, GrabModeAsync,
								 GrabModeAsync, None, None, CurrentTime);
					start = ev.xbutton;
				}
				if (pointIsInRect(x, y, RECT_CLOSE_BTN)) {
					drawCloseButtonDown(display, mw->decorationWindow, gc, RECT_CLOSE_BTN);
					downState = MouseDownStateClose;
				}
				if (pointIsInRect(x, y, RECT_MAX_BTN)) {
					drawCloseButtonDown(display, mw->decorationWindow, gc, RECT_MAX_BTN);
					downState = MouseDownStateMaximize;
					lastClickTime = 0;
				}
#if COLLAPSE_BUTTON_ENABLED
				if (pointIsInRect(x, y, RECT_COLLAPSE_BTN)) {
					drawCloseButtonDown(display, mw->decorationWindow, gc, RECT_COLLAPSE_BTN);
					downState = MouseDownStateCollapse;
					lastClickTime = 0;
				}
#endif
				if (!mw->collapsed &&
					(ev.xbutton.subwindow == mw->resizer || pointIsInRect(x, y, RECT_RESIZE_BTN))) {
					/* Grab the pointer */
					XGrabPointer(display, ev.xbutton.subwindow, True,
								 PointerMotionMask|ButtonReleaseMask, GrabModeAsync,
								 GrabModeAsync, None, None, CurrentTime);
					start = ev.xbutton;
					lastClickTime = 0;
					downState = MouseDownStateResize;
				}
			} break;
			case Expose: {
				ManagedWindow *mw = managedWindowForWindow(display, ev.xexpose.window, pool);

				if (mw) {
					/* TODO: This probably performs terribly, replace me with some caching */
					XWindowAttributes geometry;
					Window w2; /* unused */
					XGetGeometry(display, mw->decorationWindow, &w2,
								 (int *)&geometry.x, (int *)&geometry.y,
								 (unsigned int *)&geometry.width, (unsigned int *)&geometry.height,
								 (unsigned int *)&geometry.border_width, (unsigned int *)&geometry.depth);

					/* Redraw titlebar based on active or not */
					DRAW_ACTION(display, mw->decorationWindow, {
						drawDecorations(display, mw->decorationBuffer, gc, mw->title, geometry, (mw == pool->active));
					});

					/* Redraw Resizer */
					drawResizeButton(display, mw->resizer, gc, RECT_RESIZE_DRAW);
				}
			} break;
			case MotionNotify: {
				/* Invalidate double clicks */
				lastClickTime = 0;

				/* If we have a bunch of MotionNotify events queued up, */
				/* drop all but the last one, since all math is relative */
				while(XCheckTypedEvent(display, MotionNotify, &ev));

				const int x = ev.xbutton.x_root - attr.x;
				const int y = ev.xbutton.y_root - attr.y;

				const int dx = ev.xbutton.x_root - start.x_root;
				const int dy = ev.xbutton.y_root - start.y_root;

				switch (downState) {
					case MouseDownStateResize: {
						ManagedWindow *mw = managedWindowForWindow(display, start.subwindow, pool);

						/* Resize */
						resizeWindow(display, mw, attr.width + dx, attr.height + dy);
						start.x_root = ev.xbutton.x_root;
						start.y_root = ev.xbutton.y_root;

						/* Persist that info for next iteration */
						attr.width += dx;
						attr.height += dy;

						/* Redraw Titlebar */
						DRAW_ACTION(display, mw->decorationWindow, {
							drawDecorations(display, mw->decorationBuffer, gc, mw->title, attr, 1);
						});

						/* Redraw Resizer */
						drawResizeButton(display, mw->resizer, gc, RECT_RESIZE_DRAW);
					} break;
					case MouseDownStateMove: {
						XMoveWindow(display, ev.xmotion.window, attr.x + dx, attr.y + dy);
					} break;
					case MouseDownStateClose: {
						static int closeButtonStateToken;
						redrawButtonState(&closeButtonStateToken, drawCloseButton, display, ev.xmotion.window, gc, x, y, RECT_CLOSE_BTN);
					} break;
					case MouseDownStateMaximize: {
						static int maximizeButtonStateToken;
						redrawButtonState(&maximizeButtonStateToken, drawMaximizeButton, display, ev.xmotion.window, gc, x, y, RECT_MAX_BTN);
					} break;
#if COLLAPSE_BUTTON_ENABLED
					case MouseDownStateCollapse: {
						static int collapseButtonStateToken;
						redrawButtonState(&collapseButtonStateToken, drawCollapseButton, display, ev.xmotion.window, gc, x, y, RECT_COLLAPSE_BTN);
					} break;
#endif
					default:
						break;
				}
			} break;
			case ButtonRelease: {
				XUngrabPointer(display, CurrentTime);

				const int x = ev.xbutton.x_root - attr.x;
				const int y = ev.xbutton.y_root - attr.y;

				switch (downState) {
					case MouseDownStateClose: {
						drawCloseButton(display, ev.xmotion.window, gc, RECT_CLOSE_BTN);

						if (pointIsInRect(x, y, RECT_CLOSE_BTN)) {
							unclaimWindow(display, ev.xmotion.window, pool);
						}
					} break;
#if COLLAPSE_BUTTON_ENABLED
					case MouseDownStateCollapse: {
						drawCollapseButton(display, ev.xmotion.window, gc, RECT_COLLAPSE_BTN);

						if (pointIsInRect(x, y, RECT_COLLAPSE_BTN)) {
							ManagedWindow *mw = managedWindowForWindow(ev.xmotion.window, pool);
							collapseWindow(display, pool, mw, gc);
							lastClickTime = 0;
						}
					} break;
#endif
					case MouseDownStateMaximize: {
						drawMaximizeButton(display, ev.xmotion.window, gc, RECT_MAX_BTN);

						if (pointIsInRect(x, y, RECT_MAX_BTN)) {
							ManagedWindow *mw = managedWindowForWindow(display, ev.xmotion.window, pool);
							maximizeWindow(display, mw, gc);
						}
					} break;
					default: { /* Anywhere else on the titlebar */
						if (ev.xkey.window != None) {
							ManagedWindow *mw = managedWindowForWindow(display, ev.xkey.window, pool);

							if (lastClickTime >= (time(NULL) - 1) && lastClickWindow == mw->decorationWindow) {
								collapseWindow(display, pool, mw, gc);
								lastClickTime = 0;
							}
							else {
								lastClickWindow = mw->decorationWindow;
								time(&lastClickTime);
							}
						}
					} break;
				}
			} break;
			case MapNotify: {
				if (!ev.xmap.window) {
					warnx("Recieved invalid window for event \"%s\"\n", event_names[ev.type]);
				}
				claimWindow(display, ev.xmap.window, root, gc, pool);
			} break;
			default: {
				warnx("Recieved unhandled event \"%s\"\n", event_names[ev.type]);
			} break;
		}

		XFreeGC(display, gc);
	}

	XCloseDisplay(display);
	destroyPool(pool);

	return 0;
}
