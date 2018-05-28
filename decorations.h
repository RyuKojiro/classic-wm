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

#ifndef _decorations_h
#define _decorations_h

#include <X11/Xlib.h>
#include <X11/extensions/Xdbe.h>

/* Titlebar Font */
#define TITLEBAR_FONTNAME       "-*-Chicago-*-*-*--12-*-*-*-*-*-*-*"

/* Hard dimensions */
#define TITLEBAR_THICKNESS         (19)                            /* px tall (This should scale everything else) */
#define TITLEBAR_TEXT_OFFSET       (14)                            /* ptSize or pxSize */
#define TITLEBAR_TEXTURE_START     (4)                             /* px from top to start texture */
#define TITLEBAR_TEXTURE_SPACE     (TITLEBAR_THICKNESS / 10 + 1)   /* px space between each line */
#define TITLEBAR_CONTROL_SIZE      (TITLEBAR_THICKNESS - 8)        /* px^2 */
#define TITLEBAR_TEXT_MARGIN       (7)                             /* px on either side */
#define RESIZE_CONTROL_SIZE        (15)                            /* px^2 */
#define FRAME_LEFT_THICKNESS       (1)                             /* px wide */
#define FRAME_RIGHT_THICKNESS      (2)                             /* px wide */
#define FRAME_BOTTOM_THICKNESS     (2)                             /* px tall */
#define COLLAPSED_THICKNESS        (TITLEBAR_THICKNESS - 1 + FRAME_BOTTOM_THICKNESS)
#define FRAME_VERTICAL_THICKNESS   (TITLEBAR_THICKNESS + FRAME_BOTTOM_THICKNESS)
#define FRAME_HORIZONTAL_THICKNESS (FRAME_LEFT_THICKNESS + FRAME_RIGHT_THICKNESS)
#define FRAME_TUCK_INSET           (1)                             /* X windows tend to bring their own 1px border */

/* Rects */
#define RECT_TITLEBAR              0, 0, attr.width - 2, TITLEBAR_THICKNESS - 1
#define RECT_CLOSE_BTN             9, 4, TITLEBAR_CONTROL_SIZE - 1, TITLEBAR_CONTROL_SIZE - 1
#define RECT_RESIZE_BTN            attr.width - RESIZE_CONTROL_SIZE + 1, attr.height - RESIZE_CONTROL_SIZE, RESIZE_CONTROL_SIZE, RESIZE_CONTROL_SIZE
#define RECT_RESIZE_DRAW           0, 0, RESIZE_CONTROL_SIZE, RESIZE_CONTROL_SIZE

/* Right hand titlebar controls */
#define FIRST_RIGHTMOST_BTN        attr.width - (10 + TITLEBAR_CONTROL_SIZE), 4, TITLEBAR_CONTROL_SIZE - 1, TITLEBAR_CONTROL_SIZE - 1
#define SECOND_RIGHTMOST_BTN       attr.width - (7 + TITLEBAR_CONTROL_SIZE) * 2 - 2, 4, TITLEBAR_CONTROL_SIZE - 1, TITLEBAR_CONTROL_SIZE - 1

#if COLLAPSE_BUTTON_ENABLED
#define RECT_COLLAPSE_BTN          FIRST_RIGHTMOST_BTN
#define RECT_MAX_BTN               SECOND_RIGHTMOST_BTN
#else
#define RECT_MAX_BTN               FIRST_RIGHTMOST_BTN
#endif

/* Cursors */
#define XC_left_ptr (68)

/* Double Buffering */
#define DRAW_ACTION(display, window, action) { \
	XdbeSwapInfo swap_info; \
	swap_info.swap_window = window; \
	swap_info.swap_action = XdbeCopied; \
	XdbeBeginIdiom(display); \
	action \
	XdbeSwapBuffers(display, &swap_info, 1); \
	XdbeEndIdiom(display); \
}


/* Functions */
Window decorateWindow(Display *display, Drawable window, Window root, GC gc, int x, int y, int width, int height, Window *resizer);
void undecorateWindow(Display *display, Window decorationWindow, Window resizer);
void drawDecorations(Display *display, Drawable window, GC gc, const char *title, XWindowAttributes attr, int focused);
void drawTitle(Display *display, Drawable window, GC gc, const char *title, XWindowAttributes attr);
int pointIsInRect(int px, int py, int rx, int ry, int rw, int rh);

typedef void (decorationFunction)(Display *display, Drawable window, GC gc, int x, int y, int w, int h);

/* Individual Decorations */
void whiteOutUnderButton(Display *display, Drawable window, GC gc, int x, int y, int w, int h);
void drawCloseButton(Display *display, Drawable window, GC gc, int x, int y, int w, int h);
void drawCloseButtonDown(Display *display, Drawable window, GC gc, int x, int y, int w, int h);
void drawMaximizeButton(Display *display, Drawable window, GC gc, int x, int y, int w, int h);
void drawResizeButton(Display *display, Drawable window, GC gc, int x, int y, int w, int h);
void drawCollapseButton(Display *display, Drawable window, GC gc, int x, int y, int w, int h);

#endif
