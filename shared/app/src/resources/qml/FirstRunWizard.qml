import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root
    signal done()
    signal cancelled()

    property int step: 0
    property string vaultPath: typeof defaultVaultPath !== "undefined" ? defaultVaultPath : ""
    property string masterPassword: ""
    property string masterPasswordConfirm: ""
    property string extensionToken: ""
    property bool   vaultCreated: false

    Rectangle { anchors.fill: parent; color: Theme.background }

    StackLayout {
        id: stepStack
        anchors.fill: parent
        anchors.margins: Theme.spacingL
        currentIndex: root.step

        // --- Step 0: welcome ---
        ColumnLayout {
            spacing: Theme.spacingM
            Text {
                text: "Welcome to HackPass"
                color: Theme.foreground
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeXL
                font.weight: Theme.weightSemibold
            }
            Text {
                text: "Set up your vault in three steps: choose a master password, copy your extension token, and finish."
                color: Theme.foregroundMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeM
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.topMargin: Theme.spacingS
                color: "transparent"
                border.color: Theme.warning
                border.width: 1
                radius: 6
                implicitHeight: warnText.implicitHeight + Theme.spacingM * 2

                Text {
                    id: warnText
                    anchors.fill: parent
                    anchors.margins: Theme.spacingM
                    text: "HackPass is intentionally vulnerable. It is a research target for runtime instrumentation and reverse engineering, not a real password manager. Do not store passwords you actually use."
                    color: Theme.warning
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeS
                    wrapMode: Text.WordWrap
                }
            }

            Item { Layout.fillHeight: true }
            RowLayout {
                spacing: Theme.spacingS
                Layout.alignment: Qt.AlignRight
                Button { text: "Cancel"; onClicked: root.cancelled() }
                Button {
                    text: "Begin"
                    Material.background: Theme.accent
                    Material.foreground: Theme.background
                    onClicked: root.step = 1
                }
            }
        }

        // --- Step 1: master password ---
        ColumnLayout {
            spacing: Theme.spacingM
            Text {
                text: "Choose a master password"
                color: Theme.foreground
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeXL
                font.weight: Theme.weightSemibold
            }
            Text {
                text: "This is the only way to unlock your vault. We cannot recover it."
                color: Theme.foregroundMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeM
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            TextField {
                id: pw1
                echoMode: TextInput.Password
                placeholderText: "Master password (min 8 chars)"
                Layout.fillWidth: true
                onTextChanged: root.masterPassword = text
            }
            TextField {
                id: pw2
                echoMode: TextInput.Password
                placeholderText: "Confirm"
                Layout.fillWidth: true
                onTextChanged: root.masterPasswordConfirm = text
            }
            Text {
                visible: root.masterPassword !== "" && root.masterPassword !== root.masterPasswordConfirm
                text: "Passwords do not match"
                color: Theme.danger
                font.pixelSize: Theme.fontSizeS
            }
            Item { Layout.fillHeight: true }
            RowLayout {
                spacing: Theme.spacingS
                Layout.alignment: Qt.AlignRight
                Button { text: "Back"; onClicked: root.step = 0 }
                Button {
                    text: "Create vault"
                    enabled: root.masterPassword.length >= 8 && root.masterPassword === root.masterPasswordConfirm
                    Material.background: Theme.accent
                    Material.foreground: Theme.background
                    onClicked: {
                        // Create the vault file now, before showing the token step,
                        // so a failure here is recoverable without going to step 2.
                        if (typeof vaultManager !== "undefined") {
                            root.vaultCreated = vaultManager.createNew(root.vaultPath, root.masterPassword)
                        } else {
                            root.vaultCreated = true
                        }
                        if (root.vaultCreated) {
                            if (typeof tokenStore !== "undefined") {
                                root.extensionToken = tokenStore.rotate()
                            }
                            root.step = 2
                        }
                    }
                }
            }
            Text {
                visible: !root.vaultCreated && root.masterPasswordConfirm !== ""
                text: ""
                color: Theme.danger
                font.pixelSize: Theme.fontSizeS
            }
        }

        // --- Step 2: browser extension setup + token ---
        ColumnLayout {
            spacing: Theme.spacingM
            Text {
                text: "Browser extension"
                color: Theme.foreground
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeXL
                font.weight: Theme.weightSemibold
            }
            Text {
                text: "HackPass autofills credentials on web pages through a Chrome extension. To enable it:"
                color: Theme.foregroundMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeS
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            ColumnLayout {
                spacing: Theme.spacingXS
                Layout.leftMargin: Theme.spacingS
                Text {
                    text: "1. Open chrome://extensions in Chrome and enable Developer mode."
                    color: Theme.foreground
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeS
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Text {
                    text: "2. Click \"Load unpacked\" and select the extension/ folder from the HackPass repo."
                    color: Theme.foreground
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeS
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Text {
                    text: "3. Right-click the HackPass toolbar icon, choose Options, and paste this token:"
                    color: Theme.foreground
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeS
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 56
                Layout.topMargin: Theme.spacingS
                radius: Theme.radiusS
                color: Theme.surface
                border.color: Theme.border
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spacingM
                    anchors.rightMargin: Theme.spacingS
                    spacing: Theme.spacingS

                    Text {
                        text: root.extensionToken === "" ? "(generating...)" : root.extensionToken
                        color: Theme.foreground
                        font.family: Theme.fontFamilyMono
                        font.pixelSize: Theme.fontSizeM
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    Button {
                        text: "Copy"
                        flat: true
                        font.family: Theme.fontFamilyMono
                        font.pixelSize: Theme.fontSizeXS
                        onClicked: {
                            if (typeof clipboard !== "undefined") clipboard.setText(root.extensionToken)
                        }
                    }
                }
            }

            Text {
                text: "If you skip this step, the extension can still be set up later from Settings."
                color: Theme.foregroundDim
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeXS
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.topMargin: Theme.spacingS
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                spacing: Theme.spacingS
                Layout.alignment: Qt.AlignRight
                Button { text: "Back"; onClicked: root.step = 1 }
                Button {
                    text: "Finish"
                    Material.background: Theme.accent
                    Material.foreground: Theme.background
                    onClicked: root.done()
                }
            }
        }
    }
}
