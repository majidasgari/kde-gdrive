import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: root
    title: "About"

    actions: [
        Kirigami.Action {
            text: "Close"
            icon.name: "window-close"
            onTriggered: pageStack.pop()
        }
    ]

    Flickable {
        anchors.fill: parent
        contentHeight: contentCol.implicitHeight + Kirigami.Units.gridUnit * 2
        clip: true

        ColumnLayout {
            id: contentCol
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.largeSpacing

            // Main App Card
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: mainCol.implicitHeight + Kirigami.Units.largeSpacing * 2
                color: Qt.rgba(1, 1, 1, 0.04)
                border.color: Qt.rgba(1, 1, 1, 0.08)
                radius: 8

                ColumnLayout {
                    id: mainCol
                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.largeSpacing
                    spacing: Kirigami.Units.mediumSpacing

                    RowLayout {
                        spacing: Kirigami.Units.largeSpacing
                        Image {
                            source: "qrc:/logo.png"
                            Layout.preferredWidth: 48
                            Layout.preferredHeight: 48
                            fillMode: Image.PreserveAspectFit
                        }
                        ColumnLayout {
                            spacing: 2
                            Label {
                                text: "Nimbus Google Drive Client"
                                font.bold: true
                                font.pointSize: Kirigami.Theme.defaultFont.pointSize + 3
                            }
                            Label {
                                text: "v0.1.0"
                                color: Kirigami.Theme.disabledTextColor
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                            }
                        }
                    }

                    Label {
                        text: "Cross-platform desktop Nimbus Google Drive Client with changes polling, recursive directory watching, automatic deduplication, and high-performance two-way synchronization."
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        font.pointSize: Kirigami.Theme.defaultFont.pointSize
                    }
                }
            }

            // Credits Card
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: creditsCol.implicitHeight + Kirigami.Units.largeSpacing * 2
                color: Qt.rgba(1, 1, 1, 0.04)
                border.color: Qt.rgba(1, 1, 1, 0.08)
                radius: 8

                ColumnLayout {
                    id: creditsCol
                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.largeSpacing
                    spacing: Kirigami.Units.mediumSpacing

                    Label {
                        text: "Credits"
                        font.bold: true
                        font.pointSize: Kirigami.Theme.defaultFont.pointSize + 2
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.largeSpacing

                        ColumnLayout {
                            Layout.fillWidth: true
                            Label {
                                text: "Developer"
                                color: Kirigami.Theme.disabledTextColor
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                            }
                            Label {
                                text: "Max. Blackwell"
                                font.bold: true
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Label {
                                text: "AI-assisted by"
                                color: Kirigami.Theme.disabledTextColor
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                            }
                            Label {
                                text: "Gemini 3.5 Flash · GLM 5.2\nDeepSeek V4 Pro · Mimo 2.5"
                                font.bold: true
                            }
                        }
                    }
                }
            }

            // Links Card
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: linksCol.implicitHeight + Kirigami.Units.largeSpacing * 2
                color: Qt.rgba(1, 1, 1, 0.04)
                border.color: Qt.rgba(1, 1, 1, 0.08)
                radius: 8

                ColumnLayout {
                    id: linksCol
                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.largeSpacing
                    spacing: Kirigami.Units.mediumSpacing

                    Label {
                        text: "Links"
                        font.bold: true
                        font.pointSize: Kirigami.Theme.defaultFont.pointSize + 2
                    }

                    ColumnLayout {
                        Label {
                            text: "Source"
                            color: Kirigami.Theme.disabledTextColor
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                        }
                        Label {
                            text: "<a href='https://github.com/majidasgari/kde-gdrive'>github.com/majidasgari/kde-gdrive</a>"
                            textFormat: Text.RichText
                            font.bold: true
                            linkColor: Kirigami.Theme.linkColor
                            onLinkActivated: (link) => Qt.openUrlExternally(link)
                        }
                    }
                }
            }

            // Tech Stack Card
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: techCol.implicitHeight + Kirigami.Units.largeSpacing * 2
                color: Qt.rgba(1, 1, 1, 0.04)
                border.color: Qt.rgba(1, 1, 1, 0.08)
                radius: 8

                ColumnLayout {
                    id: techCol
                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.largeSpacing
                    spacing: Kirigami.Units.mediumSpacing

                    Label {
                        text: "Tech Stack"
                        font.bold: true
                        font.pointSize: Kirigami.Theme.defaultFont.pointSize + 2
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.smallSpacing

                        Repeater {
                            model: ["C++", "Qt 6", "QML", "KF6", "Kirigami", "rclone"]
                            delegate: Rectangle {
                                color: Qt.rgba(1, 1, 1, 0.08)
                                border.color: Qt.rgba(1, 1, 1, 0.12)
                                radius: 4
                                implicitWidth: textLabel.implicitWidth + 16
                                implicitHeight: textLabel.implicitHeight + 8

                                Label {
                                    id: textLabel
                                    anchors.centerIn: parent
                                    text: modelData
                                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                                    font.bold: true
                                }
                            }
                        }
                    }
                }
            }

            // Donate Card
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: donateCol.implicitHeight + Kirigami.Units.largeSpacing * 2
                color: Qt.rgba(1, 1, 1, 0.04)
                border.color: Qt.rgba(1, 1, 1, 0.08)
                radius: 8

                ColumnLayout {
                    id: donateCol
                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.largeSpacing
                    spacing: Kirigami.Units.mediumSpacing

                    Label {
                        text: "Donate"
                        font.bold: true
                        font.pointSize: Kirigami.Theme.defaultFont.pointSize + 2
                    }

                    Label {
                        text: "Support development with TRC-20 (TRON):"
                        color: Kirigami.Theme.disabledTextColor
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: addressText.implicitHeight + 20
                        color: Qt.rgba(0, 0, 0, 0.2)
                        border.color: Qt.rgba(1, 1, 1, 0.1)
                        radius: 4

                        Label {
                            id: addressText
                            anchors.centerIn: parent
                            text: "TLELddwY6sCCACAzu1Wn2kihSBX8PHW32n"
                            font.bold: true
                            font.family: "monospace"
                            font.pointSize: Kirigami.Theme.defaultFont.pointSize
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                mainWindow.copyToClipboard(addressText.text)
                                feedbackLabel.text = "Copied!"
                                resetTimer.start()
                            }
                        }
                    }

                    Label {
                        id: feedbackLabel
                        text: "Click to copy"
                        color: Kirigami.Theme.disabledTextColor
                        font.pointSize: Kirigami.Theme.smallFont.pointSize - 1
                    }

                    Timer {
                        id: resetTimer
                        interval: 2000
                        onTriggered: feedbackLabel.text = "Click to copy"
                    }
                }
            }
        }
    }
}
