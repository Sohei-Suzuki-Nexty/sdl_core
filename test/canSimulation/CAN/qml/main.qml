import QtQuick 2.2
import QtQuick.Controls 1.1
import "components" 1.0

ApplicationWindow {

    id: root
    visible: true
    width: 800
    height: Style.windowHeight
    title: qsTr("Hello World")

    signal viewClicked(string name)
    signal createConnection(string ip, int port)

    function incoming(msg) {
        console.log("Got message:", msg)
        return "some return value"
    }

    menuBar: MenuBar {
        Menu {
            title: qsTr("File")
            MenuItem {
                text: qsTr("Exit")
                onTriggered: Qt.quit();
            }
        }
    }

    TabView {

        id: viewsTab
        width: parent.width
        height: root.height - 47 - tcpLogsView.height

        Tab {
            title: "Media"
            Media {
                id: mediaComponent

                onRequestButtonClick: {
                    tcpLogsView.textColor = "green"
                    tcpLogsView.append("Request " + item.objectName + " send")
                    switch (item.objectName) {
                        case "OnRadioDetails": {

                            console.log("HAHAHAHAHA")
                            root.viewClicked(item.objectName)

                            break;
                        }
                    }


                }
            }
        }
        Tab {
            title: "Climate"
            Rectangle { color: "blue" }
        }
        Tab {
            title: "Navigation"
            Rectangle { color: "green" }
        }
        Tab {
            title: "Phone"
            Rectangle { color: "yellow" }
        }
        Tab {
            title: "MCS"
            Rectangle { color: "white" }
        }
        Tab {
            title: "Settings"
            Settings {
                id: settingsComponent

                onConnect: {

                    console.log(settingsComponent.ip, settingsComponent.port)
                    createConnection(settingsComponent.ip, settingsComponent.port);
                }
            }
        }
    }

    Rectangle {
        id: appearRect

        width: root.width
        anchors.left: root.left
        anchors.right: root.right
        height: 20
        anchors.top: viewsTab.bottom
        color: "gray"

        Text {
            text: "Logger"
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (tcpLogsView.height === 0) {
                    tcpLogsView.height = 100
                } else {

                    tcpLogsView.height = 0
                }

            }
        }
    }

    TextArea {

        id: tcpLogsView
        width: parent.width
        anchors.top: appearRect.bottom
        height: 00
        visible: true
        text: "Logger";
    }
}
