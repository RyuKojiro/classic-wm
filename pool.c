//
//  pool.c
//  classic-wm
//
//  Created by Daniel Loffgren on 9/13/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>

#include "pool.h"

ManagedWindowPool *createPool(void) {
	ManagedWindowPool *pool = malloc(sizeof(ManagedWindowPool));
	pool->head = NULL;
	pool->active = NULL;
	return pool;
}

void addWindowToPool(Window decorationWindow, Window actualWindow, ManagedWindowPool *pool) {	
	ManagedWindow *mw = malloc(sizeof(ManagedWindow));
	if (!mw) {
		perror("malloc");
		return;
	}
	
	mw->actualWindow = actualWindow;
	mw->decorationWindow = decorationWindow;
	mw->next = pool->head;
	
	pool->head = mw;
}

void activateWindowInPool(Window window, ManagedWindowPool *pool) {
	ManagedWindow *mw = managedWindowForWindow(window, pool);
	pool->active = mw;
}

void removeWindowFromPool(ManagedWindow *managedWindow, ManagedWindowPool *pool) {
	ManagedWindow *last = NULL;
	ManagedWindow *this = pool->head;
	if (this == managedWindow) {
		pool->head = NULL;
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
