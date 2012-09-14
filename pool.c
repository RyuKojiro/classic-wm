//
//  pool.c
//  classic-wm
//
//  Created by Daniel Loffgren on 9/13/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pool.h"

ManagedWindowPool *createPool(void) {
	ManagedWindowPool *pool = malloc(sizeof(ManagedWindowPool));
	memset(pool, 0, sizeof *pool);
	return pool;
}

void addWindowToPool(Window decorationWindow, Window actualWindow, ManagedWindowPool *pool) {	
	pool->windows = realloc(pool->windows, (sizeof(ManagedWindow)) * ++pool->len);
	if (!pool->windows) {
		perror("realloc");
		return;
	}
	
	pool->windows[pool->len].decorationWindow = decorationWindow;
	pool->windows[pool->len].actualWindow = actualWindow;
}

void freePool(ManagedWindowPool *pool) {
	free(pool);
}