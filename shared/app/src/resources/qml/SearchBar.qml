import QtQuick

Rectangle {
    id: root
    property alias text: input.text
    signal searchChanged(string text)

    implicitHeight: 36
    radius: Theme.radiusS
    color: Theme.surfaceElevated
    border.color: input.activeFocus ? Theme.borderFocus : Theme.border
    border.width: 1
    clip: true

    TextInput {
        id: input
        anchors.fill: parent
        anchors.leftMargin: Theme.spacingS
        anchors.rightMargin: Theme.spacingS
        verticalAlignment: TextInput.AlignVCenter
        color: Theme.foreground
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeS
        selectByMouse: true
        clip: true
        onTextChanged: root.searchChanged(text)
    }

    Text {
        visible: input.text === "" && !input.activeFocus
        anchors.left: parent.left
        anchors.leftMargin: Theme.spacingS
        anchors.verticalCenter: parent.verticalCenter
        text: "search"
        color: Theme.foregroundDim
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeS
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.IBeamCursor
        onClicked: input.forceActiveFocus()
        propagateComposedEvents: true
    }
}
