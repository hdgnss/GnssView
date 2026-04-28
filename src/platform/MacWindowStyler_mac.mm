#include "MacWindowStyler.h"

#ifdef Q_OS_MACOS

#include <QWindow>

#import <Cocoa/Cocoa.h>

namespace hdgnss {

void applyPlatformWindowStyle(QWindow *window) {
    if (!window) {
        return;
    }

    window->create();
    auto viewId = window->winId();
    if (!viewId) {
        return;
    }

    NSView *nsView = reinterpret_cast<NSView *>(viewId);
    NSWindow *nsWindow = nsView.window;
    if (!nsWindow) {
        return;
    }

    // Keep the standard macOS title bar interaction model, but blend the
    // chrome into the app's dark surface instead of using the default white
    // system fill.
    nsWindow.titleVisibility = NSWindowTitleHidden;
    nsWindow.titlebarAppearsTransparent = YES;
    nsWindow.movableByWindowBackground = NO;
    nsWindow.styleMask &= ~NSWindowStyleMaskFullSizeContentView;
    nsWindow.backgroundColor = [NSColor colorWithCalibratedRed:0.02 green:0.04 blue:0.08 alpha:1.0];
    nsWindow.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];

    if (@available(macOS 11.0, *)) {
        nsWindow.toolbarStyle = NSWindowToolbarStyleAutomatic;
        nsWindow.titlebarSeparatorStyle = NSTitlebarSeparatorStyleNone;
    }
}

}  // namespace hdgnss

#endif
