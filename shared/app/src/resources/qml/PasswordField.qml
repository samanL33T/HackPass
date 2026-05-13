import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root
    property string label: "password"
    property string valueText: ""
    property bool revealed: false
    spacing: 2

    Text {
        text: root.label
        color: Theme.foregroundMuted
        font.family: Theme.fontFamilyMono
        font.pixelSize: Theme.fontSizeXS
    }

    Rectangle {
        Layout.fillWidth: true
        implicitHeight: 36
        radius: Theme.radiusS
        color: Theme.surface
        border.color: Theme.border
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.spacingS
            anchors.rightMargin: Theme.spacingS
            spacing: Theme.spacingS

            Text {
                Layout.fillWidth: true
                text: root.valueText === ""
                    ? ""
                    : (root.revealed
                        ? root.valueText
                        : Array(root.valueText.length + 1).join("•"))
                color: Theme.foreground
                font.family: Theme.fontFamilyMono
                font.pixelSize: Theme.fontSizeS
                elide: Text.ElideRight
            }
            Button {
                text: root.revealed ? "hide" : "show"
                flat: true
                enabled: root.valueText !== ""
                font.family: Theme.fontFamilyMono
                font.pixelSize: Theme.fontSizeXS
                onClicked: root.revealed = !root.revealed
            }
            Button {
                text: "copy"
                flat: true
                enabled: root.valueText !== ""
                font.family: Theme.fontFamilyMono
                font.pixelSize: Theme.fontSizeXS
                onClicked: {
                    if (typeof clipboard !== "undefined") clipboard.setText(root.valueText)
                }
            }
        }
    }
}
