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

#include <stdio.h>
#include <stdlib.h>

#include "pool.h"

ManagedWindowPool *createPool(void) {
	ManagedWindowPool *pool = malloc(sizeof(ManagedWindowPool));
	pool->head = NULL;
	pool->active = NULL;
	return pool;
}

ManagedWindow *addWindowToPool(Display *display, Window decorationWindow, Window actualWindow, Window resizer, ManagedWindowPool *pool) {
	ManagedWindow *mw = malloc(sizeof(ManagedWindow));
	if (!mw) {
		perror("malloc");
		return NULL;
	}
	
	mw->resizer = resizer;
	mw->actualWindow = actualWindow;
	mw->decorationWindow = decorationWindow;
	mw->next = pool->head;
	mw->last_h = 0;
	mw->last_w = 0;
	mw->last_x = 0;
	mw->last_y = 0;

	mw->decorationBuffer = XdbeAllocateBackBufferName(display, decorationWindow, XdbeCopied);

	pool->head = mw;
	
	return mw;
}

void activateWindowInPool(Window window, ManagedWindowPool *pool) {
	ManagedWindow *mw = managedWindowForWindow(window, pool);
	pool->active = mw;
}

void removeWindowFromPool(Display *display, ManagedWindow *managedWindow, ManagedWindowPool *pool) {
	ManagedWindow *last = NULL;
	ManagedWindow *this = pool->head;
	if (this == managedWindow) {
		pool->head = this->next;
		XdbeDeallocateBackBufferName(display, this->decorationBuffer);
		free(this);
		return;
	}
	while (this->next) {
		last = this;
		this = this->next;
		if (this == managedWindow) {
			last->next = this->next;
			free(this);
			return;
		}
	}
}

ManagedWindow *managedWindowForWindow(Window window, ManagedWindowPool *pool) {
	ManagedWindow *this = pool->head;
	while (this) {
		if (this->decorationWindow == window || this->actualWindow == window) {
			return this;
		}
		this = this->next;
	}
	return NULL;
}

void destroyPool(ManagedWindowPool *pool) {
	ManagedWindow *last;
	ManagedWindow *this = pool->head;
	while (this->next) {
		last = this;
		this = this->next;
		free(last);
	}
	free(this);
}

void printPool(ManagedWindowPool *pool) {
	ManagedWindow *this = pool->head;
	fprintf(stderr, "ManagedWindowPool %p {\n", pool);
	if (this) {
		fprintf(stderr, "\tManagedWindow %p {\n", this);
		fprintf(stderr, "\t\tdecorationWindow = %lu,\n", this->decorationWindow);
		fprintf(stderr, "\t\tactualWindow = %lu,\n", this->actualWindow);
		fprintf(stderr, "\t}\n");
		while (this->next) {
			this = this->next;
			fprintf(stderr, "\tManagedWindow %p {\n", this);
			fprintf(stderr, "\t\tdecorationWindow = %lu,\n", this->decorationWindow);
			fprintf(stderr, "\t\tactualWindow = %lu,\n", this->actualWindow);
			fprintf(stderr, "\t}\n");
		}
	}
	fprintf(stderr, "}\n");
}
