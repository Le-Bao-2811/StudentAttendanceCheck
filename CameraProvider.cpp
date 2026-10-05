#include "cameraprovider.h"
#include <QDebug>

// ==========================================
// CAMERA CONTROLLER
// ==========================================
CameraController::CameraController(QObject *parent) : QObject(parent), m_isAlreadyRecognized(false)
{
    m_timer = new QTimer(this);
    // Connect directly to the base QObject signal here; no more conflict issue
    connect(m_timer, &QTimer::timeout, this, &CameraController::grabFrame);
}

CameraController::~CameraController()
{
    if (m_capture.isOpened()) {
        m_capture.release();
    }
}

// In CameraController::loadDummyData:
void CameraController::loadDummyData(const QString &photoPath) {
    cv::Mat myPhoto = cv::imread(photoPath.toStdString());
    if (!myPhoto.empty()) {
        m_myFaceCrop = m_faceMatcher.getFaceCrop(myPhoto);
        m_dummyStudent.avatarPath = photoPath;
        qDebug() << "Face extracted successfully from the sample image!";
    } else {
        qDebug() << "Unable to open the sample image file!";
    }
}

bool CameraController::openDroidCam(int cameraIndex)
{
    if (m_capture.isOpened()) {
        m_capture.release();
    }

    // Force using Media Foundation (CAP_MSMF) only; do not try CAP_ANY to avoid DirectShow crashes
    m_capture.open(cameraIndex, cv::CAP_MSMF);

    if (!m_capture.isOpened()) {
        qDebug() << "Unable to connect to DroidCam via MSMF at index:" << cameraIndex;
        return false;
    }

    m_timer->start(33); // ~30 FPS
    return true;
}

bool CameraController::openDroidCamUrl(const QString &url)
{
    if (m_capture.isOpened()) {
        m_capture.release();
    }

    std::string stdUrl = url.toStdString();
    qDebug() << "Opening URL:" << url;

    // Use FFMPEG to open the video stream
    m_capture.open(stdUrl, cv::CAP_FFMPEG);

    if (!m_capture.isOpened()) {
        // Try fallback to CAP_ANY
        m_capture.open(stdUrl, cv::CAP_ANY);
    }

    if (!m_capture.isOpened()) {
        qDebug() << "ERROR: m_capture.isOpened() is still FALSE for URL:" << url;
        return false;
    }

    qDebug() << "SUCCESS: DroidCam stream connected!";
    m_timer->start(33); // ~30 FPS
    return true;
}

void CameraController::grabFrame()
{
    cv::Mat frame;
    if (m_capture.read(frame) && !frame.empty() && frame.cols > 0 && frame.rows > 0) {

        // Only compare faces if the sample face has been loaded and the student is not already checked in
        if (!m_myFaceCrop.empty() && !m_isAlreadyRecognized) {

            // Detect and crop the face currently being scanned by the camera
            cv::Mat currentCamFace = m_faceMatcher.getFaceCrop(frame);

            if (!currentCamFace.empty()) {
                // Compare the face in the camera with the sample face
                bool isMatched = m_faceMatcher.compareFaces(m_myFaceCrop, currentCamFace);

                if (isMatched) {
                    // [MATCHED FACE] Lock recognition and emit a signal to send attendance data to QML
                    processDummyAttendance();
                } else {
                    // [MISMATCHED FACE] Emit mismatch only if not in the reset wait state
                    emit faceMismatch();
                }
            }
        }

        // Convert the frame for display in QML
        cv::Mat rgbFrame;
        cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);

        m_currentFrame = QImage(rgbFrame.data, rgbFrame.cols, rgbFrame.rows,
                                static_cast<int>(rgbFrame.step),
                                QImage::Format_RGB888).copy();

        emit frameUpdated();
    }
}
void CameraController::processDummyAttendance() {
    // [NEW] Enable the lock flag to stop duplicate comparisons
    m_isAlreadyRecognized = true;
    // [NEW] Update the current time (Current DateTime)
    m_dummyStudent.updateCheckInTime();

    // [NEW] Emit a signal sending the complete dummy attendance data to QML
    emit studentRecognized(
        m_dummyStudent.fullName,
        m_dummyStudent.className,
        m_dummyStudent.checkInTime,
        m_dummyStudent.avatarPath
        );

    // [NEW] Wait 5 seconds, then automatically re-enable scanning for the next attempt
    QTimer::singleShot(5000, this, [this]() {
        m_isAlreadyRecognized = false;
    });
}

// ==========================================
// CAMERA IMAGE PROVIDER
// ==========================================
CameraImageProvider::CameraImageProvider(CameraController *controller)
    : QQuickImageProvider(QQuickImageProvider::Image), m_controller(controller)
{
}

QImage CameraImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(id);
    Q_UNUSED(requestedSize);

    if (!m_controller) return QImage();

    QImage img = m_controller->getCurrentImage();
    if (size) *size = img.size();

    if (img.isNull()) {
        QImage emptyImg(640, 480, QImage::Format_RGB888);
        emptyImg.fill(Qt::black);
        return emptyImg;
    }
    return img;
}