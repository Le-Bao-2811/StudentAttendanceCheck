import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
Window {
    id: mainWindow
    width: 1024
    height: 600
    visible: true
    title: "Hệ thống điểm danh bằng khuôn mặt"
    color: "#1e1e2e" // Màu nền Dark theme

    // Dữ liệu giả lập từ Backend C++ trả về khi quét thành công
    property bool isRecognized: false
    property string studentName: "Nguyễn Văn A"
    property string studentId: "SV2026001"
    property string studentClass: "CNTT K18"
    property string timeStamp: ""
    property string avatarSource: "image://camera/stream" // Đường dẫn ImageProvider từ C++
    property int frameCounter: 0

        // Khi C++ phát signal frameUpdated -> Cập nhật lại Image Source để đổi Frame mới
    Connections {
        target: cameraController
        function onFrameUpdated() {
            frameCounter++;
            cameraView.source = "image://camera/frame_" + frameCounter;
        }
    }

    Component.onCompleted: {
        console.log("da calll ")
        cameraController.openDroidCamUrl("");
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // ==========================================
        // KHU VỰC 1: HIỂN THỊ CAMERA QUÉT KHUÔN MẶT
        // ==========================================
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#11111b"
            radius: 12
            border.color: "#313244"
            border.width: 2

            // Image provider nhận frame ảnh từ OpenCV truyền qua C++
            Image {
                id: cameraView
                anchors.fill: parent
                anchors.margins: 5
                fillMode: Image.PreserveAspectFit
                cache: false;
                // Bạn có thể liên kết nguồn ảnh thực tế từ C++ ImageProvider tại đây
                source: "image://camera/frame_0"
            }

            // Khung quét nhấp nháy tạo hiệu ứng Scanning
            Rectangle {
                width: parent.width * 0.5
                height: parent.width * 0.5
                anchors.centerIn: parent
                color: "transparent"
                border.color: isRecognized ? "#a6e3a1" : "#89b4fa" // Xanh lá khi nhận diện được, Xanh dương khi đang chờ
                border.width: 3
                radius: 10

                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    running: !isRecognized
                    NumberAnimation { from: 1.0; to: 0.3; duration: 800 }
                    NumberAnimation { from: 0.3; to: 1.0; duration: 800 }
                }
            }

            Text {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottomMargin: 15
                text: "Đang quét khuôn mặt..."
                color: "#cdd6f4"
                font.pixelSize: 14
            }
        }

        // ==========================================
        // KHU VỰC 2: BẢNG THÔNG TIN HỌC SINH
        // ==========================================
        Rectangle {
            Layout.preferredWidth: 360
            Layout.fillHeight: true
            color: "#181825"
            radius: 12
            border.color: "#313244"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 15

                Text {
                    text: "THÔNG TIN ĐIỂM DANH"
                    color: "#cdd6f4"
                    font.pixelSize: 18
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#313244"
                }

                // Avatar / Ảnh quét
                Rectangle {
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 120
                    Layout.alignment: Qt.AlignHCenter
                    radius: 60
                    color: "#313244"
                    clip: true

                    Text {
                        anchors.centerIn: parent
                        text: isRecognized ? "" : "No Image"
                        color: "#a6adc8"
                    }

                    Image {
                        anchors.fill: parent
                        visible: isRecognized
                        source: "https://via.placeholder.com/120" // URL ảnh đại diện sinh viên
                    }
                }

                // Chi tiết thông tin
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    visible: isRecognized

                    Rectangle {
                        Layout.fillWidth: true
                        height: 35
                        color: "#27a060"
                        radius: 6
                        Text {
                            anchors.centerIn: parent
                            text: "✓ THÀNH CÔNG"
                            color: "white"
                            font.bold: true
                        }
                    }

                    Text { text: "Họ và tên: " + studentName; color: "#cdd6f4"; font.pixelSize: 15; font.bold: true }
                    Text { text: "Mã sinh viên: " + studentId; color: "#a6adc8"; font.pixelSize: 14 }
                    Text { text: "Lớp: " + studentClass; color: "#a6adc8"; font.pixelSize: 14 }
                    Text { text: "Thời gian: " + timeStamp; color: "#a6adc8"; font.pixelSize: 14 }
                }

                Text {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter
                    text: "Vui lòng nhìn vào camera..."
                    color: "#a6adc8"
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    visible: !isRecognized
                }

                //Spacer {} // Đẩy nút bấm xuống dưới

                // Nút bấm giả lập quét thành công (Dùng để Test UI)
                Button {
                    Layout.fillWidth: true
                    text: isRecognized ? "Reset / Quét người tiếp theo" : "Test Quét Thành Công"
                    onClicked: {
                        if (isRecognized) {
                            isRecognized = false;
                        } else {
                            // Giả lập nhận signal từ C++ trả dữ liệu về
                            isRecognized = true;
                            studentName = "Nguyễn Văn A";
                            studentId = "SV2026001";
                            studentClass = "CNTT K18";
                            timeStamp = Qt.formatDateTime(new Date(), "hh:mm:ss - dd/MM/yyyy");
                            resetTimer.restart(); // Tự động ẩn thông tin sau 4 giây
                        }
                    }
                }
            }
        }
    }

    // Timer tự động xóa thông tin sau khi nhận diện thành công 4 giây
    Timer {
        id: resetTimer
        interval: 4000
        repeat: false
        onTriggered: {
            isRecognized = false;
        }
    }
}