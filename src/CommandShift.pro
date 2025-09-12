QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

macx {
QMAKE_APPLE_DEVICE_ARCHS = arm64
QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.15
}

CONFIG += c++17

ICON = icons/icon.icns

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    keypresscatcher.cpp \
    main.cpp

HEADERS += \
    constants.h \
    keypresscatcher.h

LIBS += -framework ApplicationServices

# Use custom Info.plist
macx {
    QMAKE_INFO_PLIST = $$PWD/Info.plist
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    app_resources.qrc

