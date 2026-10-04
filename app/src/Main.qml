import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: app

    title: qsTr("Omadrop")
    width: 1120
    height: 720
    minimumWidth: 900
    minimumHeight: 640
    maximumWidth: 1120
    maximumHeight: 720
    color: app.cBg
    // Stay up until the renderer window is mapped, so there is never a bare desktop.
    visible: backend.curtainVisible || (!backend.playing && !backend.busy)
    property string focusedKey: ""
    property bool detailsOpen: false
    readonly property int focusedIndex: {
        for (var i = 0; i < app.visibleItems.length; ++i)
            if (String(app.visibleItems[i].key) === app.focusedKey) return i
        return -1
    }
    function moveCard(delta) {
        if (!app.visibleItems.length) return
        var next = app.focusedIndex < 0 ? 0
                   : Math.max(0, Math.min(app.visibleItems.length - 1, app.focusedIndex + delta))
        app.focusedKey = String(app.visibleItems[next].key)
        grid.positionViewAtIndex(next, GridView.Contain)
    }
    function hideFocused() {
        if (app.focusedIndex < 0) return
        var item = app.visibleItems[app.focusedIndex]
        if (item.scene) backend.toggleSceneHidden(item.number)
    }

    // Everything is derived from the live Omarchy theme and fades on a switch.
    property color tBg: theme.background
    property color tFg: theme.foreground
    property color tAccent: theme.accent
    property color tRed: theme.red
    Behavior on tBg { ColorAnimation { duration: 260; easing.type: Easing.InOutQuad } }
    Behavior on tFg { ColorAnimation { duration: 260; easing.type: Easing.InOutQuad } }
    Behavior on tAccent { ColorAnimation { duration: 260; easing.type: Easing.InOutQuad } }
    Behavior on tRed { ColorAnimation { duration: 260; easing.type: Easing.InOutQuad } }

    function mix(from, to, amount) {
        return Qt.rgba(from.r + (to.r - from.r) * amount, from.g + (to.g - from.g) * amount,
                       from.b + (to.b - from.b) * amount, 1)
    }

    readonly property color cBg: app.tBg
    readonly property color cPanel: app.mix(app.tBg, app.tFg, 0.045)
    readonly property color cPanelHover: app.mix(app.tBg, app.tFg, 0.09)
    readonly property color cCard: app.mix(app.tBg, app.tFg, 0.065)
    readonly property color cBorder: app.mix(app.tBg, app.tFg, 0.15)
    readonly property color cBorderSoft: app.mix(app.tBg, app.tFg, 0.10)
    readonly property color cText: app.tFg
    readonly property color cTextDim: app.mix(app.tFg, app.tBg, 0.32)
    readonly property color cTextMute: app.mix(app.tFg, app.tBg, 0.52)
    readonly property color cWarn: app.tRed
    readonly property color cErrorBg: app.mix(app.tBg, app.tRed, 0.14)
    readonly property color cErrorBorder: app.mix(app.tBg, app.tRed, 0.45)
    readonly property color cErrorText: app.cText
    readonly property color cAccent: app.tAccent
    // Text drawn on an accent fill.
    readonly property color cOnAccent: (0.2126 * app.tAccent.r + 0.7152 * app.tAccent.g
                                        + 0.0722 * app.tAccent.b) > 0.55 ? "#141414" : "#ffffff"

    function accentAlpha(a) {
        var c = app.cAccent
        return Qt.rgba(c.r, c.g, c.b, a)
    }

    readonly property string modeName: backend.mode ? String(backend.mode).toLowerCase() : "milkdrop"
    readonly property bool milkdropMode: app.modeName === "milkdrop"
    readonly property bool omarchyMode: app.modeName === "omarchy"
    readonly property var scenes: backend.scenes ? backend.scenes : []
    readonly property bool hasError: backend.error ? String(backend.error).length > 0 : false
    readonly property bool typing: searchField.activeFocus

    // MilkDrop cards. Osaka is one indefinitely looping scene.
    readonly property var items: {
        var out = []
        if (app.milkdropMode) {
            for (var i = 0; i < app.scenes.length; i++) {
                var s = app.scenes[i]
                if (!s) continue
                out.push({ key: s.number, number: s.number,
                           name: s.label ? String(s.label) : ("Scene " + s.number),
                           description: s.description ? String(s.description) : "",
                           thumbnail: s.thumbnail ? String(s.thumbnail) : "",
                           hidden: !!s.hidden, scene: true })
            }
        }
        return out
    }

    readonly property string query: searchField ? String(searchField.text).trim().toLowerCase() : ""

    readonly property var visibleItems: {
        var out = []
        for (var i = 0; i < app.items.length; i++) {
            var it = app.items[i]
            if (app.query.length > 0 && it.name.toLowerCase().indexOf(app.query) < 0) continue
            out.push(it)
        }
        out.sort(function(a, b) {
            if (a.hidden !== b.hidden) return a.hidden ? 1 : -1
            return a.name.toLowerCase().localeCompare(b.name.toLowerCase())
        })
        return out
    }

    readonly property string emptyHint: {
        if (app.omarchyMode) return qsTr("Osaka Jade\nA living street, listening to your music. Press Play.")
        if (app.items.length === 0) return qsTr("No scenes were found.")
        if (app.query.length > 0) return qsTr("Nothing matches your search.")
        return qsTr("Nothing is available.")
    }

    readonly property bool playEnabled: app.milkdropMode
                                        ? backend.milkdropAvailable
                                        : backend.omarchyAvailable
    readonly property string rotationSummary: app.milkdropMode ? qsTr("%1 scenes").arg(app.scenes.length) : qsTr("Osaka Jade")
    readonly property string modeReason: {
        if (!backend.milkdropAvailable && !backend.omarchyAvailable)
            return qsTr("MilkDrop and Omarchy aren't installed")
        if (!backend.milkdropAvailable) return qsTr("MilkDrop isn't installed")
        if (!backend.omarchyAvailable) return qsTr("Omarchy isn't installed")
        return ""
    }

    function displayIs(value) {
        var current = backend.display ? String(backend.display).toLowerCase() : ""
        if (current.length === 0) current = "single"
        return current === value
    }

    function switchMode() {
        if (app.milkdropMode) {
            if (backend.omarchyAvailable) backend.setMode("omarchy")
        } else if (backend.milkdropAvailable) {
            backend.setMode("milkdrop")
        }
    }

    function openItem(item) {
        if (!item) return
        if (app.milkdropMode) backend.playScene(item.number)
    }

    Connections {
        target: backend
        function onShowControls() {
            app.show()
            app.raise()
            app.requestActivate()
        }
    }

    onClosing: function(close) {
        close.accepted = true
        Qt.quit()
    }

    Shortcut {
        sequence: "Space"
        enabled: !app.typing
        onActivated: if (app.playEnabled) backend.play()
    }
    Shortcut {
        sequence: "Return"
        enabled: !app.typing
        onActivated: {
            if (app.focusedIndex >= 0) app.openItem(app.visibleItems[app.focusedIndex])
            else if (app.playEnabled) backend.play()
        }
    }
    Shortcut {
        sequence: "Tab"
        enabled: !app.typing
        onActivated: app.switchMode()
    }
    Shortcut {
        sequence: "Ctrl+F"
        onActivated: searchField.forceActiveFocus()
    }
    Shortcut {
        sequence: "/"
        enabled: !app.typing
        onActivated: searchField.forceActiveFocus()
    }
    Shortcut {
        sequence: "Escape"
        onActivated: Qt.quit()
    }
    Shortcut {
        sequence: "Q"
        enabled: !app.typing
        onActivated: Qt.quit()
    }

    Shortcut { sequence: "Left"; enabled: !app.typing; onActivated: app.moveCard(-1) }
    Shortcut { sequence: "Right"; enabled: !app.typing; onActivated: app.moveCard(1) }
    Shortcut { sequence: "Up"; enabled: !app.typing; onActivated: app.moveCard(-4) }
    Shortcut { sequence: "Down"; enabled: !app.typing; onActivated: app.moveCard(4) }
    Shortcut { sequence: "Enter"; enabled: !app.typing; onActivated: {
        if (app.focusedIndex >= 0) app.openItem(app.visibleItems[app.focusedIndex])
    } }
    Shortcut { sequence: "H"; enabled: !app.typing; onActivated: app.hideFocused() }

    MouseArea {
        anchors.fill: parent
        onClicked: searchField.focus = false
    }

    ColumnLayout {
        id: content
        // Dim while visuals are starting; fade in when the controls return.
        opacity: backend.curtainVisible ? 0.35 : (app.visible ? 1 : 0)
        scale: backend.curtainVisible ? 0.985 : 1
        Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }
        Behavior on scale { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }
        anchors.fill: parent
        anchors.margins: 26
        spacing: 14

        RowLayout {
            Layout.fillWidth: true
            spacing: 18

            Label {
                id: wordmark
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Omadrop")
                color: app.cText
                font.pixelSize: 30
                font.weight: Font.DemiBold
            }

            Item { Layout.fillWidth: true }

            ColumnLayout {
                Layout.alignment: Qt.AlignVCenter
                spacing: 4

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    implicitWidth: switchRow.implicitWidth + 8
                    implicitHeight: 38
                    radius: 11
                    color: app.cPanel
                    border.width: 1
                    border.color: app.cBorder

                    Row {
                        id: switchRow
                        anchors.centerIn: parent
                        spacing: 4

                        Rectangle {
                            width: mdSegment.implicitWidth + 28
                            height: 30
                            radius: 8
                            color: app.milkdropMode ? app.accentAlpha(0.18)
                                                    : (mdSegHover.hovered && backend.milkdropAvailable ? app.cPanelHover : "transparent")
                            border.width: app.milkdropMode ? 1 : 0
                            border.color: app.cAccent
                            opacity: backend.milkdropAvailable ? 1.0 : 0.45
                            Text {
                                id: mdSegment
                                anchors.centerIn: parent
                                text: qsTr("MilkDrop")
                                color: app.milkdropMode ? app.cText : app.cTextDim
                                font.pixelSize: 14
                                font.weight: app.milkdropMode ? Font.DemiBold : Font.Normal
                            }
                            HoverHandler { id: mdSegHover }
                            MouseArea {
                                anchors.fill: parent
                                enabled: backend.milkdropAvailable
                                cursorShape: backend.milkdropAvailable ? Qt.PointingHandCursor : Qt.ArrowCursor
                                onClicked: backend.setMode("milkdrop")
                            }
                        }

                        Rectangle {
                            width: omSegment.implicitWidth + 28
                            height: 30
                            radius: 8
                            color: app.omarchyMode ? app.accentAlpha(0.18)
                                                   : (omSegHover.hovered && backend.omarchyAvailable ? app.cPanelHover : "transparent")
                            border.width: app.omarchyMode ? 1 : 0
                            border.color: app.cAccent
                            opacity: backend.omarchyAvailable ? 1.0 : 0.45
                            Text {
                                id: omSegment
                                anchors.centerIn: parent
                                text: qsTr("Omarchy")
                                color: app.omarchyMode ? app.cText : app.cTextDim
                                font.pixelSize: 14
                                font.weight: app.omarchyMode ? Font.DemiBold : Font.Normal
                            }
                            HoverHandler { id: omSegHover }
                            MouseArea {
                                anchors.fill: parent
                                enabled: backend.omarchyAvailable
                                cursorShape: backend.omarchyAvailable ? Qt.PointingHandCursor : Qt.ArrowCursor
                                onClicked: backend.setMode("omarchy")
                            }
                        }
                    }
                }

                Label {
                    Layout.alignment: Qt.AlignHCenter
                    visible: app.modeReason.length > 0
                    text: app.modeReason
                    color: app.cTextMute
                    font.pixelSize: 11
                }
            }

            Item { Layout.fillWidth: true }

            Item {
                Layout.preferredWidth: wordmark.implicitWidth
                Layout.preferredHeight: 1
            }
        }

        Rectangle {
            Layout.fillWidth: true
            visible: app.hasError
            Layout.preferredHeight: 56
            radius: 12
            color: app.cErrorBg
            border.width: 1
            border.color: app.cErrorBorder

            RowLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                height: 32
                spacing: 12

                Label {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    text: backend.error || ""
                    color: app.cErrorText
                    font.pixelSize: 13
                    renderType: Text.NativeRendering
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }

                Button {
                    id: errorDetailsButton
                    Layout.preferredHeight: 32
                    Layout.alignment: Qt.AlignVCenter
                    visible: !!backend.errorDetails
                    text: qsTr("Details")
                    leftPadding: 12
                    rightPadding: 12
                    contentItem: Text {
                        text: errorDetailsButton.text
                        color: app.cErrorText
                        font.pixelSize: 13
                        renderType: Text.NativeRendering
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: 8
                        color: errorDetailsButton.down ? app.mix(app.cErrorBg, app.cText, 0.12)
                               : errorDetailsButton.hovered ? app.mix(app.cErrorBg, app.cText, 0.08)
                               : "transparent"
                        border.width: 1
                        border.color: errorDetailsButton.visualFocus ? app.cAccent : app.cErrorBorder
                    }
                    HoverHandler { cursorShape: Qt.PointingHandCursor }
                    onClicked: app.detailsOpen = !app.detailsOpen
                }

                Button {
                    id: errorCloseButton
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 32
                    Layout.alignment: Qt.AlignVCenter
                    padding: 0
                    Accessible.name: qsTr("Dismiss error")
                    contentItem: Text {
                        text: "×"
                        color: app.cErrorText
                        font.pixelSize: 18
                        renderType: Text.NativeRendering
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: 8
                        color: errorCloseButton.down ? app.mix(app.cErrorBg, app.cText, 0.12)
                               : errorCloseButton.hovered ? app.mix(app.cErrorBg, app.cText, 0.08)
                               : "transparent"
                        border.width: 1
                        border.color: errorCloseButton.visualFocus ? app.cAccent : app.cErrorBorder
                    }
                    HoverHandler { cursorShape: Qt.PointingHandCursor }
                    onClicked: backend.clearError()
                }
            }
        }

        ScrollView {
            id: errorDetailsPanel
            Layout.fillWidth: true
            // Keep short messages compact; long logs scroll within a bounded panel.
            Layout.preferredHeight: Math.min(errorDetailsText.implicitHeight, app.height * 0.25)
            Layout.minimumHeight: Layout.preferredHeight
            Layout.maximumHeight: Layout.preferredHeight
            visible: app.hasError && app.detailsOpen && !!backend.errorDetails
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            background: Rectangle {
                radius: 12
                color: app.cErrorBg
                border.width: 1
                border.color: app.cErrorBorder
            }
            TextArea {
                id: errorDetailsText
                width: errorDetailsPanel.availableWidth
                text: backend.errorDetails
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                color: app.cErrorText
                font.pixelSize: 12
                renderType: Text.NativeRendering
                padding: 14
                background: Item {}
            }
        }

        TextField {
            id: searchField
            Layout.fillWidth: true
            visible: app.milkdropMode
            placeholderText: qsTr("Search scenes")
            color: app.cText
            placeholderTextColor: app.cTextMute
            font.pixelSize: 13
            selectByMouse: true
            focusPolicy: Qt.StrongFocus
            background: Rectangle {
                radius: 9
                color: app.cCard
                border.width: 1
                border.color: searchField.activeFocus ? app.cAccent : app.cBorder
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            GridView {
                id: grid
                anchors.fill: parent
                clip: true
                cellWidth: width / 4
                cellHeight: 194
                model: app.visibleItems
                delegate: cardDelegate
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
            }

            Rectangle {
                anchors.centerIn: parent
                // Leave room for the captions when the error details reduce the viewport.
                width: Math.min(parent.width - 40, 640, Math.max(240, (parent.height - 80) * 16 / 9 + 16))
                height: Math.min(parent.height, osakaPreview.implicitHeight)
                visible: app.omarchyMode
                radius: 12
                color: app.cCard
                border.width: 1
                border.color: app.cBorder

                ColumnLayout {
                    id: osakaPreview
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.margins: 8
                    anchors.topMargin: 0
                    anchors.bottomMargin: 0
                    spacing: 6

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.topMargin: 8
                        Layout.preferredHeight: (osakaPreview.width) * 9 / 16
                        Layout.fillHeight: true
                        Layout.minimumHeight: 0
                        radius: 8
                        clip: true
                        color: app.cPanel
                        Image {
                            anchors.fill: parent
                            source: "qrc:/assets/osaka-jade.jpg"
                            fillMode: Image.PreserveAspectCrop
                            smooth: true
                        }
                    }
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Osaka Jade")
                        color: app.cText
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                    }
                    Label {
                        Layout.fillWidth: true
                        Layout.bottomMargin: 12
                        text: qsTr("A living street, listening to your music. Press Play.")
                        color: app.cTextMute
                        font.pixelSize: 13
                        wrapMode: Text.WordWrap
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                width: parent.width - 40
                visible: !app.omarchyMode && app.visibleItems.length === 0
                text: app.emptyHint
                color: app.cTextMute
                font.pixelSize: 13
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 76
            radius: 14
            color: app.cPanel
            border.width: 1
            border.color: app.cBorder

            RowLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 16

                Rectangle {
                    Layout.preferredWidth: 132
                    Layout.preferredHeight: 48
                    radius: 12
                    color: app.playEnabled ? (playHover.hovered ? Qt.lighter(app.cAccent, 1.08) : app.cAccent)
                                           : app.cPanelHover
                    opacity: app.playEnabled ? 1.0 : 0.55
                    Row {
                        anchors.centerIn: parent
                        spacing: 9
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "▶"
                            color: app.playEnabled ? app.cOnAccent : app.cTextMute
                            font.pixelSize: 15
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Play")
                            color: app.playEnabled ? app.cOnAccent : app.cTextMute
                            font.pixelSize: 16
                            font.weight: Font.DemiBold
                        }
                    }
                    HoverHandler { id: playHover }
                    MouseArea {
                        anchors.fill: parent
                        enabled: app.playEnabled
                        cursorShape: app.playEnabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                        onClicked: backend.play()
                    }
                }

                Label {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    text: app.rotationSummary
                    color: app.cText
                    font.pixelSize: 15
                    elide: Text.ElideRight
                }

                RowLayout {
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 14

                    RowLayout {
                        visible: app.milkdropMode
                        spacing: 8
                        Layout.alignment: Qt.AlignVCenter
                        Label {
                            Layout.alignment: Qt.AlignVCenter
                            text: qsTr("ASCII")
                            color: app.cTextMute
                            font.pixelSize: 12
                        }
                        Rectangle {
                            id: asciiToggle
                            Layout.preferredWidth: 46
                            Layout.preferredHeight: 26
                            radius: 13
                            color: backend.ascii ? app.cAccent : app.cBorder
                            Rectangle {
                                width: 20; height: 20; radius: 10
                                y: 3
                                x: backend.ascii ? asciiToggle.width - width - 3 : 3
                                color: backend.ascii ? app.cOnAccent : app.cTextMute
                                Behavior on x {
                                    NumberAnimation { duration: 120; easing.type: Easing.InOutQuad }
                                }
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: backend.setAscii(!backend.ascii)
                            }
                        }
                    }

                    RowLayout {
                        visible: app.milkdropMode
                        spacing: 8
                        Layout.alignment: Qt.AlignVCenter
                        Label {
                            Layout.alignment: Qt.AlignVCenter
                            text: qsTr("Scene names")
                            color: app.cTextMute
                            font.pixelSize: 12
                        }
                        Rectangle {
                            id: captionsToggle
                            Layout.preferredWidth: 46
                            Layout.preferredHeight: 26
                            radius: 13
                            color: backend.captions ? app.cAccent : app.cBorder
                            Rectangle {
                                width: 20; height: 20; radius: 10
                                y: 3
                                x: backend.captions ? captionsToggle.width - width - 3 : 3
                                color: backend.captions ? app.cOnAccent : app.cTextMute
                                Behavior on x {
                                    NumberAnimation { duration: 120; easing.type: Easing.InOutQuad }
                                }
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: backend.setCaptions(!backend.captions)
                            }
                        }
                    }

                    RowLayout {
                        spacing: 8
                        Layout.alignment: Qt.AlignVCenter
                        Label {
                            Layout.alignment: Qt.AlignVCenter
                            text: qsTr("Display")
                            color: app.cTextMute
                            font.pixelSize: 12
                        }
                        Rectangle {
                            Layout.preferredHeight: 28
                            Layout.preferredWidth: dispSingle.implicitWidth + 22
                            radius: 8
                            color: app.displayIs("single") ? app.accentAlpha(0.16) : (dispSingleHover.hovered ? app.cPanelHover : "transparent")
                            border.width: 1
                            border.color: app.displayIs("single") ? app.cAccent : app.cBorder
                            Text {
                                id: dispSingle
                                anchors.centerIn: parent
                                text: qsTr("This screen")
                                color: app.displayIs("single") ? app.cText : app.cTextDim
                                font.pixelSize: 12
                            }
                            HoverHandler { id: dispSingleHover }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: backend.setDisplay("single")
                            }
                        }
                        Rectangle {
                            Layout.preferredHeight: 28
                            Layout.preferredWidth: dispAll.implicitWidth + 22
                            radius: 8
                            color: app.displayIs("all") ? app.accentAlpha(0.16) : (dispAllHover.hovered ? app.cPanelHover : "transparent")
                            border.width: 1
                            border.color: app.displayIs("all") ? app.cAccent : app.cBorder
                            Text {
                                id: dispAll
                                anchors.centerIn: parent
                                text: qsTr("All screens")
                                color: app.displayIs("all") ? app.cText : app.cTextDim
                                font.pixelSize: 12
                            }
                            HoverHandler { id: dispAllHover }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: backend.setDisplay("all")
                            }
                        }
                    }

                    Label {
                        Layout.alignment: Qt.AlignVCenter
                        text: qsTr("Esc returns here")
                        color: app.cTextMute
                        font.pixelSize: 11
                    }
                }
            }
        }
    }

    Component {
        id: cardDelegate

        Item {
            width: grid.cellWidth
            height: grid.cellHeight

            Rectangle {
                id: card
                anchors.fill: parent
                anchors.margins: 6
                radius: 12
                color: cardHover.hovered ? app.cPanelHover : app.cCard
                border.width: String(modelData.key) === app.focusedKey ? 2 : 1
                border.color: String(modelData.key) === app.focusedKey ? app.cAccent : cardHover.hovered ? app.cAccent : ((modelData && modelData.hidden) ? app.cBorderSoft : app.cBorder)
                opacity: (modelData && modelData.hidden) ? 0.55 : 1.0
                clip: true

                HoverHandler { id: cardHover }
                ToolTip.visible: cardHover.hovered && !!(modelData && modelData.description)
                ToolTip.delay: 700
                ToolTip.text: modelData && modelData.description ? String(modelData.description) : ""

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: app.openItem(modelData)
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 6

                    Rectangle {
                        id: thumb
                        Layout.fillWidth: true
                        // 16:9, derived from the grid column so the layout can't loop on our own width.
                        Layout.preferredHeight: (grid.cellWidth - 28) * 9 / 16
                        radius: 8
                        color: app.cPanel
                        clip: true

                        Image {
                            id: thumbImage
                            anchors.fill: parent
                            source: modelData && modelData.thumbnail ? String(modelData.thumbnail) : ""
                            scale: cardHover.hovered ? 1.03 : 1
                            Behavior on scale { NumberAnimation { duration: 120 } }
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            visible: source != ""
                        }

                        // The MilkDrop ASCII filter, shown on the scenes it will apply to.
                        Image {
                            anchors.fill: parent
                            readonly property bool wanted: backend.ascii && !!(modelData && modelData.scene)
                            source: (wanted || opacity > 0) && thumbImage.visible
                                    ? String(modelData.thumbnail).replace("/scenes/", "/scenes-ascii/").replace(".jpg", ".png")
                                    : ""
                            scale: cardHover.hovered ? 1.03 : 1
                            Behavior on scale { NumberAnimation { duration: 120 } }
                            fillMode: Image.PreserveAspectCrop
                            mipmap: true
                            opacity: wanted ? 1 : 0
                            Behavior on opacity { NumberAnimation { duration: 220; easing.type: Easing.InOutQuad } }
                        }

                        Text {
                            anchors.centerIn: parent
                            opacity: cardHover.hovered || !thumbImage.visible ? 1 : 0
                            Behavior on opacity { NumberAnimation { duration: 120 } }
                            text: "▶"
                            color: app.cText
                            font.pixelSize: 22
                        }

                        Row {
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 6
                            spacing: 6
                            visible: cardHover.hovered || eyeHover.hovered

                            Rectangle {
                                width: 28; height: 28; radius: 8
                                color: eyeHover.hovered ? app.cPanelHover : app.accentAlpha(0.22)
                                border.width: 1
                                border.color: app.accentAlpha(0.5)
                                Text {
                                    anchors.centerIn: parent
                                    text: (modelData && modelData.hidden) ? "⊘" : "◉"
                                    color: (modelData && modelData.hidden) ? app.cTextMute : app.cTextDim
                                    font.pixelSize: 14
                                }
                                HoverHandler { id: eyeHover }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (!modelData) return
                                        if (modelData.scene) backend.toggleSceneHidden(modelData.number)
                                    }
                                }
                                ToolTip.visible: eyeHover.hovered
                                ToolTip.delay: 450
                                ToolTip.text: (modelData && modelData.hidden)
                                              ? qsTr("Show in rotation")
                                              : qsTr("Hide from rotation")
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: modelData && modelData.name ? String(modelData.name) : ""
                        color: app.cText
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }

                }
            }
        }
    }
}
