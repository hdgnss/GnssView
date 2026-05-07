import QtQuick

QtObject {
    readonly property bool lightMode: typeof appSettings !== "undefined" && appSettings && appSettings.uiTheme === "light"
    readonly property color bgStart: token("#06090d", "#eef4fb")
    readonly property color bgMid: token("#03070b", "#e7f0f8")
    readonly property color bgEnd: token("#010307", "#dce8f3")
    readonly property color panelBase: token("#0a1117", "#f7fbff")
    readonly property color panelOverlay: token("#111b24", "#ffffff")
    readonly property color border: token("#334757", "#b6c9d9")
    readonly property color borderStrong: token("#5d7284", "#7898b3")
    readonly property color accent: token("#45a6ff", "#0b76d8")
    readonly property color accentStrong: token("#45a6ff", "#005fb8")
    readonly property color accentSoft: token("#9bd8ff", "#d5ebff")
    readonly property color textPrimary: token("#eaf2ff", "#102236")
    readonly property color textSecondary: token("#9eb2c7", "#4f6578")
    readonly property color warning: token("#f2b35f", "#b46b00")
    readonly property color ok: token("#42d77d", "#147a48")
    readonly property color bad: token("#ff6578", "#b4233d")
    readonly property color semanticPosition: accentStrong
    readonly property color semanticTime: token("#73cfff", "#087da5")
    readonly property color semanticMotion: warning
    readonly property color semanticPrecision: token("#8aa0ff", "#5472d3")
    readonly property color semanticCorrection: token("#9bf6e0", "#0b8f75")
    readonly property color semanticPower: token("#ffb86f", "#a95f00")
    readonly property color semanticRf: token("#ff86e9", "#ba4bb1")
    readonly property color semanticData: accentStrong
    readonly property color semanticContext: token("#9bd8ff", "#2470a8")
    readonly property color semanticNeutral: textSecondary
    readonly property color semanticHealthGood: ok
    readonly property color semanticHealthWarn: warning
    readonly property color semanticHealthBad: bad
    readonly property color shellBorder: token("#284153", "#9eb7ca")
    readonly property color surface0: token("#080d12", "#f1f7fc")
    readonly property color surface1: token("#0d151b", "#ffffff")
    readonly property color surface2: token("#111b23", "#eaf3fa")
    readonly property color surface3: token("#17252f", "#dcebf6")
    readonly property color surface4: token("#213947", "#c5dced")
    readonly property color panelHeader: token("#111b24", "#f8fbfe")
    readonly property color panelHeaderBottom: token("#0b1219", "#eaf3fb")
    readonly property color panelBorder: token("#223948", "#c7d7e3")
    readonly property color panelBorderStrong: token("#315064", "#9ab4c7")
    readonly property color innerBorder: lightMode ? Qt.rgba(0.48, 0.62, 0.74, 0.42) : Qt.rgba(0.33, 0.52, 0.65, 0.36)
    readonly property color inputBg: token("#070c11", "#ffffff")
    readonly property color inputBorder: token("#294354", "#adc2d3")
    readonly property color inputBorderFocus: token("#45a6ff", "#0b76d8")
    readonly property color tabBarBg: token("#070c11", "#e9f2f8")
    readonly property color tabBarBorder: token("#263d4d", "#b7cad8")
    readonly property color tabBarGlow: token("#163240", "#d0e7f9")
    readonly property color tabButtonBg: token("#0a1016", "#eef5fa")
    readonly property color tabButtonHover: token("#12202a", "#dcecf8")
    readonly property color tabButtonActive: token("#153743", "#cfe7f8")
    readonly property color tabSelectedAccent: accentStrong
    readonly property color hoverText: token("#d3f9ff", "#044c8f")
    readonly property color divider: token("#334c5d", "#b6c9d8")
    readonly property color lineSoft: lightMode ? Qt.rgba(0.10, 0.22, 0.34, 0.08) : Qt.rgba(1, 1, 1, 0.08)
    readonly property color lineStrong: lightMode ? Qt.rgba(0.10, 0.22, 0.34, 0.22) : Qt.rgba(1, 1, 1, 0.22)
    readonly property color tooltipBg: token("#081016", "#ffffff")
    readonly property color tooltipText: token("#eef6ff", "#102236")
    readonly property color tooltipDetail: token("#a7bacd", "#536a7f")
    readonly property color chartBg: token("#071016", "#f8fbfe")
    readonly property color chartGridMajor: token("#355466", "#abc2d4")
    readonly property color chartGridMinor: token("#1c303d", "#d7e5ef")
    readonly property color chartAxis: token("#2a4657", "#6f91aa")
    readonly property color chartAxisMuted: token("#1b303e", "#c4d6e3")
    readonly property color chartLabel: token("#9ab4c8", "#36556d")
    readonly property color chartLabelMuted: token("#678295", "#7891a5")
    readonly property color chartPeak: token("#9bf6e0", "#0b76d8")
    readonly property color chartPeakText: token("#dff5ff", "#0f3e68")
    readonly property color visibleHint: lightMode ? Qt.rgba(0.07, 0.18, 0.28, 0.18) : Qt.rgba(1, 1, 1, 0.16)
    readonly property color rawRx: token("#43eaa7", "#168a61")
    readonly property color rawTx: token("#ff9b70", "#c45b2d")
    readonly property color rawRxText: token("#78f6bf", "#126f51")
    readonly property color rawTxText: token("#ffad90", "#9a4522")
    readonly property color rawRxRow: lightMode ? Qt.rgba(0.07, 0.55, 0.38, 0.10) : Qt.rgba(0.20, 0.86, 0.62, 0.10)
    readonly property color rawTxRow: lightMode ? Qt.rgba(0.78, 0.30, 0.14, 0.11) : Qt.rgba(1.0, 0.55, 0.34, 0.13)
    readonly property color badgeNmeaBg: token("#102f35", "#dff7fb")
    readonly property color badgeNmeaBorder: token("#45a6ff", "#0b76d8")
    readonly property color badgeNmeaText: token("#bdfbef", "#0f4967")
    readonly property color badgeBinBg: token("#25263a", "#edf0ff")
    readonly property color badgeBinBorder: token("#8aa0ff", "#5472d3")
    readonly property color badgeBinText: token("#d7ddff", "#273b8d")
    readonly property color rawTextNmea: token("#c8f7ff", "#164a60")
    readonly property color rawTextBin: token("#9fd6ff", "#185b91")
    readonly property color shellGlass: lightMode ? Qt.rgba(0.93, 0.97, 1.0, 0.78) : Qt.rgba(0.04, 0.07, 0.09, 0.70)
    readonly property color shellOutline: lightMode ? Qt.rgba(0.45, 0.61, 0.74, 0.34) : Qt.rgba(0.39, 0.62, 0.74, 0.25)
    readonly property color overlayScrim: lightMode ? Qt.rgba(0.64, 0.73, 0.82, 0.44) : Qt.rgba(0.01, 0.03, 0.08, 0.68)
    readonly property color backgroundGrid: token("#203644", "#c6d8e6")
    readonly property color caption: token("#758fa4", "#697f92")
    readonly property color constellationGpsUsed: token("#72f2ff", "#0089b7")
    readonly property color constellationGpsVisible: token("#466d75", "#94c3d0")
    readonly property color constellationGloUsed: token("#7df59b", "#1f8f48")
    readonly property color constellationGloVisible: token("#4f7059", "#9fc8aa")
    readonly property color constellationGalUsed: token("#ffd16f", "#b77900")
    readonly property color constellationGalVisible: token("#7e6b4b", "#d3bc7e")
    readonly property color constellationBdsUsed: token("#ff86e9", "#ba4bb1")
    readonly property color constellationBdsVisible: token("#74536f", "#d0a2cc")
    readonly property color constellationQzsUsed: token("#93b8ff", "#466fd0")
    readonly property color constellationQzsVisible: token("#4e607a", "#a7b9e0")
    readonly property color constellationSbsUsed: token("#83f8ad", "#2e985d")
    readonly property color constellationSbsVisible: token("#527263", "#a4c9b3")
    readonly property color constellationNavicUsed: token("#ff9f73", "#c05f31")
    readonly property color constellationNavicVisible: token("#785b4e", "#d6aa94")
    readonly property color constellationUnknownUsed: token("#9fb8ff", "#5672c0")
    readonly property color constellationUnknownVisible: token("#4f5d7d", "#a6b2d5")
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

    function token(darkValue, lightValue) {
        return lightMode ? lightValue : darkValue
    }

    function semanticPalette() {
        return {
            position: semanticPosition,
            time: semanticTime,
            motion: semanticMotion,
            precision: semanticPrecision,
            correction: semanticCorrection,
            power: semanticPower,
            rf: semanticRf,
            data: semanticData,
            context: semanticContext,
            neutral: semanticNeutral,
            healthGood: semanticHealthGood,
            healthWarn: semanticHealthWarn,
            healthBad: semanticHealthBad
        }
    }

    function semanticText(value) {
        if (value === undefined || value === null) {
            return ""
        }
        return String(value).trim()
    }

    function numericValue(value) {
        var text = semanticText(value)
        if (text.length === 0 || text === "--") {
            return Number.NaN
        }
        var match = text.match(/-?\d+(?:\.\d+)?/)
        return match ? Number(match[0]) : Number.NaN
    }

    function numericValues(value) {
        var text = semanticText(value)
        if (text.length === 0 || text === "--") {
            return []
        }
        var matches = text.match(/-?\d+(?:\.\d+)?/g)
        if (!matches) {
            return []
        }
        var values = []
        for (var i = 0; i < matches.length; ++i) {
            values.push(Number(matches[i]))
        }
        return values
    }

    function fixQualityColor(value) {
        var quality = numericValue(value)
        if (isNaN(quality)) return semanticNeutral
        return quality > 0 ? semanticHealthGood : semanticHealthBad
    }

    function dopColor(value) {
        var dop = numericValue(value)
        if (isNaN(dop) || dop <= 0) return semanticNeutral
        if (dop <= 1.5) return semanticHealthGood
        if (dop <= 3.0) return semanticPrecision
        if (dop <= 6.0) return semanticHealthWarn
        return semanticHealthBad
    }

    function ageColor(value) {
        var seconds = numericValue(value)
        if (isNaN(seconds) || seconds < 0) return semanticNeutral
        if (seconds <= 2) return semanticHealthGood
        if (seconds <= 10) return semanticHealthWarn
        return semanticHealthBad
    }

    function satelliteUsageColor(usedValue, totalValue) {
        var used = numericValue(usedValue)
        var total = numericValue(totalValue)
        if (isNaN(used) || used <= 0) return semanticHealthBad
        if (used < 4) return semanticHealthBad
        if (used < 8) return semanticHealthWarn
        if (!isNaN(total) && total > 0 && used / total < 0.35) return semanticHealthWarn
        return semanticHealthGood
    }

    function statusColor(value) {
        var text = semanticText(value).toLowerCase()
        if (text.length === 0 || text === "--") return semanticNeutral
        if (text === "a" || text === "d" || text === "f" || text === "r") return semanticHealthGood
        if (text === "v" || text === "n") return semanticHealthBad
        if (text.indexOf("warn") >= 0 || text.indexOf("stale") >= 0 || text.indexOf("degrad") >= 0) return semanticHealthWarn
        if (text.indexOf("err") >= 0 || text.indexOf("fail") >= 0 || text.indexOf("invalid") >= 0 || text.indexOf("bad") >= 0) return semanticHealthBad
        if (text.indexOf("ok") >= 0 || text.indexOf("valid") >= 0 || text.indexOf("fix") >= 0 || text.indexOf("lock") >= 0
                || text.indexOf("active") >= 0 || text.indexOf("enable") >= 0 || text.indexOf("true") >= 0) {
            return semanticHealthGood
        }
        return semanticNeutral
    }

    function errorColor(value) {
        var values = numericValues(value)
        if (values.length > 0) {
            for (var i = 0; i < values.length; ++i) {
                if (Math.abs(values[i]) > 0) {
                    return semanticHealthBad
                }
            }
            return semanticHealthGood
        }
        return statusColor(value)
    }

    function semanticColor(role, value, fallbackColor) {
        var fallback = fallbackColor !== undefined ? fallbackColor : semanticData
        var key = semanticText(role).toLowerCase().replace(/_/g, "-")
        if (key.length === 0) return fallback
        if (key === "position") return semanticPosition
        if (key === "time") return semanticTime
        if (key === "motion") return semanticMotion
        if (key === "precision") return semanticPrecision
        if (key === "correction") return semanticCorrection
        if (key === "power") return semanticPower
        if (key === "rf") return semanticRf
        if (key === "data" || key === "identity" || key === "count") return semanticData
        if (key === "context") return semanticContext
        if (key === "neutral") return semanticNeutral
        if (key === "good" || key === "success") return semanticHealthGood
        if (key === "warn" || key === "warning") return semanticHealthWarn
        if (key === "bad" || key === "danger") return semanticHealthBad
        if (key === "quality" || key === "fix-quality") return fixQualityColor(value)
        if (key === "dop") return dopColor(value)
        if (key === "age" || key === "freshness") return ageColor(value)
        if (key === "satellites") return satelliteUsageColor(value)
        if (key === "status" || key === "health") return statusColor(value)
        if (key === "error") return errorColor(value)
        if (key.indexOf("constellation:") === 0) return constellationColor(key.substring(14).toUpperCase(), true)
        return fallback
    }

    function metricAccentColor(role, value, valueText, emphasize) {
        var semanticValue = value !== undefined && value !== null ? value : valueText
        return semanticColor(role, semanticValue, emphasize ? semanticHealthGood : semanticData)
    }

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
