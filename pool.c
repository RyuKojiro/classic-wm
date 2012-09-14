//
//  pool.c
//  classic-wm
//
//  Created by Daniel Loffgren on 9/13/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <stdio.h>
#include "pool.h"

ManagedWindowPool *createPool() {
	ManagedWindowPool *pool = malloc(sizeof ManagedWindowPool);
	memset(pool, 0, sizeof pool);
	return pool;
}

void addWindowToPool(Window decorationWindow, Window actualWindow, ManagedWindowPool *pool) {
	ManagedWindow *mw = malloc(sizeof ManagedWindow);
	if (!mw) {
		perror("malloc");
		return;
	}
	
	pool = realloc(pool, (sizeof pool) * ++len);
	if (!pool) {
		perror("realloc");
		return;
	}
	
	pool.windows[len].decorationWindow = decorationWindow;
	pool.windows[len].actualWindow = actualWindow;
}

void freePool(ManagedWindowPool *pool) {
	for (int i = 0; i < pool.len; i++) {
		free(pool.windows[i]);
	}
	free(pool);
}