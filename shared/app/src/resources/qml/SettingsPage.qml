import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root
    signal back()
    signal createNewVaultRequested()

    property string currentToken: typeof tokenStore !== "undefined" ? tokenStore.token() : ""
    property bool   tokenRevealed: false
    property string testStatus: ""
    property string exportStatus: ""

    // Premium status: license-driven (premiumGate.premium) OR server-side
    // override (serverFlagsApplier.exportPlaintextAllowed). Either unlocks the
    // export feature; both are independent attack/hook targets for the lesson.
    readonly property bool premiumActive: typeof premiumGate !== "undefined" && premiumGate.premium
    readonly property bool exportUnlocked: premiumActive
        || (typeof serverFlagsApplier !== "undefined" && serverFlagsApplier.exportPlaintextAllowed)

    Rectangle { anchors.fill: parent; color: Theme.background }

    ScrollView {
        anchors.fill: parent
        anchors.margins: Theme.spacingL
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: Theme.spacingM

            RowLayout {
                spacing: Theme.spacingS
                Layout.fillWidth: true
                ToolButton {
                    text: "←"
                    font.pixelSize: Theme.fontSizeL
                    onClicked: root.back()
                }
                Text {
                    text: "Settings"
                    color: Theme.foreground
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeXL
                    font.weight: Theme.weightSemibold
                }
                Item { Layout.fillWidth: true }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.divider
            }

            HardeningToggle { Layout.fillWidth: true }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingS
                Text {
                    text: "premium"
                    color: Theme.foregroundMuted
                    font.family: Theme.fontFamilyMono
                    font.pixelSize: Theme.fontSizeS
                    Layout.preferredWidth: 140
                }
                Text {
                    text: root.premiumActive ? "true (backend)" : "false (backend)"
                    color: root.premiumActive ? Theme.success : Theme.foregroundMuted
                    font.family: Theme.fontFamilyMono
                    font.pixelSize: Theme.fontSizeS
                }
                // Inline info icon. Hover reveals how to flip premium via
                // switchpremium.bat. Click also opens the same tip so touch
                // users can still see it.
                Text {
                    id: premiumInfoIcon
                    text: "ⓘ"   // circled lowercase i
                    color: Theme.accent
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeM
                    Layout.leftMargin: Theme.spacingXS
                    property bool tipOpen: false
                    MouseArea {
                        id: premiumInfoMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: premiumInfoIcon.tipOpen = !premiumInfoIcon.tipOpen
                    }
                    ToolTip {
                        visible: premiumInfoMouse.containsMouse || premiumInfoIcon.tipOpen
                        delay: 200
                        timeout: premiumInfoIcon.tipOpen ? -1 : 8000
                        text: "Premium is controlled server-side by policy.json\n(premium_active field).\n\nTo flip it: run switchpremium.bat\n(next to the app in the portable folder,\nor the 'Switch Premium' Start Menu shortcut).\nSet premium_active to true, save, then\nrelaunch HackPass."
                    }
                }
                Item { Layout.fillWidth: true }
            }

            ToggleRow {
                label: "auto-lock"
                valueText: typeof settingsBridge !== "undefined"
                           ? settingsBridge.autoLockMinutes + " minutes"
                           : "5 minutes"
                Layout.fillWidth: true
            }
            // Editable server URL. Changing the host invalidates the TOFU pin
            // for the old host so the next Test Connection can re-pin cleanly.
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingS

                Text {
                    text: "server url"
                    color: Theme.foregroundMuted
                    font.family: Theme.fontFamilyMono
                    font.pixelSize: Theme.fontSizeS
                    Layout.preferredWidth: 140
                }
                TextField {
                    id: serverUrlField
                    Layout.fillWidth: true
                    text: typeof appSettings !== "undefined" ? appSettings.serverUrl : "https://localhost:8443"
                    placeholderText: "https://host:port"
                    color: Theme.foreground
                    placeholderTextColor: Theme.foregroundDim
                    font.family: Theme.fontFamilyMono
                    font.pixelSize: Theme.fontSizeS
                    Material.accent: Theme.accent
                    enabled: typeof appSettings !== "undefined"
                    onAccepted: saveUrlBtn.clicked()
                }
                Button {
                    id: saveUrlBtn
                    text: "Save"
                    flat: true
                    enabled: typeof appSettings !== "undefined"
                             && serverUrlField.text.trim() !== ""
                             && serverUrlField.text !== appSettings.serverUrl
                    Material.foreground: Theme.accent
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeS
                    onClicked: {
                        const oldUrl = appSettings.serverUrl
                        appSettings.serverUrl = serverUrlField.text.trim()
                        if (typeof tofuStore !== "undefined") {
                            try {
                                const oldHost = new URL(oldUrl).host
                                const newHost = new URL(appSettings.serverUrl).host
                                if (oldHost && oldHost !== newHost) {
                                    tofuStore.forget(oldHost)
                                }
                            } catch (e) { /* malformed URL, ignore */ }
                        }
                        root.testStatus = "server url updated - click Test connection"
                    }
                }
            }
            ToggleRow {
                label: "device id"
                valueText: typeof appSettings !== "undefined" ? appSettings.deviceId : ""
                Layout.fillWidth: true
                visible: typeof appSettings !== "undefined" && appSettings.deviceId !== ""
            }

            // === Extension token ===
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.divider
                Layout.topMargin: Theme.spacingS
            }

            Text {
                text: "browser extension token"
                color: Theme.foregroundMuted
                font.family: Theme.fontFamilyMono
                font.pixelSize: Theme.fontSizeXS
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 44
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
                        Layout.fillWidth: true
                        text: root.currentToken === ""
                            ? "(no token set)"
                            : root.tokenRevealed
                                ? root.currentToken
                                : Array(root.currentToken.length + 1).join("•")
                        color: root.currentToken === "" ? Theme.foregroundDim : Theme.foreground
                        font.family: Theme.fontFamilyMono
                        font.pixelSize: Theme.fontSizeS
                        elide: Text.ElideRight
                    }
                    Button {
                        text: root.tokenRevealed ? "hide" : "show"
                        flat: true
                        enabled: root.currentToken !== ""
                        font.family: Theme.fontFamilyMono
                        font.pixelSize: Theme.fontSizeXS
                        onClicked: root.tokenRevealed = !root.tokenRevealed
                    }
                    Button {
                        text: "copy"
                        flat: true
                        enabled: root.currentToken !== ""
                        font.family: Theme.fontFamilyMono
                        font.pixelSize: Theme.fontSizeXS
                        onClicked: {
                            if (typeof clipboard !== "undefined") clipboard.setText(root.currentToken)
                        }
                    }
                    Button {
                        text: "rotate"
                        flat: true
                        Material.foreground: Theme.warning
                        font.family: Theme.fontFamilyMono
                        font.pixelSize: Theme.fontSizeXS
                        onClicked: {
                            if (typeof tokenStore !== "undefined") {
                                root.currentToken  = tokenStore.rotate()
                                root.tokenRevealed = true
                            }
                        }
                    }
                }
            }

            Text {
                text: "Paste this token into the HackPass browser extension's Options page. Rotate to invalidate the old one."
                color: Theme.foregroundDim
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeXS
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            // === Connection test ===
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.divider
                Layout.topMargin: Theme.spacingM
            }

            Text {
                text: "backend"
                color: Theme.foregroundMuted
                font.family: Theme.fontFamilyMono
                font.pixelSize: Theme.fontSizeXS
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingS

                Button {
                    id: testBtn
                    text: "Test connection"
                    Material.background: Theme.accent
                    Material.foreground: Theme.background
                    onClicked: {
                        if (typeof policyClient === "undefined" || typeof nam === "undefined") {
                            root.testStatus = "network layer not available"
                            return
                        }
                        root.testStatus = "testing..."
                        // First connect to a self-signed cert needs TOFU
                        // learning briefly enabled so the fingerprint can pin.
                        nam.setAllowTofuLearning(true)
                        policyClient.fetch()
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: root.testStatus
                    color: root.testStatus.indexOf("failed") === 0 ? Theme.danger
                         : root.testStatus.indexOf("ok") === 0     ? Theme.success
                         : Theme.foregroundMuted
                    font.family: Theme.fontFamilyMono
                    font.pixelSize: Theme.fontSizeS
                    wrapMode: Text.WordWrap
                }
            }

            Text {
                text: "Hits GET /api/v1/policy on the configured server URL. First connect auto-pins the server's TLS cert."
                color: Theme.foregroundDim
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeXS
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Connections {
                target: typeof policyClient !== "undefined" ? policyClient : null
                function onPolicyReceived(serverFlags) {
                    if (typeof nam !== "undefined") nam.setAllowTofuLearning(false)
                    const flagsCount = Object.keys(serverFlags || {}).length
                    root.testStatus = "ok - received " + flagsCount + " server flags"
                }
                function onPolicyFailed(reason) {
                    if (typeof nam !== "undefined") nam.setAllowTofuLearning(false)
                    root.testStatus = "failed: " + reason
                }
            }

            // === Premium features ===
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.divider
                Layout.topMargin: Theme.spacingM
            }

            Text {
                text: "premium features"
                color: Theme.foregroundMuted
                font.family: Theme.fontFamilyMono
                font.pixelSize: Theme.fontSizeXS
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingS

                Button {
                    text: "Export vault (plaintext JSON)"
                    enabled: root.exportUnlocked
                    Material.background: root.exportUnlocked ? Theme.accent : Theme.surfaceElevated
                    Material.foreground: root.exportUnlocked ? Theme.background : Theme.foregroundDim
                    onClicked: exportConfirm.open()
                }
                Text {
                    Layout.fillWidth: true
                    text: root.exportStatus !== ""
                        ? root.exportStatus
                        : (root.exportUnlocked ? "ready" : "premium required")
                    color: root.exportStatus.indexOf("failed") === 0 ? Theme.danger
                         : root.exportStatus.indexOf("exported") === 0 ? Theme.success
                         : root.exportUnlocked ? Theme.foregroundMuted : Theme.foregroundDim
                    font.family: Theme.fontFamilyMono
                    font.pixelSize: Theme.fontSizeS
                    wrapMode: Text.WordWrap
                }
            }

            Text {
                text: "Premium activates when the backend's policy.json has \"premium_active\": true. The export feature also unlocks if \"feature_export_plaintext\": true is set server-side. Either way, the resulting file is plaintext - treat with care."
                color: Theme.foregroundDim
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeXS
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            // === Vault management ===
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.divider
                Layout.topMargin: Theme.spacingM
            }

            Text {
                text: "vault"
                color: Theme.foregroundMuted
                font.family: Theme.fontFamilyMono
                font.pixelSize: Theme.fontSizeXS
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingS
                Text {
                    text: "current path"
                    color: Theme.foregroundMuted
                    font.family: Theme.fontFamilyMono
                    font.pixelSize: Theme.fontSizeS
                    Layout.preferredWidth: 140
                }
                Text {
                    text: typeof vaultManager !== "undefined" ? vaultManager.vaultPath() : ""
                    color: Theme.foreground
                    font.family: Theme.fontFamilyMono
                    font.pixelSize: Theme.fontSizeS
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }
            }

            Button {
                text: "Create new vault"
                Material.background: Theme.danger
                Material.foreground: Theme.foreground
                Layout.alignment: Qt.AlignLeft
                onClicked: confirmNewVault.open()
            }

            Text {
                text: "Creates a fresh vault file. Your current vault file will be renamed with a .bak suffix so you can recover it manually if needed."
                color: Theme.foregroundDim
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeXS
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Item { Layout.preferredHeight: Theme.spacingM }

            // === Branding footer ===
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.divider
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 2

                Text {
                    text: "HackPass " + Qt.application.version
                    color: Theme.foregroundDim
                    font.family: Theme.fontFamilyMono
                    font.pixelSize: Theme.fontSizeXS
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    id: brandLink
                    text: "by <a href=\"https://samanl33t.com\" style=\"color: " + Theme.accent + ";text-decoration: underline\">@samanl33t</a>"
                    color: Theme.foregroundDim
                    textFormat: Text.RichText
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeXS
                    Layout.alignment: Qt.AlignHCenter
                    onLinkActivated: function(link) { Qt.openUrlExternally(link) }
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.NoButton
                        cursorShape: parent.hoveredLink !== "" ? Qt.PointingHandCursor : Qt.ArrowCursor
                    }
                }
            }
        }
    }

    Dialog {
        id: confirmNewVault
        title: "Create a new vault?"
        modal: true
        anchors.centerIn: parent
        width: 440
        Material.background: Theme.surface

        contentItem: ColumnLayout {
            spacing: Theme.spacingS
            Text {
                text: "This will close your current vault and start the first-run wizard. Your current vault file will be renamed with a .bak suffix so you can recover it manually if you change your mind."
                color: Theme.foreground
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeS
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
        standardButtons: Dialog.Cancel | Dialog.Ok
        onAccepted: root.createNewVaultRequested()
    }

    Dialog {
        id: exportConfirm
        title: "Export vault to plaintext?"
        modal: true
        anchors.centerIn: parent
        width: 460
        Material.background: Theme.surface

        contentItem: ColumnLayout {
            spacing: Theme.spacingS
            Text {
                text: "This writes every entry (titles, usernames, passwords, notes, TOTP secrets, history) as plaintext JSON to your AppData/cache directory. Anyone who reads that file has full access to your credentials. Continue only if you understand the risk."
                color: Theme.foreground
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeS
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
        standardButtons: Dialog.Cancel | Dialog.Ok
        onAccepted: {
            if (typeof vaultModel === "undefined" || typeof appSettings === "undefined") {
                root.exportStatus = "failed: not ready"
                return
            }
            const stamp = new Date().toISOString().replace(/[:.]/g, "-").slice(0, 19)
            const path  = appSettings.serverUrl
                ? ""  // placeholder so the linter sees usage; real path below
                : ""
            // Build the export path next to the vault file.
            const dir = typeof defaultVaultPath !== "undefined"
                ? defaultVaultPath.substring(0, defaultVaultPath.lastIndexOf("/"))
                : ""
            if (!dir) {
                root.exportStatus = "failed: vault location unknown"
                return
            }
            const outPath = dir + "/hackpass-export-" + stamp + ".json"
            if (vaultModel.exportToJson(outPath)) {
                root.exportStatus = "exported to " + outPath
                if (typeof clipboard !== "undefined") clipboard.setText(outPath)
            } else {
                root.exportStatus = "failed: could not write file"
            }
        }
    }
}
