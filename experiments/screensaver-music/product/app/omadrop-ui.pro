QT += core gui qml quick quickcontrols2 network
CONFIG += c++17
CONFIG -= app_bundle

TEMPLATE = app
TARGET = omadrop-ui

SOURCES += \
    src/main.cpp \
    src/backend.cpp \
    src/theme.cpp

HEADERS += \
    src/backend.h \
    src/theme.h

RESOURCES += src/resources.qrc
