import QtQuick

QtObject {
    readonly property color bgStart: "#060a13"
    readonly property color bgEnd: "#02050b"
    readonly property color panelBase: "#07101d"
    readonly property color panelOverlay: "#07101d"
    readonly property color border: "#3a4f78"
    readonly property color borderStrong: "#4d668f"
    readonly property color accent: "#1de3ff"
    readonly property color accentStrong: "#45a6ff"
    readonly property color textPrimary: "#eaf2ff"
    readonly property color textSecondary: "#95accf"
    readonly property color warning: "#ffb24d"
    readonly property color ok: "#39ff95"
    readonly property color bad: "#ff5f75"
    readonly property color shellBorder: "#244670"
    readonly property color surface0: "#07101d"
    readonly property color surface1: "#07101d"
    readonly property color surface2: "#0a1322"
    readonly property color surface3: "#101a2a"
    readonly property color surface4: "#16355a"
    readonly property color panelHeader: Qt.rgba(1, 1, 1, 0.02)
    readonly property color panelBorder: "#243b56"
    readonly property color panelBorderStrong: "#2b4363"
    readonly property color innerBorder: Qt.rgba(0.17, 0.31, 0.47, 0.45)
    readonly property color inputBg: "#0b1322"
    readonly property color inputBorder: "#2b4363"
    readonly property color inputBorderFocus: "#3ddcff"
    readonly property color tabBarBg: "#07101d"
    readonly property color tabBarBorder: "#20344d"
    readonly property color tabBarGlow: "#1a2b42"
    readonly property color tabButtonBg: "#07101d"
    readonly property color tabButtonHover: "#0d1a2c"
    readonly property color tabButtonActive: "#16355a"
    readonly property color tabSelectedAccent: accentStrong
    readonly property color hoverText: "#beddff"
    readonly property color divider: "#35506d"
    readonly property color lineSoft: Qt.rgba(1, 1, 1, 0.08)
    readonly property color lineStrong: Qt.rgba(1, 1, 1, 0.22)
    readonly property color tooltipBg: "#08111d"
    readonly property color tooltipText: "#eef6ff"
    readonly property color tooltipDetail: "#9ab1d2"
    readonly property color chartBg: "#07101d"
    readonly property color chartGridMajor: "#406284"
    readonly property color chartGridMinor: "#24384f"
    readonly property color chartAxis: "#294562"
    readonly property color chartAxisMuted: "#203247"
    readonly property color chartLabel: "#8ea8cc"
    readonly property color chartLabelMuted: "#607da3"
    readonly property color chartPeak: "#8cd8ff"
    readonly property color chartPeakText: "#dff5ff"
    readonly property color visibleHint: Qt.rgba(1, 1, 1, 0.16)
    readonly property color rawRx: "#2de59c"
    readonly property color rawTx: "#ff8b6b"
    readonly property color rawRxText: "#57f8b6"
    readonly property color rawTxText: "#ff9f83"
    readonly property color rawRxRow: Qt.rgba(0.18, 0.86, 0.65, 0.10)
    readonly property color rawTxRow: Qt.rgba(1.0, 0.48, 0.31, 0.13)
    readonly property color badgeNmeaBg: "#12374a"
    readonly property color badgeNmeaBorder: "#3fe6ff"
    readonly property color badgeNmeaText: "#b8f8ff"
    readonly property color badgeBinBg: "#2a213a"
    readonly property color badgeBinBorder: "#8aa0ff"
    readonly property color badgeBinText: "#d7ddff"
    readonly property color rawTextNmea: "#c8f7ff"
    readonly property color rawTextBin: "#9fd6ff"
    readonly property color shellGlass: Qt.rgba(0.05, 0.09, 0.15, 0.58)
    readonly property color shellOutline: Qt.rgba(0.36, 0.55, 0.82, 0.28)
    readonly property color backgroundGrid: "#263a59"
    readonly property color caption: "#6f8eb2"
    readonly property color constellationGpsUsed: "#6ef5ff"
    readonly property color constellationGpsVisible: "#4b7280"
    readonly property color constellationGloUsed: "#79ff8f"
    readonly property color constellationGloVisible: "#526f58"
    readonly property color constellationGalUsed: "#ffd06e"
    readonly property color constellationGalVisible: "#7e6b4b"
    readonly property color constellationBdsUsed: "#ff84ff"
    readonly property color constellationBdsVisible: "#715372"
    readonly property color constellationQzsUsed: "#8fb5ff"
    readonly property color constellationQzsVisible: "#50617d"
    readonly property color constellationSbsUsed: "#7dffa8"
    readonly property color constellationSbsVisible: "#537262"
    readonly property color constellationNavicUsed: "#ff9f73"
    readonly property color constellationNavicVisible: "#7a5b4e"
    readonly property color constellationUnknownUsed: "#9fb8ff"
    readonly property color constellationUnknownVisible: "#4f5d7d"
    readonly property int shellRadius: 6
    readonly property int radius: 8
    readonly property int panelRadius: 8
    readonly property int cardRadius: 6
    readonly property int panelContentMargin: 8
    readonly property int controlHeight: 30
    readonly property int controlRadius: 6
    readonly property int pillRadius: 6
    readonly property int badgeRadius: 9
    readonly property int microRadius: 4
    readonly property int tinyRadius: 2
    readonly property int titleSize: 14
    readonly property int bodySize: 11
    readonly property int labelSize: 10
    readonly property int monoSize: 10
    readonly property int compactSpacing: 6
    readonly property int sectionSpacing: 8
    readonly property string titleFont: "Avenir Next"
    readonly property string bodyFont: typeof uiBodyFontFamily !== "undefined" ? uiBodyFontFamily : "Helvetica Neue"
    readonly property string monoFont: typeof uiMonoFontFamily !== "undefined" ? uiMonoFontFamily : "Menlo"

    function constellationColor(constellationName, used) {
        if (constellationName === "GPS") return used ? constellationGpsUsed : constellationGpsVisible
        if (constellationName === "GLONASS") return used ? constellationGloUsed : constellationGloVisible
        if (constellationName === "GALILEO") return used ? constellationGalUsed : constellationGalVisible
        if (constellationName === "BEIDOU") return used ? constellationBdsUsed : constellationBdsVisible
        if (constellationName === "QZSS") return used ? constellationQzsUsed : constellationQzsVisible
        if (constellationName === "SBAS") return used ? constellationSbsUsed : constellationSbsVisible
        if (constellationName === "NAVIC" || constellationName === "IRNSS") return used ? constellationNavicUsed : constellationNavicVisible
        return used ? constellationUnknownUsed : constellationUnknownVisible
    }

    function signalColor(constellationName, used, signalId) {
        // Always use the constellation colour so bars are grouped by constellation
        // regardless of which signal frequency band they were observed on.
        // The small cap drawn on top of each bar (alternateSignal indicator) already
        // distinguishes multi-frequency signals visually.
        return constellationColor(constellationName, used)
    }

    // Returns a distinct colour for each signalId, used by the small cap on top of
    // signal bars.  When the same satellite appears more than once in a band tab
    // (e.g. both L1 C/A and L1C in the L1/B1/L1C tab), the caps show different
    // colours so the user can tell the two bars apart at a glance.
    function signalCapColor(signalId) {
        var palette = {
            1:  "#ffb454",
            2:  "#7ed1ff",
            3:  "#8effc1",
            4:  "#ff8f70",
            5:  "#ff78d7",
            6:  "#b59cff",
            7:  "#7dffa8",
            8:  "#ffd86e",
            9:  "#9f8cff",
            10: "#ff6f91",
            11: "#66f0ff",
            12: "#b9ff66"
        }
        return palette[signalId] || tooltipText
    }
}
