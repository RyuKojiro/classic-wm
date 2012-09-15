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
	
	newParent = XCreateWindow(display, root, x, y, width + 3, height + 3 + TITLEBAR_THICKNESS, 0, CopyFromParent, InputOutput, CopyFromParent, CWOverrideRedirect, &attrib);
	
	XReparentWindow(display, window, newParent, 0, 19);
	
	XMapWindow(display, newParent);
	
	drawDecorations(display, newParent, "Test title");
	
	return newParent;
}

void drawDecorations(Display *display, Window window, const char *title) {
	XWindowAttributes attr;
	unsigned long white = XWhitePixel(display, DefaultScreen(display));
	unsigned long black = XBlackPixel(display, DefaultScreen(display));
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
	// Close button
	XFillRectangle(display, window, gc,
				   8,
				   4,
				   TITLEBAR_CONTROL_SIZE + 2,
				   TITLEBAR_CONTROL_SIZE);
	// Title
	if (titleWillFit) {
		XFillRectangle(display, window, gc,
					   ((attr.width - twidth)/ 2) - 7,
					   4,
					   twidth + 14,
					   TITLEBAR_CONTROL_SIZE);
	}
	// Maximize button
	XFillRectangle(display, window, gc,
				   attr.width - (TITLEBAR_CONTROL_SIZE + 11),
				   4,
				   TITLEBAR_CONTROL_SIZE + 2,
				   TITLEBAR_CONTROL_SIZE);
	// Subwindow box
	XFillRectangle(display, window, gc,
				   1,
				   TITLEBAR_THICKNESS,
				   attr.width - 3,
				   attr.height - (TITLEBAR_THICKNESS + 3));
	
	// Draw buttons and title
	XSetForeground(display, gc, black);
	// Draw close button
	XDrawRectangle(display, window, gc, RECT_CLOSE_BTN);
	// Draw Maximize Button
	XDrawRectangle(display, window, gc, RECT_MAX_BTN);
	XDrawRectangle(display, window, gc,
				   attr.width - 21,
				   4,
				   6,
				   6);
	// Subwindow box
	XDrawRectangle(display, window, gc,
				   0,
				   TITLEBAR_THICKNESS - 1,
				   attr.width - 2,
				   attr.height - 20);
	// Shadow
	XDrawLine(display, window, gc, 1, attr.height - 1, attr.width, attr.height - 1);
	XDrawLine(display, window, gc, attr.width - 1, attr.height - 1, attr.width - 1, 1);

	// Draw title
	if (titleWillFit) {
		XSetForeground(display, gc, black);
		XSetBackground(display, gc, white);
		XDrawString(display, window, gc, ((attr.width - twidth)/ 2), TITLEBAR_TEXT_SIZE, title, (int)strlen(title));
	}
	
	XFlush(display);
	XFreeGC(display, gc);
}
