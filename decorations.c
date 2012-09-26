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
	rw++;
	rh++;
	if ((px >= rx && px <= (rx + rw)) && (py >= ry && py <= (ry + rh))) {
			return 1;
	}
	return 0;
}

Window decorateWindow(Display *display, Window window, Window root, int x, int y, int width, int height, Window *resizer) {
	Window newParent;
	XSetWindowAttributes attrib;
	XWindowAttributes attr;
	char *title;

	attr.width = width;
	attr.height = height + TITLEBAR_THICKNESS;
	
	// Flag as override_redirect, so that we don't decorate decorations
	attrib.override_redirect = 1;
	
	// Create New Parent
	newParent = XCreateWindow(display, root, x, y, width + 3, height + 2 + TITLEBAR_THICKNESS, 0, CopyFromParent, InputOutput, CopyFromParent, CWOverrideRedirect, &attrib);
	XReparentWindow(display, window, newParent, 0, TITLEBAR_THICKNESS - 1);
	
	// Create Resize Button Window
	*resizer = XCreateWindow(display, newParent, RECT_RESIZE_BTN, 0, CopyFromParent, CopyFromParent, CopyFromParent, 0, 0);
	XMapRaised(display, *resizer);
	
	// Set Cursor
	Cursor cur = XCreateFontCursor(display, XC_left_ptr);
	XDefineCursor(display, newParent, cur);
	
	// Draw Time
	XMapWindow(display, newParent);
	XFetchName(display, window, &title);
	drawDecorations(display, newParent, title);

	GC gc = XCreateGC(display, window, 0, 0);	
	drawResizeButton(display, *resizer, gc, RECT_RESIZE_DRAW);
	XFlushGC(display, gc);
	XFreeGC(display, gc);

	return newParent;
}

void drawDecorations(Display *display, Window window, const char *title) {
	XWindowAttributes attr;
	white = XWhitePixel(display, DefaultScreen(display));
	black = XBlackPixel(display, DefaultScreen(display));
	
	// Create GC
	GC gc = XCreateGC(display, window, 0, 0);	
	
		
	// Get dimensions
	XGetWindowAttributes(display, window, &attr);
	
	// Draw bounding box
	whiteOutTitleBar(display, window, gc, attr);
	XSetForeground(display, gc, black);
	XDrawRectangle(display, window, gc, RECT_TITLEBAR);
	
	// Draw texture
	for (int y = TITLEBAR_TEXTURE_START; y < TITLEBAR_TEXTURE_START + TITLEBAR_CONTROL_SIZE; y += TITLEBAR_TEXTURE_SPACE) {
		XDrawLine(display, window, gc, 2, y, attr.width - 4, y);
	}
	
	// White out areas for buttons and title
	XSetForeground(display, gc, white);
	// Subwindow box
	XFillRectangle(display, window, gc,
				   1,
				   TITLEBAR_THICKNESS,
				   attr.width - 3,
				   attr.height - (TITLEBAR_THICKNESS + 3));
	
	// Draw buttons and title
	XSetForeground(display, gc, black);
	// Subwindow box
	XDrawRectangle(display, window, gc,
				   0,
				   TITLEBAR_THICKNESS - 1,
				   attr.width - 2,
				   attr.height - 20);
	// Shadow
	XDrawLine(display, window, gc, 1, attr.height - 1, attr.width, attr.height - 1);
	XDrawLine(display, window, gc, attr.width - 1, attr.height - 1, attr.width - 1, 1);

	// Draw Title
	drawTitle(display, window, gc, title, attr);
	
	// Draw Close Button
	drawCloseButton(display, window, gc, RECT_CLOSE_BTN);
	
	// Draw Maximize Button
	drawMaximizeButton(display, window, gc, RECT_MAX_BTN);

	// Draw Collapse Button
	//drawCollapseButton(display, window, gc, RECT_COLLAPSE_BTN);

	XFlush(display);
	XFreeGC(display, gc);
}

void whiteOutTitleBar(Display *display, Window window, GC gc, XWindowAttributes attr){
	XSetForeground(display, gc, white);
	XFillRectangle(display, window, gc, 1, 1, attr.width - 3, TITLEBAR_THICKNESS - 2);
}

void drawTitle(Display *display, Window window, GC gc, const char *title, XWindowAttributes attr){
	int titleWillFit = !!title;
	int twidth;
	
	if (titleWillFit) {
		// Set up text
		const char *fontname = "-FontForge-Chicago-Medium-R-Normal--12-120-75-75-P-78-MacRoman-0";
		XFontStruct *font = XLoadQueryFont(display, fontname);
		if (!font) {
			fprintf(stderr, "unable to load preferred font: %s using fixed", fontname);
			font = XLoadQueryFont(display, "fixed");
		}
		XSetFont(display, gc, font->fid);
		twidth = XTextWidth(font, title, (int)strlen(title));

		if (attr.width < (twidth + 42)) {
			titleWillFit = 0;
		}
	}

	XSetForeground(display, gc, white);
	if (titleWillFit) {
		// White out under Title
		XFillRectangle(display, window, gc,
					   ((attr.width - twidth)/ 2) - 7,
					   4,
					   twidth + 14,
					   TITLEBAR_CONTROL_SIZE);
	
		// Draw title
		XSetForeground(display, gc, black);
		XSetBackground(display, gc, white);
		XDrawString(display, window, gc, ((attr.width - twidth)/ 2), TITLEBAR_TEXT_SIZE, title, (int)strlen(title));
	}
}

void whiteOutUnderButton(Display *display, Window window, GC gc, int x, int y, int w, int h){
	// White out bg
	XSetForeground(display, gc, white);
	XFillRectangle(display, window, gc, x - 1, y, w + 3, h + 1);
}

void drawResizeButton(Display *display, Window window, GC gc, int x, int y, int w, int h) {
	whiteOutUnderButton(display, window, gc, x, y, w, h);

	// Draw Border
	XSetForeground(display, gc, black);
	XDrawRectangle(display, window, gc, x, y, w, h);

	// Bottom box
	XDrawRectangle(display, window, gc, x + 5, y + 5, 8, 8);

	// Top box
	XDrawRectangle(display, window, gc, x + 3, y + 3, 6, 6);
	
	// White out overlap
	XSetForeground(display, gc, white);
	XFillRectangle(display, window, gc, x + 4, y + 4, 5, 5);
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

void drawCollapseButton(Display *display, Window window, GC gc, int x, int y, int w, int h) {	
	whiteOutUnderButton(display, window, gc, x, y, w, h);
	
	// Draw Border
	XSetForeground(display, gc, black);
	XDrawRectangle(display, window, gc, x, y, w, h);

	XDrawRectangle(display, window, gc, x, y + w / 2 - 1, w, 2);
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
