import QtQuick
import QtQuick.Layouts

Item {
    id: root
    signal finished()

    Rectangle {
        anchors.fill: parent
        color: Theme.background
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: Theme.spacingL

        Image {
            source: "images/logo.svg"
            sourceSize.width: 96
            sourceSize.height: 96
            fillMode: Image.PreserveAspectFit
            opacity: 0
            Layout.alignment: Qt.AlignHCenter
            NumberAnimation on opacity {
                from: 0; to: 1
                duration: 600
                easing.type: Easing.OutCubic
            }
        }
        Text {
            text: "HackPass"
            color: Theme.foreground
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSize2XL
            font.weight: Theme.weightSemibold
            opacity: 0
            Layout.alignment: Qt.AlignHCenter
            NumberAnimation on opacity {
                from: 0; to: 1
                duration: 600
                easing.type: Easing.OutCubic
            }
        }
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.spacingL
        text: "by <a href=\"https://samanl33t.com\" style=\"color: " + Theme.accent + ";text-decoration: underline\">@samanl33t</a>"
        color: Theme.foregroundDim
        textFormat: Text.RichText
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeXS
        opacity: 0
        onLinkActivated: function(link) { Qt.openUrlExternally(link) }
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.NoButton
            cursorShape: parent.hoveredLink !== "" ? Qt.PointingHandCursor : Qt.ArrowCursor
        }
        NumberAnimation on opacity {
            from: 0; to: 1
            duration: 600
            easing.type: Easing.OutCubic
        }
    }

    Timer {
        interval: 900
        running: true
        onTriggered: root.finished()
    }
}
