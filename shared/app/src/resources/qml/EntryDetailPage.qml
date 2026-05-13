import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root
    property string entryId: ""

    signal editRequested()
    signal deleteRequested()

    readonly property var    entryData:      entryId !== "" && typeof vaultModel !== "undefined"
        ? vaultModel.entryById(entryId)
        : ({})
    readonly property string totpSecret:     entryData.totp_secret || ""
    readonly property bool   hasTotpSecret:  totpSecret !== ""
    readonly property bool   premiumActive:  typeof premiumGate !== "undefined" && premiumGate.premium

    property string totpCode:         "------"
    property int    totpSecondsLeft:  30

    function refreshTotp() {
        if (!root.hasTotpSecret || !root.premiumActive || typeof totp === "undefined") {
            root.totpCode        = "------"
            root.totpSecondsLeft = 30
            return
        }
        root.totpCode        = totp.generateCode(root.totpSecret, 0)
        root.totpSecondsLeft = totp.secondsRemaining(0)
    }

    Component.onCompleted: refreshTotp()
    onEntryIdChanged: refreshTotp()
    onPremiumActiveChanged: refreshTotp()

    Timer {
        id: totpTimer
        interval: 1000
        repeat: true
        running: root.visible && root.hasTotpSecret && root.premiumActive
        onTriggered: root.refreshTotp()
    }

    Rectangle { anchors.fill: parent; color: Theme.background }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingL
        spacing: Theme.spacingM

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingS

            Text {
                text: root.entryData.title || "(no title)"
                color: Theme.foreground
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeXL
                font.weight: Theme.weightSemibold
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            Button {
                text: "Edit"
                flat: true
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeS
                onClicked: root.editRequested()
            }
            Button {
                text: "Delete"
                flat: true
                Material.foreground: Theme.danger
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeS
                onClicked: root.deleteRequested()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.divider
        }

        ToggleRow {
            label: "username"
            valueText: root.entryData.username || ""
            Layout.fillWidth: true
        }
        ToggleRow {
            label: "url"
            valueText: root.entryData.url || ""
            Layout.fillWidth: true
        }
        ToggleRow {
            label: "notes"
            valueText: root.entryData.notes || ""
            multiline: true
            Layout.fillWidth: true
        }

        PasswordField {
            label: "password"
            valueText: root.entryData.password || ""
            Layout.fillWidth: true
        }

        RowLayout {
            visible: root.hasTotpSecret
            Layout.fillWidth: true
            spacing: Theme.spacingM

            Text {
                text: "totp"
                color: Theme.foregroundMuted
                font.family: Theme.fontFamilyMono
                font.pixelSize: Theme.fontSizeS
                Layout.preferredWidth: 140
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingXS

                RowLayout {
                    spacing: Theme.spacingM
                    Layout.fillWidth: true

                    Text {
                        text: root.premiumActive ? root.totpCode : "------"
                        color: root.premiumActive ? Theme.foreground : Theme.foregroundDim
                        font.family: Theme.fontFamilyMono
                        font.pixelSize: Theme.fontSize2XL
                        font.weight: Theme.weightSemibold
                    }

                    Rectangle {
                        Layout.preferredWidth: 28
                        Layout.preferredHeight: 28
                        radius: 14
                        color: "transparent"
                        border.width: 2
                        border.color: root.premiumActive ? Theme.accent : Theme.foregroundDim
                        visible: root.premiumActive

                        Text {
                            anchors.centerIn: parent
                            text: root.totpSecondsLeft
                            color: Theme.foreground
                            font.family: Theme.fontFamilyMono
                            font.pixelSize: Theme.fontSizeXS
                        }
                    }

                    Button {
                        text: "copy"
                        flat: true
                        visible: root.premiumActive
                        font.family: Theme.fontFamilyMono
                        font.pixelSize: Theme.fontSizeXS
                        onClicked: {
                            if (typeof clipboard !== "undefined") {
                                clipboard.setText(root.totpCode)
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }
                }

                Text {
                    visible: !root.premiumActive
                    text: "premium required to view current code"
                    color: Theme.foregroundDim
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeXS
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
