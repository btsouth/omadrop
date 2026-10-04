import QtQuick
import QtQuick.Window
import Osaka 1.0
Window {
    width: 1920
    height: 1080
    visible: true
    visibility: Window.FullScreen
    color: "black"
    title: "Omadrop Osaka live milestone"
    OsakaItem { anchors.fill: parent }
    Shortcut { sequence: "Escape"; context: Qt.ApplicationShortcut; onActivated: Qt.quit() }
}
