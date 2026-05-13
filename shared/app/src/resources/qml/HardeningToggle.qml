import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

RowLayout {
    id: root
    spacing: Theme.spacingM

    Text {
        text: "hardening"
        color: Theme.foregroundMuted
        font.family: Theme.fontFamilyMono
        font.pixelSize: Theme.fontSizeS
        Layout.preferredWidth: 140
    }
    Switch {
        checked: typeof appSettings !== "undefined" ? appSettings.hardeningEnabled : false
        onToggled: {
            if (typeof appSettings !== "undefined") {
                appSettings.hardeningEnabled = checked
            }
        }
    }
    Text {
        text: "anti-debug + process scanner"
        color: Theme.foregroundDim
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeXS
        Layout.fillWidth: true
    }
}
