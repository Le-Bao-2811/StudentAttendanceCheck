#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlApplicationEngine>
#include "CameraProvider.h"
int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    // 1. Khai báo biến url
    const QUrl url(QStringLiteral("qrc:/qt/qml/StudentAttendanceCheck/Main.qml"));

    // 2. Khởi tạo Controller & ImageProvider
    CameraController *cameraController = new CameraController(&app);
    engine.rootContext()->setContextProperty("cameraController", cameraController);
    engine.addImageProvider(QLatin1String("camera"), new CameraImageProvider(cameraController));

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.load(url);

    return QGuiApplication::exec();
}
