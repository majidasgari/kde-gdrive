import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: root

    title: "OAuth Setup"

    property string remoteName: "gdrive"
    property string clientId: ""
    property string clientSecret: ""

    signal done()
    signal cancelled()

    Kirigami.Action {
        id: cancelAction
        text: "Back"
        icon.name: "go-previous"
        onTriggered: root.cancelled()
    }

    actions: [cancelAction]

    ColumnLayout {
        anchors.centerIn: parent
        spacing: Kirigami.Units.largeSpacing
        Layout.preferredWidth: Math.min(400, parent.width - Kirigami.Units.gridUnit * 4)

        Kirigami.Heading {
            text: "Google Drive Authentication"
            Layout.alignment: Qt.AlignHCenter
            level: 2
        }

        Label {
            text: "This will create a new remote named 'gdrive' using Google Drive.\nYour browser will open to authorize access."
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }

        TextField {
            id: clientIdField
            placeholderText: "Client ID (optional)"
            Layout.fillWidth: true
            onTextChanged: root.clientId = text
        }

        TextField {
            id: clientSecretField
            placeholderText: "Client Secret (optional)"
            Layout.fillWidth: true
            onTextChanged: root.clientSecret = text
            echoMode: TextInput.Password
        }

        Button {
            text: "Start OAuth Setup"
            icon.name: "media-playback-start"
            Layout.alignment: Qt.AlignHCenter
            enabled: !oauthWizard.running

            onClicked: {
                oauthWizard.startOAuth(root.clientId, root.clientSecret)
            }
        }

        BusyIndicator {
            running: oauthWizard.running
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: oauthWizard.currentStep
            visible: oauthWizard.running
            Layout.alignment: Qt.AlignHCenter
            color: Kirigami.Theme.disabledTextColor
        }

        Connections {
            target: oauthWizard
            function onOAuthSuccess(name) {
                root.done()
            }
            function onOAuthFailed(error) {
                applicationWindow().showPassiveNotification("OAuth failed: " + error, "long")
            }
        }
    }
}
