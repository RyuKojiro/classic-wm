//
//  pool.h
//  classic-wm
//
//  Created by Daniel Loffgren on 9/13/12.
//  Copyright (c) 2012 Daniel Loffgren. All rights reserved.
//

#ifndef classic_wm_pool_h
#define classic_wm_pool_h

#include <X11/Xlib.h>

struct ManagedWindow_t {
	Window decorationWindow;
	Window actualWindow;
	Window resizer;
	unsigned int last_w;
	unsigned int last_h;
	unsigned int last_x;
	unsigned int last_y;
	struct ManagedWindow_t *next;
};

typedef struct ManagedWindow_t ManagedWindow;

struct ManagedWindowPool_t {
	ManagedWindow *head;
	ManagedWindow *active;
};

typedef struct ManagedWindowPool_t ManagedWindowPool;

ManagedWindowPool *createPool(void);
ManagedWindow *addWindowToPool(Window decorationWindow, Window actualWindow, Window resizer, ManagedWindowPool *pool);
void activateWindowInPool(Window window, ManagedWindowPool *pool);
void removeWindowFromPool(ManagedWindow *managedWindow, ManagedWindowPool *pool);
void destroyPool(ManagedWindowPool *pool);
ManagedWindow *managedWindowForWindow(Window window, ManagedWindowPool *pool);
void printPool(ManagedWindowPool *pool);

#endif
