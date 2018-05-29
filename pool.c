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

#include <stdlib.h>
#include <assert.h>

#include "pool.h"

ManagedWindowPool *createPool(void) {
	ManagedWindowPool *pool = calloc(1, sizeof(ManagedWindowPool));
	assert(pool);
	return pool;
}

void updateWindowTitle(Display *display, ManagedWindow *mw) {
	if (mw->title) {
		XFree(mw->title);
		mw->title = NULL;
	}
	XFetchName(display, mw->actualWindow, &(mw->title));
}

ManagedWindow *addWindowToPool(Display *display, Window decorationWindow, Window actualWindow, Window resizer, ManagedWindowPool *pool) {
	ManagedWindow *mw = calloc(1, sizeof(ManagedWindow));
	assert(mw);

	mw->resizer = resizer;
	mw->actualWindow = actualWindow;
	mw->decorationWindow = decorationWindow;
	mw->decorationBuffer = XdbeAllocateBackBufferName(display, decorationWindow, XdbeCopied);
	updateWindowTitle(display, mw);

	SLIST_INSERT_HEAD(&pool->windows, mw, entries);

	return mw;
}

void removeWindowFromPool(Display *display, ManagedWindow *managedWindow, ManagedWindowPool *pool) {
	(void)display;
	SLIST_REMOVE(&pool->windows, managedWindow, ManagedWindow_t, entries);

	/* FIXME: I need to be dealloced before the decoration window */
	/*
	XdbeDeallocateBackBufferName(display, this->decorationBuffer);
	*/
	XFree(managedWindow->title);
	free(managedWindow);
}

ManagedWindow *managedWindowForWindow(Display *display, Window window, ManagedWindowPool *pool) {
	ManagedWindow *this;
	SLIST_FOREACH(this, &pool->windows, entries) {
		if (this->decorationWindow == window || this->actualWindow == window) {
			return this;
		}

		unsigned int nchildren;
		Window *children;
		Window parent;
		Window root;
		XQueryTree(display, this->actualWindow, &root, &parent, &children, &nchildren);
		for (int i = 0; i < nchildren; i++) {
			if (children[i] == window) {
				return this;
			}
		}
	}
	return NULL;
}

void destroyPool(ManagedWindowPool *pool) {
	ManagedWindow *this;
	while (!SLIST_EMPTY(&pool->windows)) {
		this = SLIST_FIRST(&pool->windows);
		SLIST_REMOVE_HEAD(&pool->windows, entries);
		free(this);
	}
	free(pool);
}

#ifdef DEBUG
#include <stdio.h>

void printPool(ManagedWindowPool *pool) {
	fprintf(stderr, "ManagedWindowPool %p {\n", pool);
	ManagedWindow *this;
	SLIST_FOREACH(this, &pool->windows, entries) {
		fprintf(stderr, "\tManagedWindow %p {\n", this);
		fprintf(stderr, "\t\tdecorationWindow = %lu,\n", this->decorationWindow);
		fprintf(stderr, "\t\tactualWindow = %lu,\n", this->actualWindow);
		fprintf(stderr, "\t\tresizer = %lu,\n", this->resizer);
		fprintf(stderr, "\t}\n");
	}
	fprintf(stderr, "}\n");
}
#endif
