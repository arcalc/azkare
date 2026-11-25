#import <AppKit/AppKit.h>
#include "MacOverlay.h"

void azkare_setWindowToDesktopLevel(void *nsViewPtr)
{
    NSView *view = (__bridge NSView *)nsViewPtr;
    NSWindow *window = view ? [view window] : nil;
    if (!window)
        return;
    [window setLevel:kCGDesktopWindowLevel];   // above the wallpaper, below normal windows
    [window setOpaque:NO];
    [window setBackgroundColor:[NSColor clearColor]];
    [window setIgnoresMouseEvents:YES];
    [window setCollectionBehavior:NSWindowCollectionBehaviorCanJoinAllSpaces |
                                  NSWindowCollectionBehaviorStationary |
                                  NSWindowCollectionBehaviorIgnoresCycle];
}
