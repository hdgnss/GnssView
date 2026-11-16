import QtQuick
import QtQuick.Controls

// NeonButton is maintained as a 100% backward-compatible proxy to AppButton.
// Fixes the P0 defect where visualAccent was hardcoded to theme.tabSelectedAccent,
// allowing caller-provided accent (such as theme.ok, theme.bad) to accurately reflect semantic actions.
AppButton {
    id: control
    variant: "secondary"
}
