#include "MacWindowStyler.h"

#ifdef Q_OS_MACOS

#include <QWindow>

#import <Cocoa/Cocoa.h>

namespace hdgnss {

void applyPlatformWindowStyle(QWindow *window, bool lightTheme) {
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
    // chrome into the selected application theme surface.
    nsWindow.titleVisibility = NSWindowTitleHidden;
    nsWindow.titlebarAppearsTransparent = YES;
    nsWindow.movableByWindowBackground = NO;
    nsWindow.styleMask &= ~NSWindowStyleMaskFullSizeContentView;
    nsWindow.backgroundColor = lightTheme
        ? [NSColor colorWithCalibratedRed:0.93 green:0.97 blue:1.0 alpha:1.0]
        : [NSColor colorWithCalibratedRed:0.02 green:0.04 blue:0.08 alpha:1.0];
    nsWindow.appearance = [NSAppearance appearanceNamed:(lightTheme ? NSAppearanceNameAqua : NSAppearanceNameDarkAqua)];

    if (@available(macOS 11.0, *)) {
        nsWindow.toolbarStyle = NSWindowToolbarStyleAutomatic;
        nsWindow.titlebarSeparatorStyle = NSTitlebarSeparatorStyleNone;
    }
}

}  // namespace hdgnss

#endif
