/**
 * MacOverlay.mm
 * Objective-C++ bridge: pins an NSWindow to kCGDesktopWindowLevel
 * so it renders above the macOS wallpaper but below all normal windows.
 *
 * Called from OverlayWidget::applyMacOSDesktopLevel().
 */

#import <AppKit/AppKit.h>

#ifdef __cplusplus
extern "C" {
#endif

void setWindowToDesktopLevel(unsigned long long winId)
{
    // Qt's winId() on macOS returns an NSView pointer cast to WId
    NSView *view = (__bridge NSView *)(void *)winId;
    if (!view) return;

    NSWindow *window = [view window];
    if (!window) return;

    // kCGDesktopWindowLevel (-2147483623) — renders above wallpaper, below all apps
    [window setLevel:kCGDesktopWindowLevel];

    // Transparent, non-activating
    [window setOpaque:NO];
    [window setBackgroundColor:[NSColor clearColor]];
    [window setIgnoresMouseEvents:YES];
    [window setCollectionBehavior:
        NSWindowCollectionBehaviorCanJoinAllSpaces |
        NSWindowCollectionBehaviorStationary |
        NSWindowCollectionBehaviorIgnoresCycle];
}

#ifdef __cplusplus
}
#endif
