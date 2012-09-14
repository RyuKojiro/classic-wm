//
//  decorations.c
//  
//
//  Created by Daniel Loffgren on 9/10/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <X11/Xlib.h>
#include "decorations.h"

Window decorateWindow(Display *display, Window window, Window root, int x, int y, int width, int height) {
	Window newParent;
	XSetWindowAttributes attrib;
	
	attrib.override_redirect = 1;
	
	newParent = XCreateWindow(display, root, x, y, width + 3, height + 3 + 19, 0, CopyFromParent, InputOutput, CopyFromParent, CWOverrideRedirect, &attrib);
	
	XReparentWindow(display, window, newParent, 0, 19);
	
	XMapWindow(display, newParent);	
	drawDecorations(display, newParent);
	
	return newParent;
}

void drawDecorations(Display *display, Window window) {
	XWindowAttributes attribs;
	XColor white;
	XColor black;
	
	// Set up colors
	Colormap colormap = DefaultColormap(display, 0);
	XParseColor(display, colormap, "#FFFFFF", &white);
	XAllocColor(display, colormap, &white);
	XParseColor(display, colormap, "#000000", &black);
	XAllocColor(display, colormap, &black);
	
	// Create GC
	GC gc = XCreateGC(display, window, 0, 0);	
	
	XGetWindowAttributes(display, window, &attribs);
	
	// Draw bounding box
	XSetForeground(display, gc, white.pixel);
	XFillRectangle(display, window, gc, 1, 1, attribs.width - 3, 19 - 2);
	XSetForeground(display, gc, black.pixel);
	XDrawRectangle(display, window, gc, 0, 0, attribs.width - 2, 19 - 1);
	
	// Draw texture
	for (int y = 4; y < 15; y += 2) {
		XDrawLine(display, window, gc, 2, y, attribs.width - 4, y);
	}
	
	// White out areas for buttons and title
	XSetForeground(display, gc, white.pixel);
	// Close button
	XFillRectangle(display, window, gc, 8, 4, 13, 11);
	// Title
	// Calculate width of title text, add 7 to each side
	// Draw the rectangle
	// Maximize button
	XFillRectangle(display, window, gc, attribs.width - 22, 4, 13, 11);

	// Draw buttons and title
	XSetForeground(display, gc, black.pixel);
	// Draw close button
	XDrawRectangle(display, window, gc, 9, 4, 10, 10);
	// Draw Maximize Button
	XDrawRectangle(display, window, gc, attribs.width - 21, 4, 10, 10);
	XDrawRectangle(display, window, gc, attribs.width - 21, 4, 6, 6);
	
	XFlush(display);
	XFreeGC(display, gc);
}
