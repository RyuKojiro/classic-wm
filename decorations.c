//
//  decorations.c
//  
//
//  Created by Daniel Loffgren on 9/10/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <X11/Xlib.h>
#include <stdio.h>
#include <string.h>
#include "decorations.h"

static unsigned long white;
static unsigned long black;

int pointIsInRect(int px, int py, int rx, int ry, int rw, int rh) {
	if ((px >= rx && px <= (rx + rw)) && (py >= ry && py <= (ry + rh))) {
			return 1;
	}
	return 0;
}

Window decorateWindow(Display *display, Window window, Window root, int x, int y, int width, int height) {
	Window newParent;
	XSetWindowAttributes attrib;
	
	attrib.override_redirect = 1;
	
	newParent = XCreateWindow(display, root, x, y, width + 3, height + 2 + TITLEBAR_THICKNESS, 0, CopyFromParent, InputOutput, CopyFromParent, CWOverrideRedirect, &attrib);
	
	XReparentWindow(display, window, newParent, 0, TITLEBAR_THICKNESS - 1);
	
	Cursor cur = XCreateFontCursor(display, XC_left_ptr);
	XDefineCursor(display, newParent, cur);
	
	XMapWindow(display, newParent);
	
	char *title;
	XFetchName(display, window, &title);
	drawDecorations(display, newParent, title);
	
	return newParent;
}

void drawDecorations(Display *display, Window window, const char *title) {
	XWindowAttributes attr;
	white = XWhitePixel(display, DefaultScreen(display));
	black = XBlackPixel(display, DefaultScreen(display));
	int titleWillFit = 1;
	
	// Create GC
	GC gc = XCreateGC(display, window, 0, 0);	
	
	// Set up text
	const char *fontname = "-*-fixed-bold-r-*-14-*";
	XFontStruct *font = XLoadQueryFont(display, fontname);
	if (!font) {
		fprintf(stderr, "unable to load preferred font: %s using fixed", fontname);
		font = XLoadQueryFont(display, "fixed");
	}
	XSetFont(display, gc, font->fid);
	int twidth = XTextWidth(font, title, (int)strlen(title));
		
	// Get dimensions
	XGetWindowAttributes(display, window, &attr);
	
	if (attr.width < (twidth + 42)) {
		titleWillFit = 0;
	}

	// Draw bounding box
	XSetForeground(display, gc, white);
	XFillRectangle(display, window, gc, 1, 1, attr.width - 3, TITLEBAR_THICKNESS - 2);
	XSetForeground(display, gc, black);
	XDrawRectangle(display, window, gc, RECT_TITLEBAR);
	
	// Draw texture
	for (int y = TITLEBAR_TEXTURE_START; y < TITLEBAR_TEXTURE_START + TITLEBAR_CONTROL_SIZE; y += 2) {
		XDrawLine(display, window, gc, 2, y, attr.width - 4, y);
	}
	
	// White out areas for buttons and title
	XSetForeground(display, gc, white);
	// Title
	if (titleWillFit) {
		XFillRectangle(display, window, gc,
					   ((attr.width - twidth)/ 2) - 7,
					   4,
					   twidth + 14,
					   TITLEBAR_CONTROL_SIZE);
	}
	
	// Draw buttons and title
	XSetForeground(display, gc, black);
	// Shadow
	XDrawLine(display, window, gc, 1, attr.height - 1, attr.width, attr.height - 1);
	XDrawLine(display, window, gc, attr.width - 1, attr.height - 1, attr.width - 1, 1);

	// Draw title
	if (titleWillFit) {
		XSetForeground(display, gc, black);
		XSetBackground(display, gc, white);
		XDrawString(display, window, gc, ((attr.width - twidth)/ 2), TITLEBAR_TEXT_SIZE, title, (int)strlen(title));
	}
	
	// Draw Close Button
	drawCloseButton(display, window, gc, RECT_CLOSE_BTN);
	
	// Draw Maximize Button
	drawMaximizeButton(display, window, gc, RECT_MAX_BTN);
	
	XFlush(display);
	XFreeGC(display, gc);
}

void whiteOutUnderButton(Display *display, Window window, GC gc, int x, int y, int w, int h){
	// White out bg
	XSetForeground(display, gc, white);
	XFillRectangle(display, window, gc, x - 1, y, w + 3, h + 1);
}

void drawMaximizeButton(Display *display, Window window, GC gc, int x, int y, int w, int h) {	
	whiteOutUnderButton(display, window, gc, x, y, w, h);
	
	// Draw Border
	XSetForeground(display, gc, black);
	XDrawRectangle(display, window, gc, x, y, w, h);
	
	// Draw Inside
	XDrawRectangle(display, window, gc, x, y, w / 2, h / 2);
}

void drawCloseButton(Display *display, Window window, GC gc, int x, int y, int w, int h) {	
	whiteOutUnderButton(display, window, gc, x, y, w, h);

	// Draw Border
	XSetForeground(display, gc, black);
	XDrawRectangle(display, window, gc, x, y, w, h);
}

void drawCloseButtonDown(Display *display, Window window, GC gc, int x, int y, int w, int h) {	
	drawCloseButton(display, window, gc, x, y, w, h);
	
	// Draw first diag
	XDrawLine(display, window, gc, x + 2, y + 2, x + w - 2, y + h - 2);

	// Draw |
	XDrawLine(display, window, gc, x + w / 2, y, x + w / 2, y + h);

	// Draw -
	XDrawLine(display, window, gc, x, y + h / 2, x + w, y + h / 2);

	// Draw /
	XDrawLine(display, window, gc, x + w - 2, y + 2, x + 2, y + h - 2);

	// Remove Center
	XSetForeground(display, gc, white);
	XFillRectangle(display, window, gc, x + w / 2 - 1, y + h / 2 - 1, 3, 3);
}
