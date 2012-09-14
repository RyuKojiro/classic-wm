#include <stdio.h>
#include <unistd.h> // usleep

#include "window.h"

int checkWindowSize(Display *display, int screen, Window window, int w, int h) {
	XWindowAttributes attributes;
	
	if (XGetWindowAttributes(display, window, &attributes) == 0) {
		perror("XGetWindowAttributes");
		return 0;
	}
	
	if (attributes.width == w && attributes.height == h) {
		return 1;
	}
	
	return 0;
}

void repositionWindow(Display *display, int screen, Window window, int x, int y, int right, int bottom) {
	XWindowChanges values;
	unsigned int value_mask;
	
	if (right || bottom) {
		XWindowAttributes win_attr, frame_attr;
		Window wmframe;
		
		if (XGetWindowAttributes(display, window, &win_attr) == 0) {
			fprintf(stderr, "kiosk-wm: XGetWindowAttributes failed\n");
			return;
		}
		
		/* find our window manager frame, if any */
		for (wmframe = window; ; ) {
			Status status;
			Window wroot, parent;
			Window *childlist;
			unsigned int ujunk;
			
			status = XQueryTree(display, wmframe,
								&wroot, &parent, &childlist, &ujunk);
			if (parent == wroot || !parent || !status)
				break;
			wmframe = parent;
			if (status && childlist)
				XFree((char *) childlist);
		}
		
		if (right)
			x += DisplayWidth(display, screen) -
			win_attr.width -
			win_attr.border_width;
		
		if (bottom)
			y += DisplayHeight(display, screen) -
			win_attr.height -
			win_attr.border_width;
	}
	
	values.x = x;
	values.y = y;
	value_mask = CWX | CWY;
	
	if (XReconfigureWMWindow(display, window, screen, value_mask, &values) == 0) {
		fprintf(stderr, "kiosk-wm: XReconfigureWMWindow failed\n");
	}
}

void resizeWindow(Display *display, int screen, Window window, int w, int h) {
    XWindowChanges values;
    unsigned int value_mask;
    int try;
    int nw, nh;
	
    values.width = w;
    values.height = h;
    value_mask = CWWidth | CWHeight;
	
    for (try = 0; try < 2; try++) {
		if (XReconfigureWMWindow(display, window, screen, value_mask, &values) == 0)
			fprintf(stderr, "kiosk-wm: XReconfigureWMWindow failed\n");
		
		if (checkWindowSize(display, screen, window, w, h)) {
			return;
		}
		
		usleep(1000);
		if (checkWindowSize(display, screen, window, w, h)) {
			return;
		}
		
		usleep(4000);
		if (checkWindowSize(display, screen, window, w, h)) {
			return;
		}
    }
	
    values.width += values.width - nw;
    values.height += values.height - nh;
    if (XReconfigureWMWindow(display, window, screen, value_mask, &values) == 0)
		fprintf(stderr, "kiosk-wm: XReconfigureWMWindow failed\n");
}
