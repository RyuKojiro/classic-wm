//
//  decorations.h
//  
//
//  Created by Daniel Loffgren on 9/10/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef _decorations_h
#define _decorations_h

Window decorateWindow(Display *display, Window window, Window root, int x, int y, int width, int height);
void drawDecorations(Display *display, Window window, const char *title);

#endif
