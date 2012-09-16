//
//  pool.h
//  classic-wm
//
//  Created by Daniel Loffgren on 9/13/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef classic_wm_pool_h
#define classic_wm_pool_h

#include <X11/Xlib.h>

struct ManagedWindow_t {
	Window decorationWindow;
	Window actualWindow;
	struct ManagedWindow_t *next;
};

typedef struct ManagedWindow_t ManagedWindow;

struct ManagedWindowPool_t {
	ManagedWindow *head;
	ManagedWindow *active;
};

typedef struct ManagedWindowPool_t ManagedWindowPool;

ManagedWindowPool *createPool(void);
void addWindowToPool(Window decorationWindow, Window actualWindow, ManagedWindowPool *pool);
void activateWindowInPool(Window window, ManagedWindowPool *pool);
void removeWindowFromPool(ManagedWindow *managedWindow, ManagedWindowPool *pool);
void destroyPool(ManagedWindowPool *pool);
ManagedWindow *managedWindowForWindow(Window window, ManagedWindowPool *pool);
void printPool(ManagedWindowPool *pool);

#endif
