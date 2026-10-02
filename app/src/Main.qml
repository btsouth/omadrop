import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: app

    title: qsTr("Omadrop")
    width: 1080
    height: 760
    minimumWidth: 780
    minimumHeight: 580
    color: app.cBg
    visible: !backend.playing && !backend.busy

    readonly property color cBg: "#131110"
    readonly property color cPanel: "#1c1917"
    readonly property color cPanelHover: "#23201d"
    readonly property color cCard: "#1f1c19"
    readonly property color cBorder: "#2e2a25"
    readonly property color cBorderSoft: "#262220"
    readonly property color cText: "#f4efe7"
    readonly property color cTextDim: "#aaa29a"
    readonly property color cTextMute: "#7b746c"
    readonly property color cWarn: "#e08a7d"
    readonly property color cErrorBg: "#2a1a17"
    readonly property color cErrorBorder: "#6e352d"
    readonly property color cErrorText: "#f0b4a9"
    readonly property color cAccent: (typeof theme !== "undefined" && theme && theme.accent
                                      && String(theme.accent).length > 0)
                                     ? String(theme.accent) : "#f2a65a"

    function accentAlpha(a) {
        var c = app.cAccent
        return Qt.rgba(c.r, c.g, c.b, a)
    }

    readonly property string modeName: backend.mode ? String(backend.mode).toLowerCase() : "milkdrop"
    readonly property bool milkdropMode: app.modeName === "milkdrop"
    readonly property bool omarchyMode: app.modeName === "omarchy"
    readonly property var effects: backend.effects ? backend.effects : []
    readonly property bool hasError: backend.error ? String(backend.error).length > 0 : false

    property string filter: "all"
    property bool helpVisible: false
    readonly property bool typing: searchField.activeFocus

    readonly property int enabledCount: {
        var n = 0
        for (var i = 0; i < app.effects.length; i++)
            if (!app.effects[i].hidden) n++
        return n
    }
    readonly property int favoriteCount: {
        var n = 0
        for (var i = 0; i < app.effects.length; i++)
            if (app.effects[i].favorite) n++
        return n
    }
    readonly property var visibleEffects: {
        var out = []
        var q = searchField ? String(searchField.text).trim().toLowerCase() : ""
        for (var i = 0; i < app.effects.length; i++) {
            var e = app.effects[i]
            if (!e) continue
            if (app.filter === "favorites" && !e.favorite) continue
            if (app.filter === "hidden" && !e.hidden) continue
            if (q.length > 0) {
                var name = e.name ? String(e.name).toLowerCase() : ""
                if (name.indexOf(q) < 0) continue
            }
            out.push(e)
        }
        return out
    }
    readonly property string emptyHint: {
        if (app.effects.length === 0) return qsTr("No effects were found.")
        if (app.filter === "favorites") return qsTr("No favorites yet. Star an effect to give it priority.")
        if (app.filter === "hidden") return qsTr("No effects are hidden.")
        if (String(searchField ? searchField.text : "").length > 0) return qsTr("No effects match your search.")
        return qsTr("No effects are available.")
    }

    readonly property bool selectedAvailable: app.milkdropMode ? backend.milkdropAvailable : backend.omarchyAvailable
    readonly property bool playEnabled: app.milkdropMode
                                        ? backend.milkdropAvailable
                                        : (backend.omarchyAvailable && app.enabledCount > 0)
    readonly property string selectedSummary: {
        if (app.milkdropMode) return qsTr("MilkDrop — 21 authored scenes")
        if (app.effects.length > 0)
            return qsTr("Omarchy — %1 of %2 effects enabled").arg(app.enabledCount).arg(app.effects.length)
        return qsTr("Omarchy — music screensaver")
    }

    function displayIs(value) {
        var current = backend.display ? String(backend.display).toLowerCase() : ""
        if (current.length === 0) current = "single"
        return current === value
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
        enabled: !app.typing && !app.helpVisible
        onActivated: if (app.playEnabled) backend.play()
    }
    Shortcut {
        sequence: "Q"
        enabled: !app.typing && !app.helpVisible
        onActivated: Qt.quit()
    }
    Shortcut {
        sequence: "?"
        enabled: !app.typing
        onActivated: app.helpVisible = !app.helpVisible
    }
    Shortcut {
        sequence: "Escape"
        enabled: app.helpVisible
        onActivated: app.helpVisible = false
    }

    MouseArea {
        anchors.fill: parent
        onClicked: searchField.focus = false
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 26
        spacing: 18

        RowLayout {
            Layout.fillWidth: true
            spacing: 18

            ColumnLayout {
                spacing: 2
                Label {
                    text: qsTr("Omadrop")
                    color: app.cText
                    font.pixelSize: 30
                    font.weight: Font.DemiBold
                }
                Label {
                    text: qsTr("Music into motion")
                    color: app.cTextDim
                    font.pixelSize: 13
                }
            }

            Item { Layout.fillWidth: true }

            ColumnLayout {
                spacing: 2
                Layout.alignment: Qt.AlignVCenter
                Label {
                    text: qsTr("Press Esc to return to controls")
                    color: app.cTextDim
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignRight
                    Layout.alignment: Qt.AlignRight
                }
                Label {
                    text: qsTr("Omarchy exits on any key")
                    color: app.cTextMute
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignRight
                    Layout.alignment: Qt.AlignRight
                }
            }

            Rectangle {
                Layout.preferredWidth: 34
                Layout.preferredHeight: 34
                Layout.alignment: Qt.AlignVCenter
                radius: 9
                color: helpHover.hovered ? app.cPanelHover : "transparent"
                border.width: 1
                border.color: app.cBorder
                Text {
                    anchors.centerIn: parent
                    text: "?"
                    color: app.cTextDim
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                }
                HoverHandler { id: helpHover }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: app.helpVisible = !app.helpVisible
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            visible: app.hasError
            Layout.preferredHeight: 60
            radius: 12
            color: app.cErrorBg
            border.width: 1
            border.color: app.cErrorBorder

            RowLayout {
                anchors.fill: parent
                anchors.margins: 13
                spacing: 12

                Label {
                    Layout.fillWidth: true
                    Layout.maximumHeight: 34
                    text: backend.error || ""
                    color: app.cErrorText
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }

                Rectangle {
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 28
                    radius: 8
                    color: errCloseHover.hovered ? "#3a231f" : "transparent"
                    border.width: 1
                    border.color: app.cErrorBorder
                    Text {
                        anchors.centerIn: parent
                        text: "×"
                        color: app.cErrorText
                        font.pixelSize: 16
                    }
                    HoverHandler { id: errCloseHover }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: backend.clearError()
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 118

            Row {
                anchors.fill: parent
                spacing: 14

                Rectangle {
                    id: milkdropCard
                    width: (parent.width - 14) / 2
                    height: parent.height
                    radius: 14
                    color: app.milkdropMode ? app.accentAlpha(0.10)
                                             : (mdHover.hovered ? app.cPanelHover : app.cPanel)
                    border.width: app.milkdropMode ? 2 : 1
                    border.color: app.milkdropMode ? app.cAccent : app.cBorder

                    HoverHandler { id: mdHover }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: backend.setMode("milkdrop")
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 14

                        Item {
                            Layout.preferredWidth: 44
                            Layout.preferredHeight: 44
                            Rectangle {
                                anchors.centerIn: parent
                                width: 44; height: 44; radius: 22
                                color: "transparent"
                                border.width: 1
                                border.color: app.accentAlpha(0.55)
                            }
                            Rectangle {
                                anchors.centerIn: parent
                                width: 30; height: 30; radius: 15
                                color: "transparent"
                                border.width: 1
                                border.color: app.accentAlpha(0.8)
                            }
                            Rectangle {
                                anchors.centerIn: parent
                                width: 16; height: 16; radius: 8
                                color: app.accentAlpha(0.95)
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 3
                            Label {
                                Layout.fillWidth: true
                                text: qsTr("MilkDrop")
                                color: app.cText
                                font.pixelSize: 17
                                font.weight: Font.DemiBold
                            }
                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Authored visuals · 21 scenes")
                                color: app.cTextDim
                                font.pixelSize: 12
                                elide: Text.ElideRight
                            }
                        }

                        Rectangle {
                            Layout.alignment: Qt.AlignTop
                            Layout.preferredWidth: mdChip.implicitWidth + 16
                            Layout.preferredHeight: 22
                            radius: 6
                            color: backend.milkdropAvailable ? app.accentAlpha(0.14) : "transparent"
                            border.width: 1
                            border.color: backend.milkdropAvailable ? app.accentAlpha(0.5) : app.cBorder
                            Label {
                                id: mdChip
                                anchors.centerIn: parent
                                text: backend.milkdropAvailable ? qsTr("Ready") : qsTr("Not installed")
                                color: backend.milkdropAvailable ? app.cText : app.cTextMute
                                font.pixelSize: 11
                            }
                        }
                    }
                }

                Rectangle {
                    id: omarchyCard
                    width: (parent.width - 14) / 2
                    height: parent.height
                    radius: 14
                    color: app.omarchyMode ? app.accentAlpha(0.10)
                                            : (omHover.hovered ? app.cPanelHover : app.cPanel)
                    border.width: app.omarchyMode ? 2 : 1
                    border.color: app.omarchyMode ? app.cAccent : app.cBorder

                    HoverHandler { id: omHover }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: backend.setMode("omarchy")
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 14

                        Item {
                            Layout.preferredWidth: 44
                            Layout.preferredHeight: 44
                            Grid {
                                anchors.centerIn: parent
                                columns: 3
                                spacing: 3
                                Repeater {
                                    model: 9
                                    Rectangle {
                                        width: 10; height: 10; radius: 2
                                        color: app.accentAlpha(index % 3 === 0 ? 0.95
                                                                               : (index % 2 === 0 ? 0.6 : 0.35))
                                    }
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 3
                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Omarchy")
                                color: app.cText
                                font.pixelSize: 17
                                font.weight: Font.DemiBold
                            }
                            Label {
                                Layout.fillWidth: true
                                text: app.effects.length > 0
                                      ? qsTr("Music screensaver · %1 effects").arg(app.effects.length)
                                      : qsTr("Music screensaver")
                                color: app.cTextDim
                                font.pixelSize: 12
                                elide: Text.ElideRight
                            }
                        }

                        Rectangle {
                            Layout.alignment: Qt.AlignTop
                            Layout.preferredWidth: omChip.implicitWidth + 16
                            Layout.preferredHeight: 22
                            radius: 6
                            color: backend.omarchyAvailable ? app.accentAlpha(0.14) : "transparent"
                            border.width: 1
                            border.color: backend.omarchyAvailable ? app.accentAlpha(0.5) : app.cBorder
                            Label {
                                id: omChip
                                anchors.centerIn: parent
                                text: backend.omarchyAvailable ? qsTr("Ready") : qsTr("Not installed")
                                color: backend.omarchyAvailable ? app.cText : app.cTextMute
                                font.pixelSize: 11
                            }
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Rectangle {
                anchors.fill: parent
                visible: app.milkdropMode
                radius: 14
                color: app.cPanel
                border.width: 1
                border.color: app.cBorder

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 22
                    spacing: 14
                    visible: backend.milkdropAvailable

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Beautiful authored presets, adapted to the music")
                        color: app.cText
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }
                    Label {
                        Layout.fillWidth: true
                        Layout.maximumHeight: 76
                        text: qsTr("Album artwork opens the show, then dissolves into authored scenes. Omadrop adds continuous audio response while you listen — the presets, shaders and feedback are the original authored work.")
                        color: app.cTextDim
                        font.pixelSize: 13
                        wrapMode: Text.WordWrap
                        maximumLineCount: 3
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 54
                        radius: 10
                        color: app.cPanelHover
                        border.width: 1
                        border.color: app.cBorderSoft

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 16
                            spacing: 12

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1
                                Label {
                                    text: qsTr("ASCII characters")
                                    color: app.cText
                                    font.pixelSize: 14
                                }
                                Label {
                                    text: qsTr("Render the visuals as text glyphs")
                                    color: app.cTextMute
                                    font.pixelSize: 12
                                }
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
                                    color: backend.ascii ? "#ffffff" : app.cTextMute
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
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Keys while playing")
                        color: app.cTextMute
                        font.pixelSize: 12
                    }

                    Row {
                        spacing: 8

                        Rectangle {
                            height: 26; radius: 7
                            width: key1.implicitWidth + 18
                            color: app.cPanelHover
                            border.width: 1
                            border.color: app.cBorderSoft
                            Row {
                                id: key1
                                anchors.centerIn: parent
                                spacing: 7
                                Text { text: "N / P"; color: app.cAccent; font.pixelSize: 12; font.family: "monospace" }
                                Text { text: qsTr("Scene"); color: app.cTextDim; font.pixelSize: 12 }
                            }
                        }
                        Rectangle {
                            height: 26; radius: 7
                            width: key2.implicitWidth + 18
                            color: app.cPanelHover
                            border.width: 1
                            border.color: app.cBorderSoft
                            Row {
                                id: key2
                                anchors.centerIn: parent
                                spacing: 7
                                Text { text: "O"; color: app.cAccent; font.pixelSize: 12; font.family: "monospace" }
                                Text { text: qsTr("Audio response"); color: app.cTextDim; font.pixelSize: 12 }
                            }
                        }
                        Rectangle {
                            height: 26; radius: 7
                            width: key3.implicitWidth + 18
                            color: app.cPanelHover
                            border.width: 1
                            border.color: app.cBorderSoft
                            Row {
                                id: key3
                                anchors.centerIn: parent
                                spacing: 7
                                Text { text: "F11"; color: app.cAccent; font.pixelSize: 12; font.family: "monospace" }
                                Text { text: qsTr("Fullscreen"); color: app.cTextDim; font.pixelSize: 12 }
                            }
                        }
                        Rectangle {
                            height: 26; radius: 7
                            width: key4.implicitWidth + 18
                            color: app.cPanelHover
                            border.width: 1
                            border.color: app.cBorderSoft
                            Row {
                                id: key4
                                anchors.centerIn: parent
                                spacing: 7
                                Text { text: "Esc"; color: app.cAccent; font.pixelSize: 12; font.family: "monospace" }
                                Text { text: qsTr("Back to controls"); color: app.cTextDim; font.pixelSize: 12 }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }

                ColumnLayout {
                    anchors.centerIn: parent
                    width: Math.min(420, parent.width - 60)
                    spacing: 8
                    visible: !backend.milkdropAvailable

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("MilkDrop isn't available")
                        color: app.cText
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                    }
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("The MilkDrop renderer isn't installed on this system. Install it, or switch to Omarchy effects above.")
                        color: app.cTextDim
                        font.pixelSize: 13
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }

            Rectangle {
                anchors.fill: parent
                visible: app.omarchyMode
                radius: 14
                color: app.cPanel
                border.width: 1
                border.color: app.cBorder

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 22
                    spacing: 12
                    visible: backend.omarchyAvailable

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        TextField {
                            id: searchField
                            Layout.fillWidth: true
                            placeholderText: qsTr("Search effects")
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

                        Label {
                            Layout.alignment: Qt.AlignVCenter
                            text: qsTr("%1 enabled").arg(app.enabledCount)
                            color: app.cTextMute
                            font.pixelSize: 12
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Rectangle {
                            Layout.preferredHeight: 30
                            Layout.preferredWidth: segAll.implicitWidth + 26
                            radius: 8
                            color: app.filter === "all" ? app.accentAlpha(0.16) : (segAllHover.hovered ? app.cPanelHover : "transparent")
                            border.width: 1
                            border.color: app.filter === "all" ? app.cAccent : app.cBorder
                            Text {
                                id: segAll
                                anchors.centerIn: parent
                                text: qsTr("All")
                                color: app.filter === "all" ? app.cText : app.cTextDim
                                font.pixelSize: 13
                            }
                            HoverHandler { id: segAllHover }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: app.filter = "all"
                            }
                        }

                        Rectangle {
                            Layout.preferredHeight: 30
                            Layout.preferredWidth: segFav.implicitWidth + 26
                            radius: 8
                            color: app.filter === "favorites" ? app.accentAlpha(0.16) : (segFavHover.hovered ? app.cPanelHover : "transparent")
                            border.width: 1
                            border.color: app.filter === "favorites" ? app.cAccent : app.cBorder
                            Text {
                                id: segFav
                                anchors.centerIn: parent
                                text: qsTr("Favorites")
                                color: app.filter === "favorites" ? app.cText : app.cTextDim
                                font.pixelSize: 13
                            }
                            HoverHandler { id: segFavHover }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: app.filter = "favorites"
                            }
                        }

                        Rectangle {
                            Layout.preferredHeight: 30
                            Layout.preferredWidth: segHidden.implicitWidth + 26
                            radius: 8
                            color: app.filter === "hidden" ? app.accentAlpha(0.16) : (segHiddenHover.hovered ? app.cPanelHover : "transparent")
                            border.width: 1
                            border.color: app.filter === "hidden" ? app.cAccent : app.cBorder
                            Text {
                                id: segHidden
                                anchors.centerIn: parent
                                text: qsTr("Hidden")
                                color: app.filter === "hidden" ? app.cText : app.cTextDim
                                font.pixelSize: 13
                            }
                            HoverHandler { id: segHiddenHover }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: app.filter = "hidden"
                            }
                        }

                        Item { Layout.fillWidth: true }

                        Label {
                            Layout.alignment: Qt.AlignVCenter
                            text: qsTr("Favorites get priority; every effect still gets one turn per round.")
                            color: app.cTextMute
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        GridView {
                            id: effectsView
                            anchors.fill: parent
                            clip: true
                            readonly property int columns: Math.max(1, Math.floor(width / 296))
                            cellWidth: width / columns
                            cellHeight: 140
                            model: app.visibleEffects
                            delegate: effectDelegate
                            boundsBehavior: Flickable.StopAtBounds
                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                        }

                        Label {
                            anchors.centerIn: parent
                            width: parent.width - 40
                            visible: app.visibleEffects.length === 0
                            text: app.emptyHint
                            color: app.cTextMute
                            font.pixelSize: 13
                            wrapMode: Text.WordWrap
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }

                ColumnLayout {
                    anchors.centerIn: parent
                    width: Math.min(430, parent.width - 60)
                    spacing: 8
                    visible: !backend.omarchyAvailable

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Omarchy screensaver isn't available")
                        color: app.cText
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                    }
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("The Omarchy music screensaver isn't installed on this system, so its effects can't be shown. Install it, or switch to MilkDrop above.")
                        color: app.cTextDim
                        font.pixelSize: 13
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
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
                    Layout.preferredHeight: 52
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
                            color: app.playEnabled ? "#1a1512" : app.cTextMute
                            font.pixelSize: 15
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Play")
                            color: app.playEnabled ? "#1a1512" : app.cTextMute
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

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 3
                    Label {
                        Layout.fillWidth: true
                        text: app.selectedSummary
                        color: app.cText
                        font.pixelSize: 15
                        elide: Text.ElideRight
                    }
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Play music in any app; Omadrop follows your system audio.")
                        color: app.cTextMute
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }
                }

                ColumnLayout {
                    spacing: 7
                    Layout.alignment: Qt.AlignVCenter

                    RowLayout {
                        spacing: 8
                        Layout.alignment: Qt.AlignRight
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
                                text: qsTr("Single")
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
                                text: qsTr("All")
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
                        Layout.alignment: Qt.AlignRight
                        text: qsTr("Space Play  ·  ? Help  ·  Q Quit")
                        color: app.cTextMute
                        font.pixelSize: 11
                    }
                }
            }
        }
    }

    Component {
        id: effectDelegate

        Item {
            width: effectsView.cellWidth
            height: effectsView.cellHeight

            Rectangle {
                id: effectCard
                anchors.fill: parent
                anchors.margins: 6
                radius: 12
                color: cardHover.hovered ? app.cPanelHover : app.cCard
                border.width: 1
                border.color: (modelData && modelData.hidden) ? app.cBorderSoft : app.cBorder
                opacity: (modelData && modelData.hidden) ? 0.72 : 1.0

                HoverHandler { id: cardHover }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 8

                    Label {
                        Layout.fillWidth: true
                        text: modelData && modelData.name ? String(modelData.name)
                                                          : (modelData ? String(modelData.slug) : "")
                        color: app.cText
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.maximumHeight: 52
                        text: modelData && modelData.description ? String(modelData.description) : ""
                        color: app.cTextDim
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                        maximumLineCount: 3
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignTop
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Rectangle {
                            visible: modelData && modelData.hidden
                            Layout.preferredHeight: 22
                            Layout.preferredWidth: hiddenChip.implicitWidth + 14
                            radius: 6
                            color: "transparent"
                            border.width: 1
                            border.color: app.cBorderSoft
                            Label {
                                id: hiddenChip
                                anchors.centerIn: parent
                                text: qsTr("Hidden")
                                color: app.cTextMute
                                font.pixelSize: 11
                            }
                        }

                        Item { Layout.fillWidth: true }

                        Rectangle {
                            Layout.preferredWidth: 30
                            Layout.preferredHeight: 30
                            radius: 8
                            color: starHover.hovered ? app.cPanelHover : "transparent"
                            border.width: 1
                            border.color: starHover.hovered ? app.cBorder : "transparent"
                            Text {
                                anchors.centerIn: parent
                                text: (modelData && modelData.favorite) ? "★" : "☆"
                                color: (modelData && modelData.favorite) ? app.cAccent : app.cTextMute
                                font.pixelSize: 16
                            }
                            HoverHandler { id: starHover }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: if (modelData && modelData.slug) backend.toggleFavorite(String(modelData.slug))
                            }
                            ToolTip.visible: starHover.hovered
                            ToolTip.delay: 450
                            ToolTip.text: (modelData && modelData.favorite)
                                          ? qsTr("Remove from favorites")
                                          : qsTr("Add to favorites")
                        }

                        Rectangle {
                            Layout.preferredWidth: 30
                            Layout.preferredHeight: 30
                            radius: 8
                            color: eyeHover.hovered ? app.cPanelHover : "transparent"
                            border.width: 1
                            border.color: eyeHover.hovered ? app.cBorder : "transparent"
                            Text {
                                anchors.centerIn: parent
                                text: (modelData && modelData.hidden) ? "⊘" : "◉"
                                color: (modelData && modelData.hidden) ? app.cTextMute : app.cTextDim
                                font.pixelSize: 15
                            }
                            HoverHandler { id: eyeHover }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: if (modelData && modelData.slug) backend.toggleHidden(String(modelData.slug))
                            }
                            ToolTip.visible: eyeHover.hovered
                            ToolTip.delay: 450
                            ToolTip.text: (modelData && modelData.hidden)
                                          ? qsTr("Show in rotation")
                                          : qsTr("Hide from rotation")
                        }

                        Rectangle {
                            Layout.preferredHeight: 30
                            Layout.preferredWidth: previewLabel.implicitWidth + 22
                            radius: 8
                            color: previewHover.hovered ? app.accentAlpha(0.24) : app.accentAlpha(0.12)
                            border.width: 1
                            border.color: app.accentAlpha(0.4)
                            Label {
                                id: previewLabel
                                anchors.centerIn: parent
                                text: qsTr("Preview")
                                color: app.cText
                                font.pixelSize: 12
                            }
                            HoverHandler { id: previewHover }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: if (modelData && modelData.slug) backend.preview(String(modelData.slug))
                            }
                            ToolTip.visible: previewHover.hovered
                            ToolTip.delay: 450
                            ToolTip.text: qsTr("Play this effect on repeat until any key returns")
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: app.helpVisible
        z: 100
        color: "#cc0e0d0c"

        MouseArea {
            anchors.fill: parent
            onClicked: app.helpVisible = false
        }

        Rectangle {
            anchors.centerIn: parent
            width: Math.min(520, parent.width - 80)
            height: helpColumn.implicitHeight + 44
            radius: 16
            color: app.cPanel
            border.width: 1
            border.color: app.cBorder

            MouseArea { anchors.fill: parent }

            ColumnLayout {
                id: helpColumn
                anchors.fill: parent
                anchors.margins: 22
                spacing: 9

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Controls")
                        color: app.cText
                        font.pixelSize: 20
                        font.weight: Font.DemiBold
                    }
                    Label {
                        text: qsTr("?  to close")
                        color: app.cTextMute
                        font.pixelSize: 12
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Space")
                    color: app.cAccent
                    font.pixelSize: 13
                    font.family: "monospace"
                }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Play the selected mode")
                    color: app.cTextDim
                    font.pixelSize: 13
                }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Esc")
                    color: app.cAccent
                    font.pixelSize: 13
                    font.family: "monospace"
                }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Return here from the visuals (Omarchy exits on any key)")
                    color: app.cTextDim
                    font.pixelSize: 13
                }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Q")
                    color: app.cAccent
                    font.pixelSize: 13
                    font.family: "monospace"
                }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Quit Omadrop")
                    color: app.cTextDim
                    font.pixelSize: 13
                }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Omarchy effects")
                    color: app.cAccent
                    font.pixelSize: 13
                    font.family: "monospace"
                }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Star to give an effect priority, eye to hide it from the rotation, Preview to repeat one until you return.")
                    color: app.cTextDim
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                }
            }
        }
    }
}
