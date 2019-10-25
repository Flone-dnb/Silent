#-------------------------------------------------
#
# Project created by QtCreator 2019-01-30T10:43:46
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Silent
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11


FORMS += \
    ../src/View/ConnectWindow/connectwindow.ui \
    ../src/View/MainWindow/mainwindow.ui \
    ../src/View/SettingsWindow/settingswindow.ui \
    ../src/View/SingleUserSettings/singleusersettings.ui

HEADERS += \
    ../src/Controller/controller.h \
    ../src/Model/AudioService/audioservice.h \
    ../src/Model/NetworkService/networkservice.h \
    ../src/View/ConnectWindow/connectwindow.h \
    ../src/View/MainWindow/mainwindow.h \
    ../src/View/SettingsWindow/settingswindow.h \
    ../src/View/SingleUserSettings/singleusersettings.h \
    ../src/globalparams.h

SOURCES += \
    ../src/Controller/controller.cpp \
    ../src/Model/AudioService/audioservice.cpp \
    ../src/Model/NetworkService/networkservice.cpp \
    ../src/View/ConnectWindow/connectwindow.cpp \
    ../src/View/MainWindow/mainwindow.cpp \
    ../src/View/SingleUserSettings/singleusersettings.cpp \
    ../src/main.cpp \
    ../src/View/SettingsWindow/settingswindow.cpp \

win32
{
        RC_FILE += ../res/file.rc
        OTHER_FILES += ../res/file.rc
}

RESOURCES += ../res/qt_rec_file.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
