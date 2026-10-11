#include "cameraprovider.h"
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>

// ==========================================
// CAMERA CONTROLLER
// ==========================================
CameraController::CameraController(QObject *parent) : QObject(parent), m_isAlreadyRecognized(false)
{
    m_timer = new QTimer(this);
    if (connectToDatabase()) {
        // Start loading students into the cache
        loadAllStudentsFromDb();
    } else {
        qDebug() << "Unable to load student data because the database connection failed!";
    }
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
        //m_myFaceCrop = m_faceMatcher.getFaceCrop(myPhoto);
        //m_dummyStudent.avatarPath = photoPath;
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

void CameraController::startCaptureProcess(const QString &studentId, const QString &studentName, const QString &studentClass)
{
    m_tempId = studentId;
    m_tempName = studentName;
    m_tempClass = studentClass;
    m_shouldCapture = true; // Mark for grabFrame() to capture on the next loop iteration
}

bool CameraController::saveStudentToDb(const QString &studentId, const QString &studentName, const QString &studentClass, const QString &imagePath)
{
    QSqlQuery query;
    // Prepare the SQL statement to insert the information
    query.prepare("INSERT INTO StudyList (StudentId, NameStudent, Class, pathAvtImage) "
                  "VALUES (:studentId, :name, :class, :path)");

    query.bindValue(":studentId", studentId);
    query.bindValue(":name", studentName);
    query.bindValue(":class", studentClass);
    query.bindValue(":path", imagePath);

    // Execute the query and check for errors
    if (!query.exec()) {
        qDebug() << "Lỗi khi lưu sinh viên vào SQL:" << query.lastError().text();
        return false;
    }

    qDebug() << "Thêm thông tin sinh viên vào Database SQL thành công!";
    return true;
}

void CameraController::grabFrame()
{
    cv::Mat frame;
    if (m_capture.read(frame) && !frame.empty() && frame.cols > 0 && frame.rows > 0) {

        // Only compare faces if the sample face has been loaded and the student is not already checked in
        if (!m_studentCache.isEmpty() && !m_isAlreadyRecognized) {

            // Detect and crop the face currently being scanned by the camera
            cv::Mat currentCamFace = m_faceMatcher.getFaceCrop(frame);

            if (!currentCamFace.empty()) {
                // Compare the face in the camera with the sample face
                bool foundMatch = false;
                StudentData matchedStudent;
                for (const StudentData &student : m_studentCache) {
                    if (student.faceCrop.empty()) continue;

                    // Compare the camera face with each student in the cache
                    if (m_faceMatcher.compareFaces(student.faceCrop, currentCamFace)) {
                        foundMatch = true;
                        matchedStudent = student;
                        break; // Stop the loop as soon as a matching student is found
                    }
                }
                if (foundMatch) {
                    // [MATCHED FACE] Temporarily lock recognition
                    m_isAlreadyRecognized = true;

                    // Get the current time
                    QString checkInTime = QDateTime::currentDateTime().toString("hh:mm:ss - dd/MM/yyyy");

                    // Send the matched student data to QML
                    emit studentRecognized(
                        matchedStudent.fullName,
                        matchedStudent.className,
                        checkInTime,
                        matchedStudent.getFormattedAvatarUrl()
                        );

                    // Lock recognition for 4 seconds so the UI can display the result before scanning resumes
                    QTimer::singleShot(4000, this, [this]() {
                        m_isAlreadyRecognized = false;
                    });
                } else {
                    // [MISMATCHED FACE] Emit mismatch only if not in the reset wait state
                    emit faceMismatch();
                }
            }
        }

        if (m_shouldCapture) {
            m_shouldCapture = false; // Clear the flag as soon as the capture request is detected

            // 1. Create the directory for student images if it does not exist
            QString dirPath = QCoreApplication::applicationDirPath() + "/student_images";
            QDir().mkpath(dirPath);

            // 2. Generate the image filename from the student ID and capture time
            QString fileName = QString("%1_%2.jpg")
                                   .arg(m_tempId)
                                   .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
            QString fullPath = dirPath + "/" + fileName;

            // 3. Convert the current OpenCV BGR frame to QImage for saving to disk
            cv::Mat rgbMat;
            cv::cvtColor(frame, rgbMat, cv::COLOR_BGR2RGB);
            QImage captureImage(rgbMat.data, rgbMat.cols, rgbMat.rows,
                                static_cast<int>(rgbMat.step),
                                QImage::Format_RGB888);

            // 4. Save the image file and add the information to the database
            if (captureImage.copy().save(fullPath, "JPG")) {
                qDebug() << "Đã chụp và lưu ảnh sinh viên mới tại:" << fullPath;

                // Save the student information to SQL
                if (saveStudentToDb(m_tempId, m_tempName, m_tempClass, fullPath)) {
                    // Update the in-memory cache so the new student can check in immediately
                    loadAllStudentsFromDb();
                }
            } else {
                qDebug() << "Lỗi: Không thể lưu file ảnh sinh viên!";
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
    // Enable the lock flag to prevent duplicate comparisons
    m_isAlreadyRecognized = true;
    // Update the current time
    m_dummyStudent.updateCheckInTime();

    // Emit a signal sending the complete dummy attendance data to QML
    emit studentRecognized(
        m_dummyStudent.fullName,
        m_dummyStudent.className,
        m_dummyStudent.checkInTime,
        QUrl::fromLocalFile(m_dummyStudent.avatarPath).toString()
        );

    // Wait 5 seconds, then automatically re-enable scanning for the next attempt
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
bool CameraController::loadAllStudentsFromDb() {
    m_studentCache.clear();
    if (!QSqlDatabase::database().isOpen()) {
        qDebug() << "ERROR: Database was not opened before loading students!";
        return false;
    }
    QSqlQuery query("SELECT Id, StudentId, NameStudent, Class, pathAvtImage FROM StudyList");

    while (query.next()) {
        StudentData student;
        student.Id = query.value("Id").toInt();
        student.studentId = query.value("StudentId").toString();
        student.fullName = query.value("NameStudent").toString();
        student.className = query.value("Class").toString();
        student.avatarPath = query.value("pathAvtImage").toString();

        // Read the avatar image from disk and extract the standard face crop
        if (!student.avatarPath.isEmpty()) {
            cv::Mat image = cv::imread(student.avatarPath.toStdString());
            if (!image.empty()) {
                student.faceCrop = m_faceMatcher.getFaceCrop(image);
            }
        }

        // Add the student to the in-memory cache
        m_studentCache.append(student);
    }

    qDebug() << "Successfully loaded" << m_studentCache.size() << "students into the RAM cache.";
    return !m_studentCache.isEmpty();
}
bool CameraController::connectToDatabase() {
    // 1. Check whether the database is already open before reopening it
    if (QSqlDatabase::database().isOpen()) {
        return true;
    }

    // 2. Create the default connection
    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC");

    QString connectionString = "DRIVER={ODBC Driver 17 for SQL Server};"
                               "SERVER=LAPTOP-UHPOGM33\\MSSQLSERVER01;" // Or your Server/SQLEXPRESS name
                               "DATABASE=StudentAttendanceCheck;"
                               "Trusted_Connection=Yes;";

    db.setDatabaseName(connectionString);

    if (!db.open()) {
        qDebug() << "Database connection error:" << db.lastError().text();
        return false;
    }

    qDebug() << "SQL Server connection successful!";
    return true;
}