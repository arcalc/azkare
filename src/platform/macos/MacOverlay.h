#pragma once
#ifdef __cplusplus
extern "C" {
#endif
/** Pins the NSWindow behind the given NSView pointer to the desktop window level. */
void azkare_setWindowToDesktopLevel(void *nsView);
#ifdef __cplusplus
}
#endif
