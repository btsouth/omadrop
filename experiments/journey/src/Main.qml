import QtQuick
import QtQuick.Window
import Journey 1.0

Window {
    width: initialWidth
    height: initialHeight
    visible: true
    visibility: Window.FullScreen
    color: "black"
    title: "Omadrop Journey Study"

    JourneyItem {
        anchors.fill: parent
        backend: journeyBackend
    }
    Shortcut { sequence: "Escape"; context: Qt.ApplicationShortcut; onActivated: Qt.quit() }
    Shortcut { sequence: "Q"; context: Qt.ApplicationShortcut; onActivated: Qt.quit() }
    Shortcut { sequence: "Space"; context: Qt.ApplicationShortcut; onActivated: journeyBackend.togglePause() }
    Shortcut { sequence: "Left"; context: Qt.ApplicationShortcut; onActivated: journeyBackend.scrub(-1) }
    Shortcut { sequence: "Right"; context: Qt.ApplicationShortcut; onActivated: journeyBackend.scrub(1) }
}
