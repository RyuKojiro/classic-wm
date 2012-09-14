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

typedef struct {
	Window decorationWindow;
	Window actualWindow;
} ManagedWindow;

typedef struct {
	ManagedWindow *windows;
	int len;
} ManagedWindowPool;

#endif
