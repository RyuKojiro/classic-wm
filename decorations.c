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
#include <err.h>
#include <string.h>
#include "decorations.h"

static unsigned long white;
static unsigned long black;
static XFontStruct *font;

int pointIsInRect(const int px, const int py, const int rx, const int ry, int rw, int rh) {
	rw++;
	rh++;
	return ((px >= rx && px <= (rx + rw)) &&
	        (py >= ry && py <= (ry + rh)));
}

Window decorateWindow(Display *display, Drawable window, Window root, GC gc, const int x, const int y, const int width, const int height, Window *resizer) {
	Window newParent;
	XSetWindowAttributes attrib;
	XWindowAttributes attr;
	XWindowAttributes incomingAttribs;
	char *title;

	/* This is entirely for window border compensation */
	/* FIXME: This _works_, but looks like crap for anything with more than a 1px border, in the future this should do up to one pixel and start adjusting the container window for the remainder */
	XGetWindowAttributes(display, window, &incomingAttribs);

	attr.width = width;
	attr.height = height + TITLEBAR_THICKNESS;

	/* Flag as override_redirect, so that we don't decorate decorations */
	attrib.override_redirect = 1;

	/* Create New Parent */
	newParent = XCreateWindow(display, root, x, y, width + FRAME_HORIZONTAL_THICKNESS, height + FRAME_VERTICAL_THICKNESS, 0, CopyFromParent, InputOutput, CopyFromParent, CWOverrideRedirect, &attrib);
	XReparentWindow(display, window, newParent, FRAME_LEFT_THICKNESS - incomingAttribs.border_width, TITLEBAR_THICKNESS - incomingAttribs.border_width);

	/* Create Resize Button Window */
	*resizer = XCreateWindow(display, newParent, RECT_RESIZE_BTN, 0, CopyFromParent, CopyFromParent, CopyFromParent, 0, 0);
	XMapRaised(display, *resizer);

	/* Set Cursor */
	Cursor cur = XCreateFontCursor(display, XC_left_ptr);
	XDefineCursor(display, newParent, cur);

	/* Readjust attributes to now refer to the decoration window */
	attr.width += FRAME_HORIZONTAL_THICKNESS;
	attr.height += FRAME_BOTTOM_THICKNESS;

	/* Draw Time! */
	XMapWindow(display, newParent);
	XFetchName(display, window, &title);
	drawDecorations(display, newParent, gc, title, attr, 1);
	drawResizeButton(display, *resizer, gc, RECT_RESIZE_DRAW);

	if (title) {
		XFree(title);
	}

	return newParent;
}

void undecorateWindow(Display *display, Window decorationWindow, Window resizer) {
	XUnmapWindow(display, resizer);
	XDestroyWindow(display, resizer);
	XUnmapWindow(display, decorationWindow);
	XDestroyWindow(display, decorationWindow);
}

static void whiteOutTitleBar(Display *display, Drawable window, GC gc, XWindowAttributes attr){
	XSetForeground(display, gc, white);
	XFillRectangle(display, window, gc, 1, 1, attr.width - FRAME_HORIZONTAL_THICKNESS, TITLEBAR_THICKNESS - 2);
}

void drawDecorations(Display *display, Drawable window, GC gc, const char *title, XWindowAttributes attr, const int focused) {
	if (!white || !black) {
		white = XWhitePixel(display, DefaultScreen(display));
		black = XBlackPixel(display, DefaultScreen(display));
	}

	/* Draw bounding box */
	whiteOutTitleBar(display, window, gc, attr);
	XSetForeground(display, gc, black);
	XDrawRectangle(display, window, gc, RECT_TITLEBAR);

	if (focused) {
		/* Draw texture */
		int y;
		for (y = TITLEBAR_TEXTURE_START; y < TITLEBAR_TEXTURE_START + TITLEBAR_CONTROL_SIZE; y += TITLEBAR_TEXTURE_SPACE) {
			XDrawLine(display, window, gc, 2, y, attr.width - 4, y);
		}
	}

	/* White out areas for buttons and title */
	XSetForeground(display, gc, white);
	/* Subwindow box */
	XFillRectangle(display, window, gc,
				FRAME_RIGHT_THICKNESS,
				TITLEBAR_THICKNESS,
				attr.width - FRAME_HORIZONTAL_THICKNESS,
				attr.height - FRAME_VERTICAL_THICKNESS - 1); /* FIXME: Is this extra 1 necessary? */

	/* Draw buttons and title */
	XSetForeground(display, gc, black);
	/* Subwindow box with each edge tucked in */
	XDrawRectangle(display, window, gc,
				FRAME_LEFT_THICKNESS - FRAME_TUCK_INSET,
				TITLEBAR_THICKNESS - FRAME_TUCK_INSET,
				attr.width - FRAME_LEFT_THICKNESS - FRAME_TUCK_INSET,
				attr.height - TITLEBAR_THICKNESS - FRAME_TUCK_INSET);
	/* Shadow */
	XDrawLine(display, window, gc, 1, attr.height - 1, attr.width, attr.height - 1); /* bottom */
	XDrawLine(display, window, gc, attr.width - 1, attr.height - 1, attr.width - 1, 1); /* left */

	/* White out the shadow ends */
	XSetForeground(display, gc, white);
	XDrawPoint(display, window, gc, 0, attr.height - 1); /* bottom left */
	XDrawPoint(display, window, gc, attr.width - 1, 0); /* top right */
	XSetForeground(display, gc, black);

	/* Draw Title */
	drawTitle(display, window, gc, title, attr);

	if (focused) {
		/* Draw Close Button */
		drawCloseButton(display, window, gc, RECT_CLOSE_BTN);

		/* Draw Maximize Button */
		drawMaximizeButton(display, window, gc, RECT_MAX_BTN);

#if COLLAPSE_BUTTON_ENABLED
		/* Draw Collapse Button */
		drawCollapseButton(display, window, gc, RECT_COLLAPSE_BTN);
#endif
	}
}

void drawTitle(Display *display, Drawable window, GC gc, const char *title, XWindowAttributes attr){
	int twidth;

	if (title) {
		/* Set up text */
		if (!font) {
			font = XLoadQueryFont(display, TITLEBAR_FONTNAME);
			if (!font) {
				warnx("unable to load preferred font: " TITLEBAR_FONTNAME " using fixed");
				font = XLoadQueryFont(display, "fixed");
				assert(font);
			}
		}
		XSetFont(display, gc, font->fid);
		twidth = XTextWidth(font, title, (int)strlen(title));

		/* If the title wont fit, don't bother drawing it, just bail */
		if (attr.width < (twidth + 42 + (2 * TITLEBAR_TEXT_MARGIN))) {
			return;
		}

		/* White out under Title */
		XSetForeground(display, gc, white);
		XFillRectangle(display, window, gc,
					((attr.width - twidth)/ 2) - TITLEBAR_TEXT_MARGIN,
					4,
					twidth + (2 * TITLEBAR_TEXT_MARGIN),
					TITLEBAR_CONTROL_SIZE);

		/* Draw title */
		XSetForeground(display, gc, black);
		XSetBackground(display, gc, white);
		XDrawString(display, window, gc, ((attr.width - twidth)/ 2), TITLEBAR_TEXT_OFFSET, title, (int)strlen(title));
	}
}

void whiteOutUnderButton(Display *display, Drawable window, GC gc, const int x, const int y, const int w, const int h){
	/* White out bg */
	XSetForeground(display, gc, white);
	XFillRectangle(display, window, gc, x - 1, y, w + 3, h + 1);
}

void drawResizeButton(Display *display, Drawable window, GC gc, const int x, const int y, const int w, const int h) {
	whiteOutUnderButton(display, window, gc, x, y, w, h);

	/* Draw Border */
	XSetForeground(display, gc, black);
	XDrawRectangle(display, window, gc, x, y, w, h);

	/* Bottom box */
	XDrawRectangle(display, window, gc, x + 5, y + 5, 8, 8);

	/* Top box */
	XDrawRectangle(display, window, gc, x + 3, y + 3, 6, 6);

	/* White out overlap */
	XSetForeground(display, gc, white);
	XFillRectangle(display, window, gc, x + 4, y + 4, 5, 5);
}

void drawMaximizeButton(Display *display, Drawable window, GC gc, const int x, const int y, const int w, const int h) {
	whiteOutUnderButton(display, window, gc, x, y, w, h);

	/* Draw Border */
	XSetForeground(display, gc, black);
	XDrawRectangle(display, window, gc, x, y, w, h);

	/* Draw Inside */
	XDrawRectangle(display, window, gc, x, y, w / 2, h / 2);
}

void drawCloseButton(Display *display, Drawable window, GC gc, const int x, const int y, const int w, const int h) {
	whiteOutUnderButton(display, window, gc, x, y, w, h);

	/* Draw Border */
	XSetForeground(display, gc, black);
	XDrawRectangle(display, window, gc, x, y, w, h);
}

void drawCollapseButton(Display *display, Drawable window, GC gc, const int x, const int y, const int w, const int h) {
	whiteOutUnderButton(display, window, gc, x, y, w, h);

	/* Draw Border */
	XSetForeground(display, gc, black);
	XDrawRectangle(display, window, gc, x, y, w, h);

	XDrawRectangle(display, window, gc, x, y + w / 2 - 1, w, 2);
}

void drawCloseButtonDown(Display *display, Drawable window, GC gc, const int x, const int y, const int w, const int h) {
	drawCloseButton(display, window, gc, x, y, w, h);

	/* Draw first diag */
	XDrawLine(display, window, gc, x + 2, y + 2, x + w - 2, y + h - 2);

	/* Draw | */
	XDrawLine(display, window, gc, x + w / 2, y, x + w / 2, y + h);

	/* Draw - */
	XDrawLine(display, window, gc, x, y + h / 2, x + w, y + h / 2);

	/* Draw / */
	XDrawLine(display, window, gc, x + w - 2, y + 2, x + 2, y + h - 2);

	/* Remove Center */
	XSetForeground(display, gc, white);
	XFillRectangle(display, window, gc, x + w / 2 - 1, y + h / 2 - 1, 3, 3);
}
