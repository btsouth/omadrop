QT += core gui qml quick
CONFIG += c++17 link_pkgconfig
PKGCONFIG += fftw3f
TEMPLATE = app
TARGET = omadrop-journey
SOURCES += src/main.cpp src/backend.cpp src/audio.cpp src/scene.cpp src/caption.cpp
HEADERS += src/backend.h src/audio.h src/scene.h src/caption.h
DISTFILES += src/caption-assets/outlines.inc src/caption-assets/generate.py \
    src/caption-assets/README.md src/caption-assets/OFL.txt
RESOURCES += src/resources.qrc
