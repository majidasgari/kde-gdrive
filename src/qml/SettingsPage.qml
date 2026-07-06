import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: root
    title: "Settings"

    signal addRemoteClicked()

    actions: [
        Kirigami.Action {
            text: "Add Google Drive"
            icon.name: "list-add"
            onTriggered: root.addRemoteClicked()
        },
        Kirigami.Action {
            text: "Close"
            icon.name: "window-close"
            onTriggered: pageStack.pop()
        }
    ]

    Flickable {
        anchors.fill: parent
        contentHeight: contentCol.implicitHeight + Kirigami.Units.gridUnit
        clip: true

        ColumnLayout {
            id: contentCol
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.largeSpacing

            Kirigami.Heading { text: "Settings"; level: 2 }

            // Bisync section
            Label { text: "Bisync"; font.bold: true; font.pointSize: Kirigami.Theme.smallFont.pointSize + 2 }

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing
                Label { text: "Enable bisync:"; Layout.preferredWidth: 120 }
                CheckBox { id: syncEnabledCb; onToggled: settings.syncEnabled = checked }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing
                Label { text: "Resync mode:"; Layout.preferredWidth: 120 }
                ComboBox {
                    id: resyncModeCb
                    model: ["older", "newer", "none"]
                    Layout.fillWidth: true
                    onCurrentValueChanged: settings.syncResyncMode = currentValue
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing
                Label { text: "Sync times:"; Layout.preferredWidth: 120 }
                TextField {
                    id: syncTimesField
                    Layout.fillWidth: true
                    placeholderText: "10,18,23"
                    onEditingFinished: settings.syncTimes = text
                }
            }

            Label {
                text: "Per-remote sync paths: set in remote detail (double-click)."
                color: Kirigami.Theme.disabledTextColor
                font.pixelSize: 11
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Kirigami.Separator { Layout.fillWidth: true }

            // Daemon section
            Label { text: "Daemon"; font.bold: true; font.pointSize: Kirigami.Theme.smallFont.pointSize + 2 }

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing
                Label { text: "RC Port:"; Layout.preferredWidth: 120 }
                SpinBox {
                    id: portSpin
                    from: 1024
                    to: 65535
                    Layout.fillWidth: true
                    onValueChanged: settings.rclonePort = value
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing
                Label { text: "Auto-start daemon:"; Layout.preferredWidth: 120 }
                CheckBox { id: autoStartCb; onToggled: settings.autoStartDaemon = checked }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing
                Label { text: "Auto-mount:"; Layout.preferredWidth: 120 }
                CheckBox { id: autoMountCb; onToggled: settings.autoMountOnStart = checked }
            }

            Item { Layout.fillHeight: true }
        }
    }

    Component.onCompleted: {
        syncEnabledCb.checked = settings.syncEnabled
        portSpin.value = settings.rclonePort
        autoStartCb.checked = settings.autoStartDaemon
        autoMountCb.checked = settings.autoMountOnStart
        syncTimesField.text = settings.syncTimes
        if (settings.syncResyncMode === "older") resyncModeCb.currentIndex = 0
        else if (settings.syncResyncMode === "newer") resyncModeCb.currentIndex = 1
        else resyncModeCb.currentIndex = 2
    }

    Connections {
        target: settings
        function onSettingsChanged() {
            syncEnabledCb.checked = settings.syncEnabled
            portSpin.value = settings.rclonePort
            autoStartCb.checked = settings.autoStartDaemon
            autoMountCb.checked = settings.autoMountOnStart
            syncTimesField.text = settings.syncTimes
        }
    }
}
