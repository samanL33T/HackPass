import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    property string entryId: ""
    property string entryTitle: ""
    property string entryUsername: ""
    property string entryUrl: ""
    property bool   selected: false

    signal clicked()

    implicitHeight: 56
    radius: Theme.radiusS
    color: selected ? Theme.surfaceHover : (mouseArea.containsMouse ? Theme.surfaceElevated : "transparent")
    border.color: selected ? Theme.accentSubtle : "transparent"
    border.width: 1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingS
        spacing: 0

        Text {
            text: root.entryTitle
            color: Theme.foreground
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeM
            font.weight: Theme.weightMedium
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
        Text {
            text: root.entryUsername || root.entryUrl
            color: Theme.foregroundMuted
            font.family: Theme.fontFamilyMono
            font.pixelSize: Theme.fontSizeXS
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
