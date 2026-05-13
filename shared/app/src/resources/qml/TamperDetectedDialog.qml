import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Dialog {
    id: root
    title: "Tamper detected"
    modal: true
    closePolicy: Popup.NoAutoClose
    anchors.centerIn: parent
    width: 440
    Material.background: Theme.surface

    property string reason: ""

    contentItem: ColumnLayout {
        spacing: Theme.spacingM
        Text {
            text: "HackPass has detected interference and is closing for safety."
            color: Theme.foreground
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeM
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Text {
            text: root.reason
            color: Theme.foregroundMuted
            font.family: Theme.fontFamilyMono
            font.pixelSize: Theme.fontSizeXS
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }

    standardButtons: Dialog.Close
    onRejected: Qt.quit()
    onAccepted: Qt.quit()
}
