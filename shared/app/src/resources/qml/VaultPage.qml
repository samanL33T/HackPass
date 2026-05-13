import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root
    signal settingsRequested()
    signal locked()

    // "empty" | "detail" | "edit-new" | "edit-existing"
    property string detailMode: "empty"
    property string selectedEntryId: ""

    function selectEntry(id) {
        root.selectedEntryId = id
        root.detailMode = "detail"
    }

    function startNewEntry() {
        root.selectedEntryId = ""
        root.detailMode = "edit-new"
    }

    function startEditCurrent() {
        if (root.selectedEntryId !== "") {
            root.detailMode = "edit-existing"
        }
    }

    Rectangle { anchors.fill: parent; color: Theme.background }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // === LEFT: sidebar ===
        Rectangle {
            Layout.preferredWidth: 320
            Layout.fillHeight: true
            color: Theme.surface

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingM
                spacing: Theme.spacingS

                RowLayout {
                    spacing: Theme.spacingS
                    Layout.fillWidth: true

                    Image {
                        source: "images/logo.svg"
                        sourceSize.width: 24
                        sourceSize.height: 24
                    }
                    Text {
                        text: "HackPass"
                        color: Theme.foreground
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeL
                        font.weight: Theme.weightSemibold
                        Layout.fillWidth: true
                    }
                    ToolButton {
                        text: "+"
                        font.pixelSize: Theme.fontSizeXL
                        ToolTip.text: "Add new entry"
                        ToolTip.visible: hovered
                        ToolTip.delay: 500
                        onClicked: root.startNewEntry()
                    }
                    ToolButton {
                        text: "⚙"
                        font.pixelSize: Theme.fontSizeL
                        ToolTip.text: "Settings"
                        ToolTip.visible: hovered
                        ToolTip.delay: 500
                        onClicked: root.settingsRequested()
                    }
                    ToolButton {
                        text: "⚿"
                        font.pixelSize: Theme.fontSizeL
                        ToolTip.text: "Lock vault"
                        ToolTip.visible: hovered
                        ToolTip.delay: 500
                        onClicked: {
                            if (typeof vaultManager !== "undefined") vaultManager.lock()
                            root.locked()
                        }
                    }
                }

                SearchBar {
                    id: search
                    Layout.fillWidth: true
                    Layout.topMargin: Theme.spacingS
                }

                ListView {
                    id: entryList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 2
                    model: typeof vaultModel !== "undefined" ? vaultModel : null

                    delegate: EntryCard {
                        width: ListView.view.width
                        entryId: model.id
                        entryTitle: model.title
                        entryUsername: model.username
                        entryUrl: model.url
                        selected: root.selectedEntryId === model.id
                        onClicked: root.selectEntry(model.id)
                    }

                    Item {
                        anchors.centerIn: parent
                        visible: entryList.count === 0
                        width: parent.width
                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: Theme.spacingS
                            Text {
                                text: "no entries yet"
                                color: Theme.foregroundDim
                                font.family: Theme.fontFamilyMono
                                font.pixelSize: Theme.fontSizeS
                                Layout.alignment: Qt.AlignHCenter
                            }
                            Text {
                                text: "tap + to add one"
                                color: Theme.foregroundDim
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontSizeXS
                                Layout.alignment: Qt.AlignHCenter
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.preferredWidth: 1
            Layout.fillHeight: true
            color: Theme.divider
        }

        // === RIGHT: detail / edit / empty ===
        Loader {
            id: detailLoader
            Layout.fillWidth: true
            Layout.fillHeight: true
            sourceComponent: {
                switch (root.detailMode) {
                    case "edit-new":      return editNewComponent
                    case "edit-existing": return editExistingComponent
                    case "detail":        return root.selectedEntryId !== "" ? detailComponent : emptyComponent
                    default:              return emptyComponent
                }
            }
        }
    }

    Component {
        id: emptyComponent
        Rectangle {
            color: Theme.background
            ColumnLayout {
                anchors.centerIn: parent
                spacing: Theme.spacingM
                Text {
                    text: "Select an entry"
                    color: Theme.foregroundDim
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeL
                    Layout.alignment: Qt.AlignHCenter
                }
                Text {
                    text: "or tap + in the sidebar to add a new one"
                    color: Theme.foregroundDim
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeS
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }
    }

    Component {
        id: detailComponent
        EntryDetailPage {
            entryId: root.selectedEntryId
            onEditRequested: root.startEditCurrent()
            onDeleteRequested: {
                if (typeof vaultModel === "undefined" || typeof vaultManager === "undefined") return
                if (vaultModel.removeEntryById(root.selectedEntryId)) {
                    vaultManager.save()
                    root.selectedEntryId = ""
                    root.detailMode = "empty"
                }
            }
        }
    }

    Component {
        id: editNewComponent
        EntryEditPage {
            entryId: ""
            onSaved: function(id) { root.selectEntry(id) }
            onCancelled: root.detailMode = root.selectedEntryId !== "" ? "detail" : "empty"
        }
    }

    Component {
        id: editExistingComponent
        EntryEditPage {
            entryId: root.selectedEntryId
            onSaved: function(id) { root.selectEntry(id) }
            onCancelled: root.detailMode = "detail"
        }
    }
}
