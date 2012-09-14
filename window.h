#include <X11/Xlib.h>

int checkWindowSize(Display *display, int screen, Window window, int w, int h);
void repositionWindow(Display *display, int screen, Window window, int x, int y, int right, int bottom);
void resizeWindow(Display *display, int screen, Window window, int w, int h);