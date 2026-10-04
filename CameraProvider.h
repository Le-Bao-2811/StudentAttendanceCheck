#ifndef CAMERAPROVIDER_H
#define CAMERAPROVIDER_H

#include <QObject>
#include <QQuickImageProvider>
#include <QImage>
#include <QTimer>
#include <opencv2/opencv.hpp>

// 1. Lớp quản lý Camera & phát Signal (Kế thừa QObject)
class CameraController : public QObject
{
    Q_OBJECT
public:
    explicit CameraController(QObject *parent = nullptr);
    ~CameraController();

    Q_INVOKABLE bool openDroidCam(int cameraIndex = 0);
    Q_INVOKABLE bool openDroidCamUrl(const QString &url);

    QImage getCurrentImage() const { return m_currentFrame; }

signals:
    void frameUpdated();

private slots:
    void grabFrame();

private:
    cv::VideoCapture m_capture;
    QImage m_currentFrame;
    QTimer *m_timer;
};

// 2. Lớp cung cấp ảnh cho QML (Chỉ kế thừa QQuickImageProvider)
class CameraImageProvider : public QQuickImageProvider
{
public:
    explicit CameraImageProvider(CameraController *controller);

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    CameraController *m_controller;
};

#endif // CAMERAPROVIDER_H