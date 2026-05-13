import QtQuick
import QtQuick.Layouts

RowLayout {
    id: root
    property string label: ""
    property string valueText: ""
    property bool   multiline: false

    spacing: Theme.spacingM

    Text {
        text: root.label
        color: Theme.foregroundMuted
        font.family: Theme.fontFamilyMono
        font.pixelSize: Theme.fontSizeS
        Layout.preferredWidth: 140
    }
    Text {
        text: root.valueText
        color: Theme.foreground
        font.family: root.multiline ? Theme.fontFamily : Theme.fontFamilyMono
        font.pixelSize: Theme.fontSizeS
        wrapMode: root.multiline ? Text.WordWrap : Text.NoWrap
        elide: root.multiline ? Text.ElideNone : Text.ElideRight
        Layout.fillWidth: true
    }
}
