import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Window {
    id: mainWindow
    width: 1024
    height: 600
    visible: true
    title: "Face Attendance System"
    color: "#1e1e2e" // Dark theme background color

    // Simulated data returned from the C++ backend after a successful scan
    property bool isRecognized: false
    property string studentName: "Nguyễn Văn A"
    property string studentId: "SV2026001"
    property string studentClass: "CNTT K18"
    property string timeStamp: ""

    // Default empty image path
    property string avatarSource: ""
    property int frameCounter: 0

    // When C++ emits the frameUpdated signal, refresh the Image source to show the new frame
    Connections {
        target: cameraController
        function onFrameUpdated() {
            frameCounter++;
            cameraView.source = "image://camera/frame_" + frameCounter;
        }
    }

    Component.onCompleted: {
        cameraController.loadDummyData("E:/hoaibao.jpg"); // Recommended to use forward slashes / instead of \
        cameraController.openDroidCamUrl("");
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // ==========================================
        // AREA 1: CAMERA DISPLAY FOR FACE SCANNING
        // ==========================================
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#11111b"
            radius: 12
            border.color: "#313244"
            border.width: 2

            Image {
                id: cameraView
                anchors.fill: parent
                anchors.margins: 5
                fillMode: Image.PreserveAspectFit
                cache: false
                source: "image://camera/frame_0"
            }

            // Scanning frame with a blinking effect
            Rectangle {
                width: parent.width * 0.5
                height: parent.width * 0.5
                anchors.centerIn: parent
                color: "transparent"
                border.color: isRecognized ? "#a6e3a1" : (infoPanel.isMismatch ? "#f38ba8" : "#89b4fa") // Updated frame color when an error occurs
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
                text: isRecognized ? "Attendance recorded" : "Scanning face..."
                color: "#cdd6f4"
                font.pixelSize: 14
            }
        }

        // ==========================================
        // AREA 2: STUDENT INFORMATION PANEL
        // ==========================================
        Rectangle {
            id: infoPanel
            Layout.preferredWidth: 360
            Layout.fillHeight: true
            color: "#181825"
            radius: 12
            border.color: "#313244"
            border.width: 1

            property bool isMismatch: false

            // Connect and listen to signals from the C++ class (CameraController)
            Connections {
                target: cameraController

                // Handler when C++ recognizes the correct face
                function onStudentRecognized(name, className, timeStr, avatar) {
                    studentName = name;
                    studentClass = className;
                    timeStamp = timeStr;
                    avatarSource = avatar;

                    infoPanel.isMismatch = false; // Clear the error state
                    isRecognized = true;         // Enable success state

                    resetTimer.restart(); // Automatically reset the UI after 4 seconds
                }

                // Handler when C++ scans the wrong face or cannot find a face
                function onFaceMismatch() {
                    // Report mismatch only if attendance has not succeeded yet
                    if (!isRecognized) {
                        infoPanel.isMismatch = true;
                        resetTimer.restart();
                    }
                }
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 15

                Text {
                    text: "ATTENDANCE INFORMATION"
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

                // Avatar / scanned image
                Rectangle {
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 120
                    Layout.alignment: Qt.AlignHCenter
                    radius: 60
                    color: "#313244"
                    clip: true

                    Text {
                        anchors.centerIn: parent
                        text: isRecognized ? "" : (infoPanel.isMismatch ? "Mismatch" : "No Image")
                        color: "#a6adc8"
                    }

                    Image {
                        anchors.fill: parent
                        visible: isRecognized
                        // Use the avatarSource variable from C++ instead of a hardcoded URL
                        source: avatarSource !== "" ? avatarSource : "https://via.placeholder.com/120"
                        fillMode: Image.PreserveAspectCrop
                    }
                }

                // Student details
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    visible: isRecognized || infoPanel.isMismatch

                    // Status label frame (Success / Mismatch)
                    Rectangle {
                        Layout.fillWidth: true
                        height: 35
                        // Adjust background color dynamically: green for success, red for mismatch
                        color: infoPanel.isMismatch ? "#f38ba8" : "#27a060"
                        radius: 6

                        Text {
                            anchors.centerIn: parent
                            text: infoPanel.isMismatch ? "✕ MISMATCH" : "✓ SUCCESS"
                            color: infoPanel.isMismatch ? "#11111b" : "white"
                            font.bold: true
                        }
                    }

                    // Detailed student information when recognition is successful
                    Text { visible: isRecognized; text: "Full name: " + studentName; color: "#cdd6f4"; font.pixelSize: 15; font.bold: true }
                    Text { visible: isRecognized; text: "Student ID: " + studentId; color: "#a6adc8"; font.pixelSize: 14 }
                    Text { visible: isRecognized; text: "Class: " + studentClass; color: "#a6adc8"; font.pixelSize: 14 }
                    Text { visible: isRecognized; text: "Time: " + timeStamp; color: "#a6adc8"; font.pixelSize: 14 }

                    // Warning message when mismatch occurs
                    Text {
                        visible: infoPanel.isMismatch && !isRecognized
                        text: "Face not found in the system!"
                        color: "#f38ba8"
                        font.pixelSize: 14
                        Layout.alignment: Qt.AlignHCenter
                    }
                }

                Text {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter
                    text: "Please look at the camera..."
                    color: "#a6adc8"
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    visible: !isRecognized && !infoPanel.isMismatch
                }
            }
        }
    }

    // Timer that clears the information and returns the UI to the initial scanning state
    Timer {
        id: resetTimer
        interval: 4000
        repeat: false
        onTriggered: {
            isRecognized = false;
            infoPanel.isMismatch = false; // Reset mismatch flag as well
        }
    }
}