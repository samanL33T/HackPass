import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Dialog {
    id: root
    title: "Authentication required"
    modal: true
    anchors.centerIn: parent
    width: 460

    Material.background: Theme.surface

    property string messageText: ""
    property string masterPasswordEntry: ""

    contentItem: ColumnLayout {
        spacing: Theme.spacingM

        Text {
            text: root.messageText
            color: Theme.foreground
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeM
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        TextField {
            id: pwField
            echoMode: TextInput.Password
            placeholderText: "Master password"
            Layout.fillWidth: true
            onTextChanged: root.masterPasswordEntry = text
        }
    }

    standardButtons: Dialog.Ok | Dialog.Cancel
    onAccepted: {
        if (typeof vaultManager !== "undefined") {
            vaultManager.reUnlockAfterAutoLock(root.masterPasswordEntry)
        }
    }
}
