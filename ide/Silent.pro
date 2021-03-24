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

CONFIG += c++17


FORMS += \
    ../src/View/AboutQtWindow/aboutqtwindow.ui \
    ../src/View/AboutWindow/aboutwindow.ui \
    ../src/View/ConnectWindow/connectwindow.ui \
    ../src/View/GlobalMessageWindow/globalmessagewindow.ui \
    ../src/View/MainWindow/mainwindow.ui \
    ../src/View/RoomPassInputWindow/roompassinputwindow.ui \
    ../src/View/SVoiceMeterWidget/svoicemeterwidget.ui \
    ../src/View/SettingsWindow/settingswindow.ui \
    ../src/View/SingleUserSettings/singleusersettings.ui \
    ../src/View/WindowControlWidget/windowcontrolwidget.ui

HEADERS += \
    ../ext/AES/AES.h \
    ../ext/integer/integer.h \
    ../src/Controller/controller.h \
    ../src/Model/AudioService/audioservice.h \
    ../src/Model/NetworkService/networkservice.h \
    ../src/Model/OutputTextType.h \
    ../src/Model/SettingsManager/SettingsFile.h \
    ../src/Model/SettingsManager/settingsmanager.h \
    ../src/Model/User.h \
    ../src/View/AboutQtWindow/aboutqtwindow.h \
    ../src/View/AboutWindow/aboutwindow.h \
    ../src/View/ConnectWindow/connectwindow.h \
    ../src/View/CustomList/SListItem/slistitem.h \
    ../src/View/CustomList/SListItemRoom/slistitemroom.h \
    ../src/View/CustomList/SListItemUser/slistitemuser.h \
    ../src/View/CustomList/SListWidget/slistwidget.h \
    ../src/View/CustomQPlainTextEdit/customqplaintextedit.h \
    ../src/View/GlobalMessageWindow/globalmessagewindow.h \
    ../src/View/MainWindow/mainwindow.h \
    ../src/View/RoomPassInputWindow/roompassinputwindow.h \
    ../src/View/SVoiceMeterWidget/svoicemeterwidget.h \
    ../src/View/SettingsWindow/settingswindow.h \
    ../src/View/SingleUserSettings/singleusersettings.h \
    ../src/Model/net_params.h \
    ../src/View/StyleAndInfoPaths.h \
    ../src/View/WindowControlWidget/windowcontrolwidget.h

SOURCES += \
    ../ext/AES/AES.cpp \
    ../ext/integer/integer.cpp \
    ../src/Controller/controller.cpp \
    ../src/Model/AudioService/audioservice.cpp \
    ../src/Model/NetworkService/networkservice.cpp \
    ../src/Model/SettingsManager/settingsmanager.cpp \
    ../src/View/AboutQtWindow/aboutqtwindow.cpp \
    ../src/View/AboutWindow/aboutwindow.cpp \
    ../src/View/ConnectWindow/connectwindow.cpp \
    ../src/View/CustomList/SListItem/slistitem.cpp \
    ../src/View/CustomList/SListItemRoom/slistitemroom.cpp \
    ../src/View/CustomList/SListItemUser/slistitemuser.cpp \
    ../src/View/CustomList/SListWidget/slistwidget.cpp \
    ../src/View/CustomQPlainTextEdit/customqplaintextedit.cpp \
    ../src/View/GlobalMessageWindow/globalmessagewindow.cpp \
    ../src/View/MainWindow/mainwindow.cpp \
    ../src/View/RoomPassInputWindow/roompassinputwindow.cpp \
    ../src/View/SVoiceMeterWidget/svoicemeterwidget.cpp \
    ../src/View/SingleUserSettings/singleusersettings.cpp \
    ../src/View/WindowControlWidget/windowcontrolwidget.cpp \
    ../src/main.cpp \
    ../src/View/SettingsWindow/settingswindow.cpp \

RC_ICONS = ../res/icons/appMainIcon.ico

RESOURCES += ../res/qt_rec_file.qrc

INCLUDEPATH += "../src"
INCLUDEPATH += "../ext"

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

