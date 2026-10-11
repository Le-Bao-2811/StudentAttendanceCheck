#ifndef CAMERAPROVIDER_H
#define CAMERAPROVIDER_H

#include <QObject>
#include <QQuickImageProvider>
#include <QImage>
#include <QTimer>
#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QDateTime>
#include <QCoreApplication>
#include <QDebug>

// Structure to store dummy attendance data for a student
struct StudentData {
    int Id=0;
    QString fullName = "Nguyễn Hoài Bảo";
    QString className = "CNTT - K18";
    QString avatarPath = "E:/hoaibao.jpg";
    QString studentId = "Sv001";
    QString checkInTime = "";

    void updateCheckInTime() {
        checkInTime = QDateTime::currentDateTime().toString("hh:mm:ss - dd/MM/yyyy");
    }
    QString getFormattedAvatarUrl() const {
        if (avatarPath.isEmpty()) return "";
        if (avatarPath.startsWith("http://") || avatarPath.startsWith("https://") || avatarPath.startsWith("file:///")) {
            return avatarPath;
        }
        return "file:///" + avatarPath;
    }
    cv::Mat faceCrop;
};


class FaceMatcher {
public:
    FaceMatcher() {
        // Load the default Haar Cascade file from OpenCV
        if (!faceCascade.load("haarcascade_frontalface_alt2.xml")) {
            qDebug() << "Unable to load the XML Cascade file!";
        }
    }

    // Crop the face region from an image
    cv::Mat getFaceCrop(const cv::Mat& image) {
        if (image.empty() || faceCascade.empty()) return cv::Mat();

        cv::Mat gray;
        if (image.channels() == 3) {
            cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
        } else {
            gray = image;
        }
        cv::equalizeHist(gray, gray);

        std::vector<cv::Rect> faces;
        faceCascade.detectMultiScale(gray, faces, 1.1, 3, 0, cv::Size(80, 80));

        if (faces.empty()) return cv::Mat();

        // Get the largest face found
        cv::Mat faceCrop = gray(faces[0]);
        cv::resize(faceCrop, faceCrop, cv::Size(100, 100)); // Standard resize size
        return faceCrop.clone();
    }

    // Compare two face regions using simple cosine similarity
    bool compareFaces(const cv::Mat& face1, const cv::Mat& face2) {
        if (face1.empty() || face2.empty()) return false;

        cv::Mat f1, f2;
        face1.convertTo(f1, CV_32F);
        face2.convertTo(f2, CV_32F);

        double dot = f1.dot(f2);
        double norm1 = cv::norm(f1);
        double norm2 = cv::norm(f2);

        if (norm1 == 0 || norm2 == 0) return false;

        double similarity = dot / (norm1 * norm2);
        // Similarity threshold
        return similarity >= 0.85;
    }

private:
    cv::CascadeClassifier faceCascade;
};
// 1. Camera management class and signal emission class (inherits QObject)
class CameraController : public QObject
{
    Q_OBJECT
public:
    explicit CameraController(QObject *parent = nullptr);
    ~CameraController();

    Q_INVOKABLE bool openDroidCam(int cameraIndex = 0);
    Q_INVOKABLE bool openDroidCamUrl(const QString &url);
    Q_INVOKABLE void loadDummyData(const QString &photoPath);
    Q_INVOKABLE bool loadAllStudentsFromDb();
    Q_INVOKABLE bool connectToDatabase();
    Q_INVOKABLE void startCaptureProcess(const QString &studentId, const QString &studentName, const QString &studentClass);
    QImage getCurrentImage() const { return m_currentFrame; }

signals:
    void frameUpdated();
    void studentRecognized(QString name, QString className, QString timeStr, QString avatar);
    void faceMismatch();

private slots:
    void grabFrame();

private:
    void processDummyAttendance();
    cv::VideoCapture m_capture;
    QImage m_currentFrame;
    QTimer *m_timer;
    bool saveStudentToDb(const QString &studentId, const QString &studentName, const QString &studentClass, const QString &imagePath);

    QVector<StudentData> m_studentCache;
    StudentData m_dummyStudent;
    FaceMatcher m_faceMatcher;
    bool m_isAlreadyRecognized;

    // [NEW] Flag indicating a capture request for the next frame
    bool m_shouldCapture = false;

    // [NEW] Temporary storage for student information entered in QML before capture
    QString m_tempId;
    QString m_tempName;
    QString m_tempClass;
};

// 2. Image provider class for QML (inherits only from QQuickImageProvider)
class CameraImageProvider : public QQuickImageProvider
{
public:
    explicit CameraImageProvider(CameraController *controller);

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    CameraController *m_controller;
};

#endif // CAMERAPROVIDER_H