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

Window decorateWindow(Display *display, Window window, Window root, int x, int y, int width, int height) {
	Window newParent;
	XSetWindowAttributes attrib;
	
	attrib.override_redirect = 1;
	
	newParent = XCreateWindow(display, root, x, y, width + 3, height + 3 + 19, 0, CopyFromParent, InputOutput, CopyFromParent, CWOverrideRedirect, &attrib);
	
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
	
	if (attribs.width < (twidth + 42)) {
		titleWillFit = 0;
	}
	
	// Get dimensions
	XGetWindowAttributes(display, window, &attribs);
	
	// Draw bounding box
	XSetForeground(display, gc, white);
	XFillRectangle(display, window, gc, 1, 1, attribs.width - 3, 19 - 2);
	XSetForeground(display, gc, black);
	XDrawRectangle(display, window, gc, 0, 0, attribs.width - 2, 19 - 1);
	
	// Draw texture
	for (int y = 4; y < 15; y += 2) {
		XDrawLine(display, window, gc, 2, y, attribs.width - 4, y);
	}
	
	// White out areas for buttons and title
	XSetForeground(display, gc, white);
	// Close button
	XFillRectangle(display, window, gc, 8, 4, 13, 11);
	// Title
	if (titleWillFit) {
		XFillRectangle(display, window, gc, ((attribs.width - twidth)/ 2) - 7, 4, twidth + 14, 11);
	}
	// Maximize button
	XFillRectangle(display, window, gc, attribs.width - 22, 4, 13, 11);
	// Subwindow box
	XFillRectangle(display, window, gc, 1, 19, attribs.width - 3, attribs.height - 21);
	
	// Draw buttons and title
	XSetForeground(display, gc, black);
	// Draw close button
	XDrawRectangle(display, window, gc, 9, 4, 10, 10);
	// Draw Maximize Button
	XDrawRectangle(display, window, gc, attribs.width - 21, 4, 10, 10);
	XDrawRectangle(display, window, gc, attribs.width - 21, 4, 6, 6);
	
	// Draw title
	if (titleWillFit) {
		XSetForeground(display, gc, black);
		XSetBackground(display, gc, white);
		XDrawString(display, window, gc, ((attribs.width - twidth)/ 2), 15, title, (int)strlen(title));
	}
	
	XFlush(display);
	XFreeGC(display, gc);
}
