QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11 console

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ks7reader.cpp \
    main.cpp \
    mainwindow.cpp \
    snap7/s7_client.cpp \
    snap7/s7_isotcp.cpp \
    snap7/s7_micro_client.cpp \
    snap7/s7_peer.cpp \
    snap7/snap_msgsock.cpp \
    snap7/snap_sysutils.cpp \
    snap7/snap_threads.cpp

HEADERS += \
    ks7reader.h \
    mainwindow.h \
    snap7/s7_client.h \
    snap7/s7_firmware.h \
    snap7/s7_isotcp.h \
    snap7/s7_micro_client.h \
    snap7/s7_partner.h \
    snap7/s7_peer.h \
    snap7/s7_server.h \
    snap7/s7_text.h \
    snap7/s7_types.h \
    snap7/snap7_libmain.h \
    snap7/snap_msgsock.h \
    snap7/snap_platform.h \
    snap7/snap_sysutils.h \
    snap7/snap_tcpsrvr.h \
    snap7/snap_threads.h \
    snap7/sol_threads.h \
    snap7/win_threads.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    开发记录.txt

# 我添加的内容
INCLUDEPATH += $$PWD//snap7

# ws2_32.lib WINDOWS SOCKET库
# winmm.lib windows标准库
LIBS += -lws2_32  -lwinmm
