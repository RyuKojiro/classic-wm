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

#ifndef classic_wm_pool_h
#define classic_wm_pool_h

#include <X11/Xlib.h>
#include <X11/extensions/Xdbe.h>
#include <sys/queue.h>

struct ManagedWindow_t {
	Window decorationWindow;
	XdbeBackBuffer decorationBuffer;
	Window actualWindow;
	Window resizer;

	/* The last_ members are for collapsing and maximizing only */
	unsigned int last_w;
	unsigned int last_h;
	unsigned int last_x;
	unsigned int last_y;

	/* The minimum size hints are cached for performance */
	unsigned int min_w;
	unsigned int min_h;

	SLIST_ENTRY(ManagedWindow_t) entries;
	char *title;

	int collapsed;
};

typedef struct ManagedWindow_t ManagedWindow;

struct ManagedWindowPool_t {
	SLIST_HEAD(windowlist, ManagedWindow_t) windows;
	ManagedWindow *active;
};

typedef struct ManagedWindowPool_t ManagedWindowPool;

ManagedWindowPool *createPool(void);
ManagedWindow *addWindowToPool(Display *display, Window decorationWindow, Window actualWindow, Window resizer, ManagedWindowPool *pool);
void removeWindowFromPool(Display *display, ManagedWindow *managedWindow, ManagedWindowPool *pool);
void destroyPool(ManagedWindowPool *pool);
ManagedWindow *managedWindowForWindow(Display *display, Window window, ManagedWindowPool *pool);
void printPool(ManagedWindowPool *pool);
void updateWindowTitle(Display *display, ManagedWindow *mw);

#endif
