import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root
    signal unlocked()
    signal wizardRequested()

    property string errorText: ""
    property bool   hasVault: typeof vaultManager !== "undefined" && vaultManager.state !== 0

    Rectangle { anchors.fill: parent; color: Theme.background }

    Rectangle {
        id: warningStrip
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 30
        color: Theme.warning
        Text {
            anchors.centerIn: parent
            text: "Intentionally vulnerable - do not store real passwords"
            color: "#1a1208"
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeS
            font.weight: Theme.weightMedium
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: 340
        spacing: Theme.spacingM

        RowLayout {
            spacing: Theme.spacingS
            Layout.alignment: Qt.AlignHCenter
            Image {
                source: "images/logo.svg"
                sourceSize.width: 32
                sourceSize.height: 32
            }
            Text {
                text: "HackPass"
                color: Theme.foreground
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeXL
                font.weight: Theme.weightSemibold
            }
        }

        Text {
            text: root.hasVault ? "Unlock your vault" : "No vault found - set one up to begin"
            color: Theme.foregroundMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeS
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: Theme.spacingS
        }

        TextField {
            id: passwordField
            Layout.fillWidth: true
            echoMode: TextInput.Password
            placeholderText: "Master password"
            enabled: root.hasVault
            color: Theme.foreground
            placeholderTextColor: Theme.foregroundDim
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeM
            Material.accent: Theme.accent
            onAccepted: unlockButton.clicked()
        }

        Button {
            id: unlockButton
            text: "Unlock"
            Layout.fillWidth: true
            enabled: root.hasVault && passwordField.text.length > 0
            Material.background: Theme.accent
            Material.foreground: Theme.background
            font.family: Theme.fontFamily
            font.weight: Theme.weightMedium
            onClicked: {
                if (typeof vaultManager === "undefined") {
                    root.unlocked()
                    return
                }
                if (vaultManager.unlock(passwordField.text)) {
                    passwordField.text = ""
                    root.errorText = ""
                    root.unlocked()
                } else {
                    root.errorText = "Invalid master password"
                }
            }
        }

        Text {
            text: root.errorText
            color: Theme.danger
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeS
            visible: root.errorText !== ""
            Layout.alignment: Qt.AlignHCenter
        }

        Item { Layout.preferredHeight: Theme.spacingS }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.divider
        }

        Text {
            text: root.hasVault ? "Forgot your master password? Set up a fresh vault."
                                : "Set up a new vault"
            color: Theme.accent
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeS
            font.underline: true
            Layout.alignment: Qt.AlignHCenter
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.wizardRequested()
            }
        }
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.spacingM
        text: "by <a href=\"https://samanl33t.com\" style=\"color: " + Theme.accent + ";text-decoration: underline\">@samanl33t</a>"
        color: Theme.foregroundDim
        textFormat: Text.RichText
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeXS
        onLinkActivated: function(link) { Qt.openUrlExternally(link) }
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.NoButton
            cursorShape: parent.hoveredLink !== "" ? Qt.PointingHandCursor : Qt.ArrowCursor
        }
    }
}
