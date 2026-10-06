import QtQuick
import QtQuick.Window
import Osaka 1.0
Window {
    width: 1280
    height: 720
    minimumWidth: 480
    minimumHeight: 270
    visible: true
    color: "black"
    title: "Omadrop preview: " + worldPreview.worldName
    // Osaka draws a 16:9 picture; keep it undistorted when the window is resized.
    OsakaItem {
        width: Math.min(parent.width, parent.height * 16 / 9)
        height: width * 9 / 16
        anchors.centerIn: parent
    }
    Rectangle {
        id: panel
        x: 10
        y: parent.height - height - 10
        width: Math.min(label.implicitWidth, parent.width * 0.7) + 16
        height: label.implicitHeight + 12
        radius: 4
        color: "#a0000000"
        opacity: worldPreview.error !== "" ? 1 : (worldPreview.flash ? 0.9 : 0.45)
        Behavior on opacity { NumberAnimation { duration: 300 } }
        Text {
            id: label
            x: 8
            y: 6
            width: Math.min(implicitWidth, panel.parent.width * 0.7)
            wrapMode: Text.Wrap
            font.pixelSize: 14
            textFormat: Text.PlainText
            color: worldPreview.error !== "" ? "#ff9a8a" : (worldPreview.flash ? "#9be8b0" : "#d8d8d8")
            text: worldPreview.error !== ""
                ? worldPreview.worldName + ": reload failed, still showing the last good version\n" + worldPreview.error
                : worldPreview.worldName + (worldPreview.flash ? ": reloaded" : "  (R reloads, Esc quits)")
        }
    }
    Shortcut { sequence: "R"; context: Qt.ApplicationShortcut; onActivated: worldPreview.reloadNow() }
    Shortcut { sequence: "Escape"; context: Qt.ApplicationShortcut; onActivated: Qt.quit() }
}
