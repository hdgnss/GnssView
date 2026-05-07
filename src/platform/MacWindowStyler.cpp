#include "MacWindowStyler.h"

#ifdef Q_OS_WIN

#include <QtGlobal>

#define NOMINMAX
#include <windows.h>
#include <dwmapi.h>

namespace hdgnss {

namespace {

constexpr DWORD kDwmUseImmersiveDarkMode20 = 20;
constexpr DWORD kDwmUseImmersiveDarkMode19 = 19;
constexpr DWORD kDwmBorderColor = 34;
constexpr DWORD kDwmCaptionColor = 35;
constexpr DWORD kDwmTextColor = 36;

COLORREF rgbToColorRef(int red, int green, int blue) {
    return RGB(red, green, blue);
}

void setDwmAttributeIfAvailable(HWND hwnd, DWORD attribute, const void *value, DWORD size) {
    if (!hwnd) {
        return;
    }
    ::DwmSetWindowAttribute(hwnd, attribute, value, size);
}

}  // namespace

void applyPlatformWindowStyle(QWindow *window, bool lightTheme) {
    if (!window) {
        return;
    }

    window->create();
    const auto hwnd = reinterpret_cast<HWND>(window->winId());
    if (!hwnd) {
        return;
    }

    const BOOL enabled = lightTheme ? FALSE : TRUE;
    const COLORREF captionColor = lightTheme ? rgbToColorRef(247, 251, 255) : rgbToColorRef(7, 16, 29);
    const COLORREF borderColor = lightTheme ? rgbToColorRef(154, 180, 199) : rgbToColorRef(22, 53, 90);
    const COLORREF textColor = lightTheme ? rgbToColorRef(16, 34, 54) : rgbToColorRef(234, 242, 255);

    setDwmAttributeIfAvailable(hwnd, kDwmUseImmersiveDarkMode20, &enabled, sizeof(enabled));
    setDwmAttributeIfAvailable(hwnd, kDwmUseImmersiveDarkMode19, &enabled, sizeof(enabled));
    setDwmAttributeIfAvailable(hwnd, kDwmCaptionColor, &captionColor, sizeof(captionColor));
    setDwmAttributeIfAvailable(hwnd, kDwmBorderColor, &borderColor, sizeof(borderColor));
    setDwmAttributeIfAvailable(hwnd, kDwmTextColor, &textColor, sizeof(textColor));
}

}  // namespace hdgnss

#else

namespace hdgnss {

void applyPlatformWindowStyle(QWindow *window, bool lightTheme) {
    Q_UNUSED(window);
    Q_UNUSED(lightTheme);
}

}  // namespace hdgnss

#endif
