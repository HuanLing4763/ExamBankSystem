import QtQuick
import QtQuick.Controls

Item {
    id: root
    width: 800
    height: 600

    property alias text: label.text
    property real size: 200

    // 纯白背景
    Rectangle {
        anchors.fill: parent
        color: "white"
    }

    Column {
        anchors.centerIn: parent
        spacing: 40

        // 简约圆环加载器
        Item {
            id: spinnerContainer
            width: root.size
            height: root.size
            anchors.horizontalCenter: parent.horizontalCenter

            // 主圆环
            Image {
                id: spinner
                anchors.centerIn: parent
                source: "qrc:/ExamBankSystem/resources/loading.svg"
                sourceSize: Qt.size(root.size, root.size)
            }

            // 旋转动画
            RotationAnimator {
                target: spinnerContainer
                from: 0
                to: 360
                duration: 1500
                loops: Animator.Infinite
                running: true
                easing.type: Easing.Linear
            }
        }

        // 文字样式
        Label {
            id: label
            text: "正在进入考试环境..."
            color: "#66ccff"
            font {
                pixelSize: 26
                family: "Microsoft YaHei"
                letterSpacing: 1
            }
            anchors.horizontalCenter: parent.horizontalCenter
        }

        // 简约进度指示
        Rectangle {
            width: 120
            height: 2
            color: "#e6f3ff"
            radius: 1
            anchors.horizontalCenter: parent.horizontalCenter

            Rectangle {
                width: parent.width
                height: parent.height
                color: "#66ccff"
                radius: 1
                clip: true
                
                NumberAnimation on x {
                    duration: 1800
                    loops: Animation.Infinite
                    easing.type: Easing.InOutSine
                }
            }
        }
    }

    // 装饰性圆点
    Row {
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: 60
        }
        spacing: 10

        Repeater {
            model: 3
            Rectangle {
                width: 12
                height: 12
                radius: 6
                color: "#66ccff"
                opacity: index === 0 ? 0.3 : 0.1

                NumberAnimation on opacity {
                    loops: Animation.Infinite
                    from: index === 0 ? 0.3 : 0.1
                    to: index === 0 ? 0.1 : 0.3
                    duration: 1200
                    easing.type: Easing.InOutQuad
                }
            }
        }
    }
}