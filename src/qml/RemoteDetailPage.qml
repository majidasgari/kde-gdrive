import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: root
    property string remoteName: ""
    title: remoteName

    actions: [
        Kirigami.Action {
            text: "Mount"
            icon.name: "drive-harddisk"
            onTriggered: mainWindow.mountRemote(remoteName)
        },
        Kirigami.Action {
            text: "Sync"
            icon.name: "view-refresh"
            onTriggered: {
                var p = settings.syncPathForRemote(remoteName)
                if (p) bisync.startSync(remoteName + ":", p)
            }
        },
        Kirigami.Action {
            text: "Deduplicate"
            icon.name: "document-cleanup"
            enabled: !bisync.running
            onTriggered: {
                bisync.runDedupe(remoteName)
                pageStack.pop() // Pop back to MainWindow to see terminal output
            }
        },
        Kirigami.Action {
            text: "Close"
            icon.name: "window-close"
            onTriggered: pageStack.pop()
        }
    ]

    Flickable {
        anchors.fill: parent
        contentHeight: detailCol.implicitHeight + Kirigami.Units.gridUnit
        clip: true

        ColumnLayout {
            id: detailCol
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.largeSpacing

            Kirigami.Heading { text: remoteName; level: 2 }

            // Info
            Label { text: "Info"; font.bold: true; font.pointSize: Kirigami.Theme.smallFont.pointSize + 2 }

            RowLayout {
                spacing: Kirigami.Units.largeSpacing
                Label { text: "Type:"; color: Kirigami.Theme.disabledTextColor; Layout.preferredWidth: 80 }
                Label {
                    font.bold: true
                    text: {
                        var idx = mainWindow.remotesModel.rowForName(remoteName)
                        return idx >= 0 ? mainWindow.remotesModel.remoteType(idx) : "?"
                    }
                }
            }

            RowLayout {
                spacing: Kirigami.Units.largeSpacing
                Label { text: "Mounted:"; color: Kirigami.Theme.disabledTextColor; Layout.preferredWidth: 80 }
                Label {
                    font.bold: true
                    text: mainWindow.isRemoteMounted(remoteName) ? "Yes" : "No"
                    color: mainWindow.isRemoteMounted(remoteName) ? "green" : Kirigami.Theme.textColor
                }
            }

            Kirigami.Separator { Layout.fillWidth: true }

            // Config
            Label { text: "Config"; font.bold: true; font.pointSize: Kirigami.Theme.smallFont.pointSize + 2 }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 150
                color: Kirigami.Theme.alternateBackgroundColor
                radius: Kirigami.Units.smallRadius

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.smallSpacing

                    TextArea {
                        readOnly: true
                        font.family: "monospace"
                        font.pixelSize: 11
                        wrapMode: TextEdit.WrapAnywhere
                        background: null
                        text: {
                            var idx = mainWindow.remotesModel.rowForName(remoteName)
                            return idx >= 0 ? mainWindow.remotesModel.remoteConfigJson(idx) : ""
                        }
                    }
                }
            }

            Kirigami.Separator { Layout.fillWidth: true }

            // Sync Path
            Label { text: "Sync Path"; font.bold: true; font.pointSize: Kirigami.Theme.smallFont.pointSize + 2 }

            Label {
                text: "Local folder for bisync:"
                color: Kirigami.Theme.disabledTextColor
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                TextField {
                    id: syncPathField
                    text: settings.syncPathForRemote(remoteName)
                    placeholderText: "/home/user/" + remoteName
                    Layout.fillWidth: true
                }

                Button {
                    text: "Save"
                    icon.name: "document-save"
                    onClicked: {
                        settings.setSyncPathForRemote(remoteName, syncPathField.text)
                        applicationWindow().showPassiveNotification("Path saved for " + remoteName, "short")
                    }
                }
            }

            Label {
                text: "Default: ~/Cloud/" + remoteName
                color: Kirigami.Theme.disabledTextColor
                font.pixelSize: 11
            }

            Item { Layout.fillHeight: true }
        }
    }
}
