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
#define TITLEBAR_CONTROL_SIZE	11		// px dimension for buttons
#define TITLEBAR_TEXT_SIZE		15		// ptSize or pxSize
#define TITLEBAR_TEXTURE_START	4		// px from top to start texture
#define TITLEBAR_THICKNESS		19		// px tall
#define TITLEBAR_TEXT_MARGIN	7		// px on either side

// Rects
#define RECT_TITLEBAR			0, 0, attr.width - 2, TITLEBAR_THICKNESS - 1
#define RECT_CLOSE_BTN			9, 4, TITLEBAR_CONTROL_SIZE - 1, TITLEBAR_CONTROL_SIZE - 1
#define RECT_MAX_BTN			attr.width - (10 + TITLEBAR_CONTROL_SIZE), 4, TITLEBAR_CONTROL_SIZE - 1, TITLEBAR_CONTROL_SIZE - 1

// Cursors
#define XC_left_ptr 68

// Functions
Window decorateWindow(Display *display, Window window, Window root, int x, int y, int width, int height);
void drawDecorations(Display *display, Window window, const char *title);
int pointIsInRect(int px, int py, int rx, int ry, int rw, int rh);

// Individual Decorations
void drawCloseButton(Display *display, Window window, GC gc, int x, int y, int w, int h);
void drawCloseButtonDown(Display *display, Window window, GC gc, int x, int y, int w, int h);

#endif
