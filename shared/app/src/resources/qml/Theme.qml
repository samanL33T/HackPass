pragma Singleton

import QtQuick

QtObject {
    // Base surface palette. Warm dark, KeePassXC-restrained, samanl33t-leaning.
    readonly property color background:       "#14141A"
    readonly property color surface:          "#1A1A22"
    readonly property color surfaceElevated:  "#22222C"
    readonly property color surfaceHover:     "#2A2A35"

    // Text. Light, slightly warm. Not pure white. Easier on the eye for long sessions.
    readonly property color foreground:       "#E4E4E8"
    readonly property color foregroundMuted:  "#8A8A93"
    readonly property color foregroundDim:    "#5C5C66"

    // Lines, borders, separators. Subtle but present.
    readonly property color divider:          "#26262F"
    readonly property color border:           "#33333E"
    readonly property color borderFocus:      "#7895B5"

    // Accents. Slate-blue primary lifted from the samanl33t portfolio palette,
    // sits in the same family as KeePassXC's traditional teal-blue accent.
    readonly property color primary:          "#7895B5"
    readonly property color accent:           "#7895B5"
    readonly property color accentSubtle:     "#3F4D5C"
    readonly property color accentMuted:      "#4F6478"

    // State colors. All muted, no neon.
    readonly property color danger:           "#C26460"
    readonly property color success:          "#7AAA8F"
    readonly property color warning:          "#C9A876"
    readonly property color info:             "#7895B5"

    // Radii. Smaller than Material 3 default; closer to KeePassXC's flat-but-not-sharp look.
    readonly property int   radiusS:          3
    readonly property int   radiusM:          5
    readonly property int   radiusL:          8

    // Spacing. 4px grid.
    readonly property int   spacingXS:        4
    readonly property int   spacingS:         8
    readonly property int   spacingM:         16
    readonly property int   spacingL:         24
    readonly property int   spacingXL:        32

    // Type sizes. Slightly tighter than the prior pass.
    readonly property int   fontSizeXS:       11
    readonly property int   fontSizeS:        12
    readonly property int   fontSizeM:        13
    readonly property int   fontSizeL:        15
    readonly property int   fontSizeXL:       18
    readonly property int   fontSize2XL:      24

    // Type families. font.family is a single string in QML; Qt picks the system
    // default if the requested face is missing. Pick the best face that ships with
    // Windows 10/11 by default so no extra fonts are required.
    readonly property string fontFamily:      "Segoe UI Variable"
    readonly property string fontFamilyMono:  "Cascadia Mono"

    // Weights.
    readonly property int   weightRegular:    400
    readonly property int   weightMedium:     500
    readonly property int   weightSemibold:   600
}
