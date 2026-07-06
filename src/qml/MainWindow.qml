import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root
    title: "KDE GDrive"
    width: 800
    height: 600
    visible: true

    onClosing: {
        close.accepted = false
        root.visible = false
    }

    Connections {
        target: mainWindow
        function onSettingsRequested() {
            if (pageStack.depth < 2)
                pageStack.push(settingsPageComp)
        }
    }

    Component { id: detailPageComp; RemoteDetailPage {} }
    Component { id: settingsPageComp; SettingsPage { onAddRemoteClicked: pageStack.push(oauthPageComp) } }
    Component { id: oauthPageComp; OAuthWizard {} }

    pageStack.initialPage: Kirigami.Page {
        id: mainPage
        title: "KDE GDrive"

        onVisibleChanged: if (visible) mainWindow.refreshRemotes()

        actions: [
            Kirigami.Action { text: "Refresh"; icon.name: "view-refresh"; onTriggered: { mainWindow.refreshRemotes(); mainWindow.mountsModel.refresh() } },
            Kirigami.Action { text: "Settings"; icon.name: "settings-configure"; onTriggered: pageStack.push(settingsPageComp) }
        ]

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 6
            spacing: 2

            // Status row
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Label { text: "rclone:"; color: Kirigami.Theme.disabledTextColor; font.pixelSize: 11 }
                Label { text: mainWindow.rcloneVersion || "..."; font.bold: true; font.pixelSize: 11 }
                Item { Layout.fillWidth: true }
                Label { text: "Daemon:"; color: Kirigami.Theme.disabledTextColor; font.pixelSize: 11 }
                Label { text: mainWindow.daemonRunning ? "Running" : "Stopped"; color: mainWindow.daemonRunning ? "green" : "red"; font.bold: true; font.pixelSize: 11 }
            }

            // Sync row
            RowLayout {
                Layout.fillWidth: true
                spacing: 4
                Label { text: "Last:"; color: Kirigami.Theme.disabledTextColor; font.pixelSize: 11 }
                Label { text: bisync.lastSyncTime || "never"; font.bold: true; font.pixelSize: 11 }
                Label { text: "|"; color: Kirigami.Theme.disabledTextColor; font.pixelSize: 11 }
                Label { text: bisync.running ? bisync.currentRemote : (bisync.lastSyncStatus || "idle"); color: bisync.running ? "#2980b9" : (bisync.lastSyncStatus && bisync.lastSyncStatus.indexOf("Success") >= 0 ? "#18b242" : (bisync.lastSyncStatus ? "#da4453" : Kirigami.Theme.textColor)); font.bold: true; font.pixelSize: 11 }
                Label { text: "|"; color: Kirigami.Theme.disabledTextColor; font.pixelSize: 11 }
                Label { text: "Next:"; color: Kirigami.Theme.disabledTextColor; font.pixelSize: 11 }
                Label { text: bisync.nextSyncTime || "--:--"; font.bold: true; font.pixelSize: 11 }
                Item { Layout.fillWidth: true }
                BusyIndicator { running: bisync.running; visible: bisync.running; implicitHeight: 14; implicitWidth: 14 }
                Button { text: "Sync All"; enabled: !bisync.running; icon.name: "view-refresh"; onClicked: bisync.startAllSyncs(); flat: true; visible: !bisync.running }
                Button { text: "Stop"; icon.name: "process-stop"; visible: bisync.running; onClicked: bisync.cancel(); flat: true }
                Button { text: "Clear"; icon.name: "edit-clear"; visible: !bisync.running; onClicked: bisync.clearAllLocks(); flat: true }
            }

            // Remotes - use ListView for proper layout
            ListView {
                id: remotesView
                Layout.fillWidth: true
                Layout.preferredHeight: contentHeight
                model: mainWindow.remotesModel
                clip: true
                spacing: 0

                delegate: Item {
                    width: remotesView.width
                    height: 28
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 4
                        anchors.rightMargin: 4
                        spacing: 6
                        Label { text: model.name || ""; font.bold: true; font.pixelSize: 11 }
                        Label { text: model.type || ""; color: Kirigami.Theme.disabledTextColor; font.pixelSize: 11 }
                        Label { text: mainWindow.isRemoteMounted(model.name) ? "✓ mounted" : ""; color: "#18b242"; font.bold: true; font.pixelSize: 11 }
                        Item { Layout.fillWidth: true }
                        Button {
                            text: mainWindow.isRemoteMounted(model.name) ? "Unmount" : "Mount"
                            icon.name: mainWindow.isRemoteMounted(model.name) ? "media-eject" : "drive-harddisk"
                            flat: true
                            onClicked: {
                                if (mainWindow.isRemoteMounted(model.name))
                                    mainWindow.unmountRemote(model.name)
                                else
                                    mainWindow.mountRemote(model.name)
                            }
                        }
                        Button {
                            text: "Details"
                            icon.name: "document-properties"
                            flat: true
                            onClicked: pageStack.push(detailPageComp, { remoteName: model.name })
                        }
                    }
                }
            }

            // Terminal
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 100
                visible: bisync.currentOutput.length > 0
                color: "#232629"
                radius: 2
                border.width: 1
                border.color: "#444"

                ScrollView {
                    id: syncScroll
                    anchors.fill: parent
                    anchors.margins: 2
                    clip: true

                    TextArea {
                        id: syncOutputArea
                        readOnly: true
                        font.family: "monospace"
                        font.pixelSize: 11
                        wrapMode: TextEdit.WrapAnywhere
                        textFormat: Text.RichText
                        background: null
                        color: "#fcfcfc"
                        text: bisync.currentOutputRich

                        property bool stickToBottom: true

                        onContentHeightChanged: {
                            if (stickToBottom)
                                Qt.callLater(scrollToBottom)
                        }

                        function scrollToBottom() {
                            var f = syncOutputArea.flickable
                            if (f && f.contentHeight > f.height)
                                f.contentY = f.contentHeight - f.height
                        }

                        Timer {
                            id: stickTimer
                            interval: 200
                            repeat: true
                            running: bisync.running && syncOutputArea.stickToBottom
                            onTriggered: syncOutputArea.scrollToBottom()
                        }

                        onStickToBottomChanged: stickTimer.running = bisync.running && syncOutputArea.stickToBottom
                    }
                }
            }
        }
    }
}
