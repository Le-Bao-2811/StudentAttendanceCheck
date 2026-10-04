cmake_minimum_required(VERSION 3.16)

project(StudentAttendanceCheck VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)

set(OpenCV_DIR "E:/OpenCVGit/OpenCV-MinGW-Build-OpenCV-4.1.0-x64/x64/mingw/lib")
find_package(OpenCV REQUIRED)

qt_standard_project_setup(REQUIRES 6.10)

# Content of downloaded design from Figma to Qt or similar
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/importedcontent/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/importedcontent)
endif()

include_directories(${OpenCV_INCLUDE_DIRS})
link_directories("E:/opencv/mingw-build/lib")
link_directories("E:/opencv/mingw-build/x64/mingw/lib")

qt_add_executable(appStudentAttendanceCheck
    main.cpp
)

qt_add_qml_module(appStudentAttendanceCheck
    URI StudentAttendanceCheck
    QML_FILES
        Main.qml
        SOURCES CameraProvider.h
        SOURCES CameraProvider.cpp
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appStudentAttendanceCheck PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appStudentAttendanceCheck
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appStudentAttendanceCheck
    PRIVATE Qt6::Quick
     ${OpenCV_LIBS}
)

install(TARGETS appStudentAttendanceCheck
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
