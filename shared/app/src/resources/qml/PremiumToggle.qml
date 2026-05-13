import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

RowLayout {
    id: root
    spacing: Theme.spacingM

    Text {
        text: "premium"
        color: Theme.foregroundMuted
        font.family: Theme.fontFamilyMono
        font.pixelSize: Theme.fontSizeS
        Layout.preferredWidth: 140
    }
    Switch {
        checked: typeof premiumGate !== "undefined" ? premiumGate.premium : false
        onToggled: {
            if (typeof premiumGate !== "undefined") {
                premiumGate.setPremium(checked)
            }
        }
    }
    Item { Layout.fillWidth: true }
}
