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

#define TITLEBAR_CONTROL_SIZE	11		// px dimension for buttons
#define TITLEBAR_TEXT_SIZE		15		// ptSize or pxSize
#define TITLEBAR_TEXTURE_START	4		// px from top to start texture
#define TITLEBAR_THICKNESS		19		// px tall
#define TITLEBAR_TEXT_MARGIN	7		// px on either side

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
	XWindowAttributes attribs;
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
	XGetWindowAttributes(display, window, &attribs);
	
	if (attribs.width < (twidth + 42)) {
		titleWillFit = 0;
	}

	// Draw bounding box
	XSetForeground(display, gc, white);
	XFillRectangle(display, window, gc, 1, 1, attribs.width - 3, TITLEBAR_THICKNESS - 2);
	XSetForeground(display, gc, black);
	XDrawRectangle(display, window, gc, 0, 0, attribs.width - 2, TITLEBAR_THICKNESS - 1);
	
	// Draw texture
	for (int y = TITLEBAR_TEXTURE_START; y < TITLEBAR_TEXTURE_START + TITLEBAR_CONTROL_SIZE; y += 2) {
		XDrawLine(display, window, gc, 2, y, attribs.width - 4, y);
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
					   ((attribs.width - twidth)/ 2) - 7,
					   4,
					   twidth + 14,
					   TITLEBAR_CONTROL_SIZE);
	}
	// Maximize button
	XFillRectangle(display, window, gc,
				   attribs.width - (TITLEBAR_CONTROL_SIZE + 11),
				   4,
				   TITLEBAR_CONTROL_SIZE + 2,
				   TITLEBAR_CONTROL_SIZE);
	// Subwindow box
	XFillRectangle(display, window, gc,
				   1,
				   TITLEBAR_THICKNESS,
				   attribs.width - 3,
				   attribs.height - (TITLEBAR_THICKNESS + 3));
	
	// Draw buttons and title
	XSetForeground(display, gc, black);
	// Draw close button
	XDrawRectangle(display, window, gc,
				   9,
				   4,
				   TITLEBAR_CONTROL_SIZE - 1,
				   TITLEBAR_CONTROL_SIZE - 1);
	// Draw Maximize Button
	XDrawRectangle(display, window, gc,
				   attribs.width - (10 + TITLEBAR_CONTROL_SIZE),
				   4,
				   TITLEBAR_CONTROL_SIZE - 1,
				   TITLEBAR_CONTROL_SIZE - 1);
	XDrawRectangle(display, window, gc,
				   attribs.width - 21,
				   4,
				   6,
				   6);
	// Subwindow box
	XDrawRectangle(display, window, gc,
				   0,
				   TITLEBAR_THICKNESS - 1,
				   attribs.width - 2,
				   attribs.height - 20);
	// Shadow
	XDrawLine(display, window, gc, 1, attribs.height - 1, attribs.width, attribs.height - 1);
	XDrawLine(display, window, gc, attribs.width - 1, attribs.height - 1, attribs.width - 1, 1);

	// Draw title
	if (titleWillFit) {
		XSetForeground(display, gc, black);
		XSetBackground(display, gc, white);
		XDrawString(display, window, gc, ((attribs.width - twidth)/ 2), TITLEBAR_TEXT_SIZE, title, (int)strlen(title));
	}
	
	XFlush(display);
	XFreeGC(display, gc);
}
