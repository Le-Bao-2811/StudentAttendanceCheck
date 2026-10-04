#include "cameraprovider.h"
#include <QDebug>

// ==========================================
// CAMERA CONTROLLER
// ==========================================
CameraController::CameraController(QObject *parent) : QObject(parent)
{
    m_timer = new QTimer(this);
    // Lúc này connect() gọi trực tiếp từ QObject chuẩn, không còn bị xung đột
    connect(m_timer, &QTimer::timeout, this, &CameraController::grabFrame);
}

CameraController::~CameraController()
{
    if (m_capture.isOpened()) {
        m_capture.release();
    }
}

bool CameraController::openDroidCam(int cameraIndex)
{
    if (m_capture.isOpened()) {
        m_capture.release();
    }

    // Ép buộc chỉ dùng Media Foundation (CAP_MSMF), KHÔNG thử CAP_ANY để tránh DirectShow bị crash
    m_capture.open(cameraIndex, cv::CAP_MSMF);

    if (!m_capture.isOpened()) {
        qDebug() << "Không thể kết nối DroidCam qua MSMF tại index:" << cameraIndex;
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
    qDebug() << "Dang mo URL:" << url;

    // Sử dụng FFMPEG để mở luồng video
    m_capture.open(stdUrl, cv::CAP_FFMPEG);

    if (!m_capture.isOpened()) {
        // Thử fallback sang CAP_ANY
        m_capture.open(stdUrl, cv::CAP_ANY);
    }

    if (!m_capture.isOpened()) {
        qDebug() << "LOI: m_capture.isOpened() van FALSE voi URL:" << url;
        return false;
    }

    qDebug() << "THANH CONG: Da ket noi DroidCam stream!";
    m_timer->start(33); // ~30 FPS
    return true;
}

void CameraController::grabFrame()
{
    // Đảm bảo capture đã mở thành công
    if (!m_capture.isOpened()) return;

    cv::Mat frame;
    // Đọc frame và kiểm tra dữ liệu
    if (m_capture.read(frame) && !frame.empty() && frame.cols > 0 && frame.rows > 0) {

        cv::Mat rgbFrame;
        cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);

        m_currentFrame = QImage(rgbFrame.data, rgbFrame.cols, rgbFrame.rows,
                                static_cast<int>(rgbFrame.step),
                                QImage::Format_RGB888).copy();

        emit frameUpdated();
    } else {
        qDebug() << "Frame chưa sẵn sàng hoặc rỗng...";
    }
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