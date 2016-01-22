# classic-wm
![Screenshot](https://gist.github.com/RyuKojiro/909e3e29d83470d073a6/raw/541421d1b8e08bb4f62220e26a650275feb0e551/classic-wm.png)

# Installation
1. `make`
2. Install Chicago bitmap font.
  * You can convert the original font to a BDF via fontforge, and should end up with a font identifying as `-FontForge-Chicago-Medium-R-Normal--12-120-75-75-P-78-MacRoman-0` (That's what classic-wm looks for.)
3. Run `classic-wm` as your new window manager

# Collapsing
You can enable the single-click collapse button shown below by uncommenting the `COLLAPSE_BUTTON_ENABLED` definition in `decorations.h`.

![Collapse Button](https://gist.githubusercontent.com/RyuKojiro/c24128fe6e30e6d0eb83/raw/fbe93d6c777107506aa1babba382b1dd42c02151/collapse-button.png)

Even without the collapse button enabled, all windows will support collapsing by double clicking on any portion of the titlebar that isn't a button.
Collapsing a window does not change its width, but reduces it to consume only as much space as its title bar.

![Collapsed Window](https://gist.githubusercontent.com/RyuKojiro/c24128fe6e30e6d0eb83/raw/fbe93d6c777107506aa1babba382b1dd42c02151/collapsed.png)
