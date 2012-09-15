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
	pool->windows = NULL;
	pool->len = 0;
	return pool;
}

void addWindowToPool(Window decorationWindow, Window actualWindow, ManagedWindowPool *pool) {	
	pool->windows = realloc(pool->windows, (sizeof(ManagedWindow)) * ++pool->len);
	if (!pool->windows) {
		perror("realloc");
		return;
	}
	
	pool->windows[pool->len - 1].decorationWindow = decorationWindow;
	pool->windows[pool->len - 1].actualWindow = actualWindow;
}

ManagedWindow *managedWindowForWindow(Window window, ManagedWindowPool *pool) {
	for (int i = 0; i < pool->len; i++) {
		if (window == pool->windows[i].decorationWindow || window == pool->windows[i].actualWindow) {
			return &(pool->windows[i]);
		}
	}
	return NULL;
}

void freePool(ManagedWindowPool *pool) {
	free(pool);
}