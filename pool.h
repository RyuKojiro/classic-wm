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

struct ManagedWindow_s {
	Window decorationWindow;
	Window actualWindow;
};

typedef struct ManagedWindow_s ManagedWindow;

struct ManagedWindowPool_s {
	ManagedWindow *windows;
	int len;
};

typedef struct ManagedWindowPool_s ManagedWindowPool;

ManagedWindowPool *createPool(void);
void addWindowToPool(Window decorationWindow, Window actualWindow, ManagedWindowPool *pool);
void removeWindowFromPool(ManagedWindowPool *managedWindow, ManagedWindowPool *pool);
void freePool(ManagedWindowPool *pool);
ManagedWindow *managedWindowForWindow(Window window, ManagedWindowPool *pool);

#endif
