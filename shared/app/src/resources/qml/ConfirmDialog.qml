import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Dialog {
    id: root
    modal: true
    anchors.centerIn: parent
    width: 420
    Material.background: Theme.surface

    property string promptText: ""
    signal confirmed()

    contentItem: Text {
        text: root.promptText
        color: Theme.foreground
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeM
        wrapMode: Text.WordWrap
    }

    standardButtons: Dialog.Ok | Dialog.Cancel
    onAccepted: root.confirmed()
}
