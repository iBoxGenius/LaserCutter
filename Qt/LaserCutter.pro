TARGET = LaserCutter
TEMPLATE = app


QT       += core gui
QT       += multimedia multimediawidgets
QT       += svg xml
QT       += serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    dragwidget.cpp \
    filehandling.cpp \
    hotkeys.cpp \
    lasercutter.cpp \
    main.cpp \
    svglabel.cpp

HEADERS += \
    dragwidget.h \
    filehandling.h \
    hotkeys.h \
    lasercutter.h \
    svglabel.h

FORMS += \
    hotkeys.ui \
    lasercutter.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

INCLUDEPATH += E:\sw\openCV\opencv\build\include
#INCLUDEPATH += E:\sw\openCV\opencv\build\include/core

INCLUDEPATH += E:\sw\openCV\opencv-build\install\include
LIBS += E:\sw\openCV\opencv-build\install\x64\mingw\lib\libopencv_aruco451.dll.a

LIBS += E:\sw\openCV\opencv-build\install\x64\mingw\lib\libopencv_core451.dll.a
LIBS += E:\sw\openCV\opencv-build\install\x64\mingw\lib\libopencv_highgui451.dll.a
LIBS += E:\sw\openCV\opencv-build\install\x64\mingw\lib\libopencv_imgcodecs451.dll.a
LIBS += E:\sw\openCV\opencv-build\install\x64\mingw\lib\libopencv_imgproc451.dll.a
LIBS += E:\sw\openCV\opencv-build\install\x64\mingw\lib\libopencv_features2d451.dll.a
LIBS += E:\sw\openCV\opencv-build\install\x64\mingw\lib\libopencv_calib3d451.dll.a
LIBS += E:\sw\openCV\opencv-build\install\x64\mingw\lib\libopencv_videoio451.dll.a

LIBS += -L"E:\sw\openCV\opencv-build\install\x64\mingw\bin"

RESOURCES += \
    resources.qrc
