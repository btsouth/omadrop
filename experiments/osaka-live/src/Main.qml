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
    Shortcut { sequence: "Escape"; context: Qt.ApplicationShortcut; onActivated: Qt.quit() }
}
