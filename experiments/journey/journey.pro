QT += core gui qml quick
CONFIG += c++17 link_pkgconfig
PKGCONFIG += fftw3f
TEMPLATE = app
TARGET = omadrop-journey
SOURCES += src/main.cpp src/backend.cpp src/audio.cpp src/scene.cpp
HEADERS += src/backend.h src/audio.h src/scene.h
RESOURCES += src/resources.qrc
