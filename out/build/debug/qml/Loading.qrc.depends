import QtQuick
import QtQuick.Controls

Item {
    id: root

    property alias text: label.text
    property real size: 200

    Column {
        anchors.centerIn: parent
        spacing: 20

        Image {
            id: spinner
            source: "qrc:/ExamBankSystem/resources/loading.svg"
            sourceSize: Qt.size(root.size, root.size)
            anchors.horizontalCenter: parent.horizontalCenter

            RotationAnimator {
                target: spinner
                from: 0
                to: 360
                duration: 1000
                loops: Animator.Infinite
                running: true
            }
        }

        Label {
            id: label
            text: "正在进入考试环境..."
            color: "white"
            font.pixelSize: 24
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}