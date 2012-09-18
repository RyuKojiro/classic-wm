//
//  decorations.h
//  
//
//  Created by Daniel Loffgren on 9/10/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef _decorations_h
#define _decorations_h

// Hard dimensions
#define TITLEBAR_THICKNESS		19							// px tall (This should scale everything else)
#define TITLEBAR_TEXT_SIZE		15							// ptSize or pxSize
#define TITLEBAR_TEXTURE_START	4							// px from top to start texture
#define TITLEBAR_TEXTURE_SPACE	TITLEBAR_THICKNESS / 10 + 1	// px space between each line
#define TITLEBAR_CONTROL_SIZE	TITLEBAR_THICKNESS - 8		// px^2
#define TITLEBAR_TEXT_MARGIN	7							// px on either side
#define RESIZE_CONTROL_SIZE		15							// px^2

// Rects
#define RECT_TITLEBAR			0, 0, attr.width - 2, TITLEBAR_THICKNESS - 1
#define RECT_CLOSE_BTN			9, 4, TITLEBAR_CONTROL_SIZE - 1, TITLEBAR_CONTROL_SIZE - 1
#define RECT_MAX_BTN			attr.width - (10 + TITLEBAR_CONTROL_SIZE), 4, TITLEBAR_CONTROL_SIZE - 1, TITLEBAR_CONTROL_SIZE - 1
#define RECT_RESIZE_BTN			0, 0, RESIZE_CONTROL_SIZE, RESIZE_CONTROL_SIZE

// Cursors
#define XC_left_ptr 68

// Functions
Window decorateWindow(Display *display, Window window, Window root, int x, int y, int width, int height);
void drawDecorations(Display *display, Window window, const char *title);
void drawTitle(Display *display, Window window, GC gc, const char *title, XWindowAttributes attr);
int pointIsInRect(int px, int py, int rx, int ry, int rw, int rh);

// Individual Decorations
void whiteOutTitleBar(Display *display, Window window, GC gc, XWindowAttributes attr);
void whiteOutUnderButton(Display *display, Window window, GC gc, int x, int y, int w, int h);
void drawCloseButton(Display *display, Window window, GC gc, int x, int y, int w, int h);
void drawCloseButtonDown(Display *display, Window window, GC gc, int x, int y, int w, int h);
void drawMaximizeButton(Display *display, Window window, GC gc, int x, int y, int w, int h);
void drawResizeButton(Display *display, Window window, GC gc, int x, int y, int w, int h);

#endif
