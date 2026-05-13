import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root

    property string entryId: ""
    property bool   isNew: entryId === ""

    property string fieldTitle:      ""
    property string fieldUsername:   ""
    property string fieldPassword:   ""
    property string fieldUrl:        ""
    property string fieldNotes:      ""
    property string fieldTotpSecret: ""
    property string saveError:       ""

    readonly property bool premiumActive: typeof premiumGate !== "undefined" && premiumGate.premium

    signal saved(string id)
    signal cancelled()

    Component.onCompleted: {
        if (!root.isNew && typeof vaultModel !== "undefined") {
            const e = vaultModel.entryById(root.entryId)
            root.fieldTitle      = e.title       || ""
            root.fieldUsername   = e.username    || ""
            root.fieldPassword   = e.password    || ""
            root.fieldUrl        = e.url         || ""
            root.fieldNotes      = e.notes       || ""
            root.fieldTotpSecret = e.totp_secret || ""
        }
    }

    Rectangle { anchors.fill: parent; color: Theme.background }

    // Header (fixed top)
    ColumnLayout {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: Theme.spacingL
        anchors.bottomMargin: Theme.spacingS
        spacing: Theme.spacingS

        Text {
            text: root.isNew ? "New entry" : "Edit entry"
            color: Theme.foreground
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeXL
            font.weight: Theme.weightSemibold
        }
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.divider
        }
    }

    // Footer (fixed bottom) - Save/Cancel always visible
    ColumnLayout {
        id: footer
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: Theme.spacingL
        anchors.topMargin: Theme.spacingS
        spacing: Theme.spacingS

        Text {
            visible: root.saveError !== ""
            text: root.saveError
            color: Theme.danger
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeS
        }
        RowLayout {
            spacing: Theme.spacingS
            Layout.alignment: Qt.AlignRight
            Button {
                text: "Cancel"
                onClicked: root.cancelled()
            }
            Button {
                text: "Save"
                enabled: root.fieldTitle.trim() !== ""
                Material.background: Theme.accent
                Material.foreground: Theme.background
                onClicked: {
                    if (typeof vaultModel === "undefined" || typeof vaultManager === "undefined") {
                        root.saveError = "vault not ready"
                        return
                    }
                    let newId = root.entryId
                    if (root.isNew) {
                        newId = vaultModel.addLogin(root.fieldTitle, root.fieldUsername,
                                                    root.fieldPassword, root.fieldUrl,
                                                    root.fieldNotes, root.fieldTotpSecret)
                        if (!newId) {
                            root.saveError = "could not add entry"
                            return
                        }
                    } else {
                        const ok = vaultModel.updateLogin(root.entryId, root.fieldTitle,
                                                          root.fieldUsername, root.fieldPassword,
                                                          root.fieldUrl, root.fieldNotes,
                                                          root.fieldTotpSecret)
                        if (!ok) {
                            root.saveError = "could not update entry"
                            return
                        }
                    }
                    if (!vaultManager.save()) {
                        root.saveError = "could not write vault file"
                        return
                    }
                    root.saved(newId)
                }
            }
        }
    }

    // Scrollable middle: form fields between header and footer
    ScrollView {
        id: scroll
        anchors.top: header.bottom
        anchors.bottom: footer.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: Theme.spacingL
        anchors.rightMargin: Theme.spacingL
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        contentWidth: availableWidth

        ColumnLayout {
            width: scroll.availableWidth
            spacing: Theme.spacingS

            Text {
                text: "title"
                color: Theme.foregroundMuted
                font.family: Theme.fontFamilyMono
                font.pixelSize: Theme.fontSizeXS
            }
            TextField {
                Layout.fillWidth: true
                text: root.fieldTitle
                placeholderText: "e.g. Gmail"
                Material.accent: Theme.accent
                onTextChanged: root.fieldTitle = text
            }

            Text {
                text: "username"
                color: Theme.foregroundMuted
                font.family: Theme.fontFamilyMono
                font.pixelSize: Theme.fontSizeXS
            }
            TextField {
                Layout.fillWidth: true
                text: root.fieldUsername
                placeholderText: "user@example.com"
                Material.accent: Theme.accent
                onTextChanged: root.fieldUsername = text
            }

            Text {
                text: "password"
                color: Theme.foregroundMuted
                font.family: Theme.fontFamilyMono
                font.pixelSize: Theme.fontSizeXS
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingS
                TextField {
                    id: pwField
                    Layout.fillWidth: true
                    text: root.fieldPassword
                    placeholderText: "password"
                    echoMode: TextInput.Password
                    Material.accent: Theme.accent
                    onTextChanged: root.fieldPassword = text
                }
                Button {
                    text: pwField.echoMode === TextInput.Password ? "show" : "hide"
                    flat: true
                    font.family: Theme.fontFamilyMono
                    font.pixelSize: Theme.fontSizeXS
                    onClicked: pwField.echoMode = pwField.echoMode === TextInput.Password
                        ? TextInput.Normal
                        : TextInput.Password
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: Theme.spacingXS
                spacing: Theme.spacingS

                Button {
                    text: "Generate strong password"
                    enabled: root.premiumActive
                    Material.background: root.premiumActive ? Theme.accent : Theme.surfaceElevated
                    Material.foreground: root.premiumActive ? Theme.background : Theme.foregroundDim
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeS
                    font.weight: Theme.weightMedium
                    onClicked: {
                        if (typeof passwordGenerator === "undefined") return
                        const newPw = passwordGenerator.generate(20, true, true, true, true, true)
                        if (newPw && newPw.length > 0) {
                            root.fieldPassword = newPw
                            pwField.text       = newPw
                            pwField.echoMode   = TextInput.Normal
                        }
                    }
                }
                Text {
                    text: root.premiumActive
                        ? "20 chars, mixed case + digits + symbols, ambiguous chars removed"
                        : "premium required"
                    color: root.premiumActive ? Theme.foregroundMuted : Theme.foregroundDim
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeXS
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
            }

            Text {
                text: "url"
                color: Theme.foregroundMuted
                font.family: Theme.fontFamilyMono
                font.pixelSize: Theme.fontSizeXS
                Layout.topMargin: Theme.spacingS
            }
            TextField {
                Layout.fillWidth: true
                text: root.fieldUrl
                placeholderText: "https://example.com"
                Material.accent: Theme.accent
                onTextChanged: root.fieldUrl = text
            }

            Text {
                text: "totp secret"
                color: Theme.foregroundMuted
                font.family: Theme.fontFamilyMono
                font.pixelSize: Theme.fontSizeXS
            }
            TextField {
                Layout.fillWidth: true
                text: root.fieldTotpSecret
                placeholderText: "base32 (e.g. JBSWY3DPEHPK3PXP) - optional"
                font.family: Theme.fontFamilyMono
                Material.accent: Theme.accent
                onTextChanged: root.fieldTotpSecret = text
            }
            Text {
                visible: root.fieldTotpSecret !== "" && !root.premiumActive
                text: "TOTP codes display only when premium is active."
                color: Theme.foregroundDim
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeXS
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }

            Text {
                text: "notes"
                color: Theme.foregroundMuted
                font.family: Theme.fontFamilyMono
                font.pixelSize: Theme.fontSizeXS
            }
            TextArea {
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                text: root.fieldNotes
                placeholderText: "optional notes"
                wrapMode: TextEdit.Wrap
                Material.accent: Theme.accent
                onTextChanged: root.fieldNotes = text
            }
        }
    }
}
