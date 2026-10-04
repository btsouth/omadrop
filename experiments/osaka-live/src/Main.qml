import QtQuick
import QtQuick.Window
import Osaka 1.0
Window {
    width: 1920
    height: 1080
    visible: false
    visibility: Window.Hidden
    color: "black"
    title: "Omadrop Osaka Jade"
    OsakaItem { anchors.fill: parent }
    // Hide the pointer over the world unless it has moved recently.
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
        cursorShape: pointerMoved.running ? Qt.ArrowCursor : Qt.BlankCursor
        onPositionChanged: pointerMoved.restart()
        Timer { id: pointerMoved; interval: 1500 }
    }
    Shortcut { sequence: "Escape"; context: Qt.ApplicationShortcut; onActivated: Qt.quit() }
}
